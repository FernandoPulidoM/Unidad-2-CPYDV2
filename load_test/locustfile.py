from typing import Any, List, Dict
from locust import HttpUser, task, between
import uuid
import random


class TournamentUser(HttpUser):
    # Tiempo de espera entre tareas para no hacer spam extremo
    wait_time = between(1, 3)

    # Equipos por torneo
    teams_per_tournament = 32

    #
    # Helpers generales
    #
    @staticmethod
    def _extract_id_from_location(location: str | None) -> str | None:
        """
        Convierte algo como:
          - "/teams/2f0c..."
          - "http://localhost:8080/teams/2f0c..."
        en solo "2f0c..."
        """
        if not location:
            return None
        return location.rstrip("/").rsplit("/", 1)[-1]

    #
    # Creacion de entidades basicas
    #
    def create_teams(self, count: int) -> List[Dict[str, str]]:
        """
        Crea 'count' equipos.
        Regresa una lista de dicts con { "id": ..., "name": ... }.
        """
        teams: List[Dict[str, str]] = []

        for _ in range(count):
            name = f"Team {uuid.uuid4()}"
            payload = {"name": name}

            with self.client.post(
                    "/teams",
                    json=payload,
                    catch_response=True,
                    name="POST /teams"
            ) as response:
                if response.status_code in (200, 201):
                    location = response.headers.get("Location") or response.headers.get("location")
                    team_id = self._extract_id_from_location(location)
                    if team_id:
                        teams.append({"id": team_id, "name": name})
                    else:
                        response.failure("Team created but Location header missing or invalid")
                else:
                    response.failure(f"Team creation failed: {response.status_code}")

        return teams

    def create_tournament(self) -> str | None:
        """
        Crea un torneo y regresa su id.
        Ajusta el JSON si tu API pide mas campos (format, type, etc).
        """
        payload = {
            "name": f"Tournament - {uuid.uuid4()}"
            # Si tu API necesita mas campos, los agregas aqui:
            # "format": "DOUBLE_ELIMINATION",
            # "type": "SOCCER"
        }

        with self.client.post(
                "/tournaments",
                json=payload,
                catch_response=True,
                name="POST /tournaments"
        ) as response:
            if response.status_code in (200, 201):
                location = response.headers.get("Location") or response.headers.get("location")
                tid = self._extract_id_from_location(location)
                if tid:
                    return tid
                else:
                    response.failure("Tournament created but Location header missing or invalid")
            else:
                response.failure(f"Tournament creation failed: {response.status_code}")

        return None

    def create_group(self, tournament_id: str) -> str | None:
        """
        Crea un grupo para el torneo y regresa el id.
        """
        payload = {
            "name": f"Group - {uuid.uuid4()}"
        }

        with self.client.post(
                f"/tournaments/{tournament_id}/groups",
                json=payload,
                catch_response=True,
                name="POST /tournaments/{tournamentId}/groups"
        ) as response:
            if response.status_code in (200, 201):
                location = response.headers.get("Location") or response.headers.get("location")
                gid = self._extract_id_from_location(location)
                if gid:
                    return gid
                else:
                    response.failure("Group created but Location header missing or invalid")
            else:
                response.failure(f"Group creation failed: {response.status_code}")

        return None

    def add_teams_to_group(self, tournament_id: str, group_id: str, teams: List[Dict[str, str]]) -> None:
        """
        PATCH /tournaments/{tId}/groups/{gId}/teams
        Body: [{ "id": "...", "name": "..." }, ...]
        """
        body = [{"id": t["id"], "name": t["name"]} for t in teams]

        with self.client.patch(
                f"/tournaments/{tournament_id}/groups/{group_id}/teams",
                json=body,
                catch_response=True,
                name="PATCH /tournaments/{tId}/groups/{gId}/teams"
        ) as response:
            if response.status_code not in (200, 204):
                response.failure(
                    f"Add teams to group failed: {response.status_code}, body={response.text}"
                )

    #
    # Endpoints de matches
    #
    def generate_group_matches(self, tournament_id: str) -> bool:
        """
        POST /tournaments/{id}/matches/generate
        Genera los partidos de fase de grupos.
        """
        with self.client.post(
                f"/tournaments/{tournament_id}/matches/generate",
                catch_response=True,
                name="POST /tournaments/{id}/matches/generate"
        ) as response:
            if response.status_code in (200, 201):
                return True
            else:
                response.failure(
                    f"Generate group matches failed: {response.status_code}, body={response.text}"
                )
                return False

    def generate_knockout_phase(self, tournament_id: str) -> bool:
        """
        POST /tournaments/{id}/matches/generate-knockout
        Genera fase de eliminacion directa.
        """
        with self.client.post(
                f"/tournaments/{tournament_id}/matches/generate-knockout",
                catch_response=True,
                name="POST /tournaments/{id}/matches/generate-knockout"
        ) as response:
            if response.status_code in (200, 201):
                return True
            else:
                response.failure(
                    f"Generate knockout phase failed: {response.status_code}, body={response.text}"
                )
                return False

    def list_matches(self, tournament_id: str, filter_status: str | None = None) -> list[dict]:
        """
        GET /tournaments/{id}/matches
        Opcional: ?showMatches=pending
        """
        params = {}
        name = "GET /tournaments/{id}/matches"
        if filter_status:
            params["showMatches"] = filter_status
            name = "GET /tournaments/{id}/matches?showMatches=pending"

        with self.client.get(
                f"/tournaments/{tournament_id}/matches",
                params=params or None,
                catch_response=True,
                name=name
        ) as response:
            if response.status_code == 200:
                try:
                    data = response.json()
                    # Se espera una lista de matches
                    if isinstance(data, list):
                        return data
                    else:
                        # Si fuera un objeto con otra estructura, ajustas aqui.
                        return []
                except Exception as e:
                    response.failure(f"Failed to parse matches JSON: {e}")
                    return []
            else:
                response.failure(
                    f"List matches failed: {response.status_code}, body={response.text}"
                )
                return []

    def patch_score(self, tournament_id: str, match_id: str, home: int, visitor: int) -> None:
        """
        PATCH /tournaments/{id}/matches/{matchId}
        Body: { "score": { "home": X, "visitor": Y } }
        """
        body = {
            "score": {
                "home": home,
                "visitor": visitor
            }
        }

        with self.client.patch(
                f"/tournaments/{tournament_id}/matches/{match_id}",
                json=body,
                catch_response=True,
                name="PATCH /tournaments/{id}/matches/{matchId}"
        ) as response:
            if response.status_code not in (200, 204):
                response.failure(
                    f"Patch score failed: {response.status_code}, body={response.text}"
                )

    def play_all_pending_matches(self, tournament_id: str) -> None:
        """
        Busca todos los matches con status pending y les pone un score
        aleatorio SIN empate (para evitar rechazos por advancement).
        """
        matches = self.list_matches(tournament_id, filter_status="pending")
        for m in matches:
            match_id = m.get("id")
            if not match_id:
                continue

            # Score aleatorio sin empate
            home = random.randint(0, 5)
            visitor = random.randint(0, 5)
            if home == visitor:
                visitor = (visitor + 1) % 6

            self.patch_score(tournament_id, match_id, home, visitor)

    def get_tournament_status(self, tournament_id: str) -> None:
        """
        GET /tournaments/{id}/status
        Solo para verificar que el servicio responde, no validamos contenido aqui.
        """
        with self.client.get(
                f"/tournaments/{tournament_id}/status",
                catch_response=True,
                name="GET /tournaments/{id}/status"
        ) as response:
            if response.status_code != 200:
                response.failure(
                    f"Get tournament status failed: {response.status_code}, body={response.text}"
                )
            else:
                # Si quieres revisar algo del JSON, puedes hacer response.json()
                try:
                    _ = response.json()
                except Exception:
                    # No lo marcamos como failure si el codigo fue 200, a menos que quieras ser mas estricto
                    pass

    #
    # Tarea principal: simular TODO el torneo
    #
    @task
    def full_tournament_flow(self):
        """
        Flujo completo:
          1) Crear equipos
          2) Crear torneo
          3) Crear grupo
          4) Agregar equipos al grupo
          5) Generar matches de grupos
          6) Jugar todos los matches pendientes de grupos
          7) Generar knockout
          8) Jugar todos los matches pendientes de knockout
          9) Consultar status final del torneo
        """

        # 1) Equipos
        teams = self.create_teams(self.teams_per_tournament)
        if not teams:
            return

        # 2) Torneo
        tournament_id = self.create_tournament()
        if not tournament_id:
            return

        # 3) Grupo
        group_id = self.create_group(tournament_id)
        if not group_id:
            return

        # 4) Agregar equipos al grupo
        self.add_teams_to_group(tournament_id, group_id, teams)

        # 5) Generar matches de fase de grupos
        if not self.generate_group_matches(tournament_id):
            return

        # 6) Jugar todos los pending (fase de grupos)
        self.play_all_pending_matches(tournament_id)

        # 7) Generar fase knockout
        if not self.generate_knockout_phase(tournament_id):
            return

        # 8) Jugar todos los pending (knockout)
        self.play_all_pending_matches(tournament_id)

        # 9) Consultar status del torneo
        self.get_tournament_status(tournament_id)
