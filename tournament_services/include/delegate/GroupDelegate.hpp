#ifndef SERVICE_GROUP_DELEGATE_HPP
#define SERVICE_GROUP_DELEGATE_HPP

#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <expected>
#include <exception>

// Interfaz
#include "IGroupDelegate.hpp"

// Dominios
#include "../../../tournament_common/include/domain/Group.hpp"
#include "../../../tournament_common/include/domain/Team.hpp"
#include "../../../tournament_common/include/domain/Tournament.hpp"

// Repos
#include "persistence/repository/IRepository.hpp"
#include "persistence/repository/GroupRepository.hpp"   // IGroupRepository

class GroupDelegate : public IGroupDelegate {
    std::shared_ptr<IRepository<domain::Tournament, std::string>>      tournamentRepository;
    std::shared_ptr<IGroupRepository>                                  groupRepository;
    std::shared_ptr<IRepository<domain::Team, std::string_view>>       teamRepository;

public:
    inline GroupDelegate(
        const std::shared_ptr<IRepository<domain::Tournament, std::string>>& tRepo,
        const std::shared_ptr<IGroupRepository>& gRepo,
        const std::shared_ptr<IRepository<domain::Team, std::string_view>>& teamRepo
    )
        : tournamentRepository(tRepo)
        , groupRepository(gRepo)
        , teamRepository(teamRepo)
    {}

    // ==== CREATE GROUP =======================================================
    inline std::expected<std::string, std::string>
    CreateGroup(const std::string_view& tournamentId, const domain::Group& group) override
    {
        // 1) Validar torneo
        auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
        if (tournament == nullptr) {
            return std::unexpected("Tournament doesn't exist");
        }

        // Copiamos el grupo y le inyectamos el TournamentId correcto
        domain::Group g = group;
        g.TournamentId() = tournament->Id();

        // 2) Grupo duplicado (mismo id dentro del torneo, si viene id)
        if (!g.Id().empty()) {
            auto existing =
                groupRepository->FindByTournamentIdAndGroupId(tournamentId, g.Id());
            if (existing != nullptr) {
                return std::unexpected("Group already exists");
            }
        }

        // 3) Maximo de equipos en el grupo (usa MaxTeamsPerGroup)
        const auto maxTeams = tournament->Format().MaxTeamsPerGroup();
        if (static_cast<int>(g.Teams().size()) > maxTeams) {
            // Mensaje que esperan los tests
            return std::unexpected("Group at max capacity");
        }

        // 4) Validar cada equipo
        for (const auto& t : g.Teams()) {
            if (t.Id.empty()) {
                return std::unexpected("Team id required");
            }

            // 4a) Existe el equipo?
            auto persisted = teamRepository->ReadById(t.Id);
            if (persisted == nullptr) {
                // Tests de CreateGroup_TeamMissing esperan este texto generico
                return std::unexpected("Team doesn't exist");
            }

            // 4b) No debe estar ya en algun grupo del torneo
            auto groupWithTeam =
                groupRepository->FindByTournamentIdAndTeamId(tournamentId, t.Id);
            if (groupWithTeam != nullptr) {
                return std::unexpected(
                    "Team " + t.Id +
                    " already exists in tournament " +
                    std::string(tournamentId)
                );
            }
        }

        // 5) Crear el grupo, tratando "" como grupo duplicado
        try {
            auto id = groupRepository->Create(g);
            if (id.empty()) {
                // Casos donde el repo no crea por duplicado
                return std::unexpected("Group already exists");
            }
            return id;
        } catch (const std::exception& e) {
            return std::unexpected(std::string("Error creating group: ") + e.what());
        }
    }

    // ==== GET GROUPS =========================================================
    inline std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>
    GetGroups(const std::string_view& tournamentId) override
    {
        auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
        if (tournament == nullptr) {
            return std::unexpected("Tournament doesn't exist");
        }

        try {
            return groupRepository->FindByTournamentId(tournamentId);
        } catch (const std::exception&) {
            return std::unexpected("Error when reading to DB");
        }
    }

    // ==== GET GROUP ==========================================================
    inline std::expected<std::shared_ptr<domain::Group>, std::string>
    GetGroup(const std::string_view& tournamentId,
             const std::string_view& groupId) override
    {
        auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
        if (tournament == nullptr) {
            return std::unexpected("Tournament doesn't exist");
        }

        try {
            auto g = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
            if (g == nullptr) {
                return std::unexpected("Group doesn't exist");
            }
            return g;
        } catch (const std::exception&) {
            return std::unexpected("Error when reading to DB");
        }
    }

    // ==== UPDATE GROUP =======================================================
    inline std::expected<void, std::string>
    UpdateGroup(const std::string_view& tournamentId,
                const domain::Group& group) override
    {
        if (group.Id().empty()) {
            return std::unexpected("Group id required");
        }
        if (group.Name().empty()) {
            return std::unexpected("Group name required");
        }

        auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
        if (tournament == nullptr) {
            return std::unexpected("Tournament doesn't exist");
        }

        auto existing = groupRepository->FindByTournamentIdAndGroupId(
            tournamentId, group.Id());
        if (existing == nullptr) {
            return std::unexpected("Group doesn't exist");
        }

        try {
            auto updatedId = groupRepository->Update(group);
            if (updatedId.empty()) {
                return std::unexpected("Failed to update group");
            }
            return {};
        } catch (const std::exception& e) {
            return std::unexpected(std::string("Error updating group: ") + e.what());
        }
    }

    // ==== REMOVE GROUP =======================================================
    inline std::expected<void, std::string>
    RemoveGroup(const std::string_view& tournamentId,
                const std::string_view& groupId) override
    {
        auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
        if (tournament == nullptr) {
            return std::unexpected("Tournament doesn't exist");
        }

        auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
        if (group == nullptr) {
            return std::unexpected("Group doesn't exist");
        }

        try {
            groupRepository->Delete(std::string(groupId));
            return {};
        } catch (const std::exception& e) {
            return std::unexpected(std::string("Error deleting group: ") + e.what());
        }
    }

    // ==== UPDATE TEAMS (reemplaza equipos del grupo) =========================
    inline std::expected<void, std::string>
    UpdateTeams(const std::string_view& tournamentId,
                const std::string_view& groupId,
                const std::vector<domain::Team>& teams) override
    {
        auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
        if (tournament == nullptr) {
            return std::unexpected("Tournament doesn't exist");
        }

        auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
        if (group == nullptr) {
            return std::unexpected("Group doesn't exist");
        }

        const auto maxTeams = tournament->Format().MaxTeamsPerGroup();
        if (static_cast<int>(group->Teams().size() + teams.size()) > maxTeams) {
            return std::unexpected("Group at max capacity");
        }

        // 1) Validar duplicados a nivel torneo
        for (const auto& t : teams) {
            if (t.Id.empty()) {
                return std::unexpected("Team id required");
            }

            auto existing =
                groupRepository->FindByTournamentIdAndTeamId(tournamentId, t.Id);
            if (existing != nullptr) {
                return std::unexpected(
                    "Team " + t.Id +
                    " already exists in tournament " +
                    std::string(tournamentId)
                );
            }
        }

        // 2) Verificar que todos los equipos existan y agregarlos
        for (const auto& t : teams) {
            auto persisted = teamRepository->ReadById(t.Id);
            if (persisted == nullptr) {
                return std::unexpected("Team " + t.Id + " doesn't exist");
            }

            groupRepository->UpdateGroupAddTeam(groupId, persisted);
        }

        return {};
    }

    // ==== ADD TEAM TO GROUP (un solo equipo) =================================
    inline std::expected<void, std::string>
    AddTeamToGroup(const std::string_view& tournamentId,
                   const std::string_view& groupId,
                   const std::string_view& teamId) override
    {
        auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
        if (tournament == nullptr) {
            return std::unexpected("Tournament doesn't exist");
        }

        // 1) Verificar grupo
        auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
        if (group == nullptr) {
            return std::unexpected("Group doesn't exist");
        }

        const auto maxTeams = tournament->Format().MaxTeamsPerGroup();

        // 2) Equipo existe? (los tests de TeamNotExist esperan "Team doesn't exist")
        auto team = teamRepository->ReadById(teamId);
        if (team == nullptr) {
            return std::unexpected("Team doesn't exist");
        }

        // 3) Equipo ya esta en algun grupo del torneo
        auto existing =
            groupRepository->FindByTournamentIdAndTeamId(tournamentId,
                                                         std::string(teamId));
        if (existing != nullptr) {
            return std::unexpected(
                "Team " + std::string(teamId) +
                " already exists in tournament " +
                std::string(tournamentId)
            );
        }

        // 4) Capacidad del grupo (aqui ya se llamo ReadById y FindByTournamentIdAndTeamId)
        if (static_cast<int>(group->Teams().size()) >= maxTeams) {
            return std::unexpected("Group is full");
        }

        // 5) Agregar equipo
        try {
            groupRepository->UpdateGroupAddTeam(groupId, team);
            return {};
        } catch (const std::exception& e) {
            return std::unexpected(
                std::string("Error adding team to group: ") + e.what()
            );
        }
    }
};

#endif /* SERVICE_GROUP_DELEGATE_HPP */
