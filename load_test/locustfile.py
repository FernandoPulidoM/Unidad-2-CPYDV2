from typing import Any, Optional, List
from locust import HttpUser, task, between
import json
import uuid
import random

class TournamentUser(HttpUser):
    """
    Usuario de Locust que simula el flujo completo de un torneo:
    1. Crear 32 equipos en bulk
    2. Crear torneo
    3. Crear 8 grupos
    4. Asignar 4 equipos a cada grupo
    5. Generar fase de grupos (48 partidos)
    6. Jugar todos los partidos
    7. Sistema avanza automáticamente: Octavos -> Cuartos -> Semis -> Final
    """

    # Tiempo de espera entre tareas (simulación de usuarios reales)
    wait_time = between(1, 3)

    def on_start(self):
        """Se ejecuta una vez al iniciar cada usuario"""
        self.tournament_id: Optional[str] = None
        self.group_ids: List[str] = []
        self.team_ids: List[str] = []
        self.match_ids: List[str] = []

    # ============================================
    # HELPERS: Crear entidades
    # ============================================

    def create_team(self) -> Optional[str]:
        """Crear un equipo usando el endpoint normal /teams"""
        team_data = {"name": f"Team-{uuid.uuid4().hex[:8]}"}

        with self.client.post(
                "/teams",
                json=team_data,
                catch_response=True,
                name="POST /teams"
        ) as response:
            if response.status_code in [200, 201]:
                location = response.headers.get("Location") or response.headers.get("location")
                response.success()
                return location
            else:
                response.failure(f"Team creation failed: {response.status_code}")
                return None


    def create_teams_bulk(self, count: int = 32) -> List[str]:
        # """Crear equipos uno por uno usando /teams ya existente"""
        ids = []
        for _ in range(count):
            new_id = self.create_team()
            if new_id:
                ids.append(new_id)
        return ids

    def create_tournament(self) -> Optional[str]:
        """Crear torneo"""
        tournament_data = {
            "name": f"Tournament-{uuid.uuid4().hex[:8]}"
        }

        with self.client.post(
                "/tournaments",
                json=tournament_data,
                catch_response=True,
                name="POST /tournaments"
        ) as response:
            if response.status_code in [200, 201]:
                location = response.headers.get("Location") or response.headers.get("location")
                response.success()
                return location
            else:
                response.failure(f"Tournament creation failed: {response.status_code}")
                return None

    def create_group(self, tournament_id: str, group_name: str) -> Optional[str]:
        """Crear grupo en un torneo"""
        group_data = {"name": group_name}

        with self.client.post(
                f"/tournaments/{tournament_id}/groups",
                json=group_data,
                catch_response=True,
                name="POST /tournaments/<tid>/groups"
        ) as response:
            if response.status_code in [200, 201]:
                location = response.headers.get("Location") or response.headers.get("location")
                response.success()
                return location
            else:
                response.failure(f"Group creation failed: {response.status_code}")
                return None

    def add_teams_to_group(self, tournament_id: str, group_id: str, team_ids: List[str]) -> bool:
        """Agregar equipos a un grupo"""
        teams_data = [{"id": team_id} for team_id in team_ids]

        with self.client.patch(
                f"/tournaments/{tournament_id}/groups/{group_id}/teams",
                json=teams_data,
                catch_response=True,
                name="PATCH /tournaments/<tid>/groups/<gid>/teams"
        ) as response:
            if response.status_code in [200, 204]:
                response.success()
                return True
            else:
                response.failure(f"Add teams failed: {response.status_code}")
                return False

    # ============================================
    # HELPERS: Fase de partidos
    # ============================================

    def generate_matches(self, tournament_id: str) -> bool:
        """Generar fase de grupos (48 partidos)"""
        with self.client.post(
                f"/tournaments/{tournament_id}/matches/generate",
                catch_response=True,
                name="POST /tournaments/<tid>/matches/generate"
        ) as response:
            if response.status_code in [200, 201]:
                response.success()
                return True
            else:
                response.failure(f"Generate matches failed: {response.status_code}")
                return False

    def get_pending_matches(self, tournament_id: str) -> List[dict]:
        """Obtener partidos pendientes"""
        with self.client.get(
                f"/tournaments/{tournament_id}/matches?showMatches=pending",
                catch_response=True,
                name="GET /tournaments/<tid>/matches?pending"
        ) as response:
            if response.status_code == 200:
                matches = response.json()
                response.success()
                return matches
            else:
                response.failure(f"Get matches failed: {response.status_code}")
                return []

    def play_match(self, tournament_id: str, match_id: str) -> bool:
        """Jugar un partido (asignar score aleatorio)"""
        score_data = {
            "score": {
                "home": random.randint(0, 5),
                "visitor": random.randint(0, 5)
            }
        }

        with self.client.patch(
                f"/tournaments/{tournament_id}/matches/{match_id}",
                json=score_data,
                catch_response=True,
                name="PATCH /tournaments/<tid>/matches/<mid>"
        ) as response:
            if response.status_code in [200, 204]:
                response.success()
                return True
            else:
                response.failure(f"Play match failed: {response.status_code}")
                return False

    def get_tournament_status(self, tournament_id: str) -> Optional[dict]:
        """Obtener estado del torneo"""
        with self.client.get(
                f"/tournaments/{tournament_id}/status",
                catch_response=True,
                name="GET /tournaments/<tid>/status"
        ) as response:
            if response.status_code == 200:
                status = response.json()
                response.success()
                return status
            else:
                response.failure(f"Get status failed: {response.status_code}")
                return None

    # ============================================
    # TAREAS PRINCIPALES
    # ============================================

    @task(1)
    def read_teams(self):
        """Tarea simple: leer todos los equipos"""
        with self.client.get("/teams", name="GET /teams") as response:
            if response.status_code != 200:
                response.failure(f"Get teams failed: {response.status_code}")

    @task(1)
    def read_tournaments(self):
        """Tarea simple: leer todos los torneos"""
        with self.client.get("/tournaments", name="GET /tournaments") as response:
            if response.status_code != 200:
                response.failure(f"Get tournaments failed: {response.status_code}")

    @task(5)
    def complete_tournament_flow(self):
        """
        Flujo completo: Crear torneo, grupos, equipos y jugar todos los partidos
        Este es el test más pesado y realista
        """
        print("\n=== Iniciando flujo completo de torneo ===")

        # 1. Crear 32 equipos en bulk
        print("1. Creando 32 equipos...")
        team_ids = self.create_teams_bulk(32)
        if not team_ids or len(team_ids) < 32:
            print(f"❌ Error: Solo se crearon {len(team_ids)} equipos")
            return
        print(f"✅ Creados {len(team_ids)} equipos")

        # 2. Crear torneo
        print("2. Creando torneo...")
        tournament_id = self.create_tournament()
        if not tournament_id:
            print("❌ Error: No se pudo crear torneo")
            return
        print(f"✅ Torneo creado: {tournament_id}")

        # 3. Crear 8 grupos
        print("3. Creando 8 grupos...")
        group_ids = []
        for i in range(8):
            group_id = self.create_group(tournament_id, f"Group-{chr(65+i)}")
            if group_id:
                group_ids.append(group_id)

        if len(group_ids) != 8:
            print(f"❌ Error: Solo se crearon {len(group_ids)} grupos")
            return
        print(f"✅ Creados {len(group_ids)} grupos")

        # 4. Asignar 4 equipos a cada grupo
        print("4. Asignando equipos a grupos...")
        for i, group_id in enumerate(group_ids):
            team_chunk = team_ids[i*4:(i+1)*4]
            success = self.add_teams_to_group(tournament_id, group_id, team_chunk)
            if not success:
                print(f"❌ Error: No se pudieron asignar equipos al grupo {i+1}")
                return
        print("✅ Equipos asignados a todos los grupos")

        # 5. Generar fase de grupos (48 partidos)
        print("5. Generando fase de grupos (48 partidos)...")
        if not self.generate_matches(tournament_id):
            print("❌ Error: No se pudieron generar partidos")
            return
        print("✅ Fase de grupos generada")

        # 6. Jugar TODOS los partidos (fase de grupos + playoffs)
        print("6. Jugando todos los partidos del torneo...")
        total_matches_played = 0
        max_iterations = 100  # Prevenir loops infinitos
        iteration = 0

        while iteration < max_iterations:
            iteration += 1

            # Obtener partidos pendientes
            pending_matches = self.get_pending_matches(tournament_id)

            if not pending_matches:
                print("✅ No hay más partidos pendientes")
                break

            print(f"   → Iteración {iteration}: {len(pending_matches)} partidos pendientes")

            # Jugar todos los partidos pendientes
            for match in pending_matches:
                match_id = match.get("id")
                if match_id:
                    if self.play_match(tournament_id, match_id):
                        total_matches_played += 1

            # Verificar estado del torneo
            status = self.get_tournament_status(tournament_id)
            if status:
                current_phase = status.get("currentPhase", "unknown")
                print(f"   → Fase actual: {current_phase}")

                if current_phase == "completed":
                    print("🏆 ¡Torneo completado!")
                    break

        print(f"✅ Total de partidos jugados: {total_matches_played}")

        # 7. Verificar estado final
        print("7. Verificando estado final...")
        final_status = self.get_tournament_status(tournament_id)
        if final_status:
            print(f"   Total de partidos: {final_status.get('totalMatches', 0)}")
            print(f"   Fase final: {final_status.get('currentPhase', 'unknown')}")

            if final_status.get('totalMatches') == 63 or final_status.get('totalMatches') == 64:
                print("✅ Torneo completado correctamente (63-64 partidos)")
            else:
                print(f"⚠️  Advertencia: Total de partidos inesperado")

        print("=== Flujo completo finalizado ===\n")

    @task(2)
    def partial_tournament_flow(self):
        """
        Flujo parcial: Crear torneo y grupos, pero solo jugar fase de grupos
        Más rápido para testing de carga
        """
        # Crear equipos
        team_ids = self.create_teams_bulk(32)
        if not team_ids or len(team_ids) < 32:
            return

        # Crear torneo
        tournament_id = self.create_tournament()
        if not tournament_id:
            return

        # Crear 8 grupos y asignar equipos
        for i in range(8):
            group_id = self.create_group(tournament_id, f"Group-{chr(65+i)}")
            if group_id:
                team_chunk = team_ids[i*4:(i+1)*4]
                self.add_teams_to_group(tournament_id, group_id, team_chunk)

        # Generar y jugar solo fase de grupos
        self.generate_matches(tournament_id)

        # Jugar solo los primeros 48 partidos (fase de grupos)
        pending_matches = self.get_pending_matches(tournament_id)
        for match in pending_matches[:48]:  # Limitar a 48
            match_id = match.get("id")
            if match_id:
                self.play_match(tournament_id, match_id)


# ============================================
# Configuración de ejecución
# ============================================

if __name__ == "__main__":
    import os
    os.system("locust -f locustfile.py --host=http://localhost:8081")