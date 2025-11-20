#ifndef SERVICE_GROUP_DELEGATE_HPP
#define SERVICE_GROUP_DELEGATE_HPP

#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <expected>
#include <exception>

// Interfaz del delegate
#include "IGroupDelegate.hpp"

// Dominios
#include "../../../tournament_common/include/domain/Group.hpp"
#include "../../../tournament_common/include/domain/Team.hpp"
#include "../../../tournament_common/include/domain/Tournament.hpp"

// Repos (interfaces y defs)
#include "persistence/repository/IRepository.hpp"
#include "persistence/repository/GroupRepository.hpp" // define IGroupRepository / GroupRepository

// IMPORTANTE: IRepository e IGroupRepository estan en el global namespace
// (no uses persistence:: para ellos)

class GroupDelegate : public IGroupDelegate {
    std::shared_ptr<IRepository<domain::Tournament, std::string>> tournamentRepository;
    std::shared_ptr<IGroupRepository> groupRepository;
    std::shared_ptr<IRepository<domain::Team, std::string_view>> teamRepository;

public:
    inline GroupDelegate(
        const std::shared_ptr<IRepository<domain::Tournament, std::string>>& tournamentRepository,
        const std::shared_ptr<IGroupRepository>& groupRepository,
        const std::shared_ptr<IRepository<domain::Team, std::string_view>>& teamRepository
    );

    std::expected<std::string, std::string>
    CreateGroup(const std::string_view& tournamentId, const domain::Group& group) override;

    std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>
    GetGroups(const std::string_view& tournamentId) override;

    std::expected<std::shared_ptr<domain::Group>, std::string>
    GetGroup(const std::string_view& tournamentId, const std::string_view& groupId) override;

    std::expected<void, std::string>
    UpdateGroup(const std::string_view& tournamentId, const domain::Group& group) override;

    std::expected<void, std::string>
    RemoveGroup(const std::string_view& tournamentId, const std::string_view& groupId) override;

    std::expected<void, std::string>
    UpdateTeams(const std::string_view& tournamentId, const std::string_view& groupId,
                const std::vector<domain::Team>& teams) override;

    // Agregar UN equipo a un grupo
    std::expected<void, std::string>
    AddTeamToGroup(const std::string_view& tournamentId, const std::string_view& groupId,
                   const std::string_view& teamId) override;
};

// ================= IMPLEMENTACION =================

inline GroupDelegate::GroupDelegate(
    const std::shared_ptr<IRepository<domain::Tournament, std::string>>& tournamentRepository,
    const std::shared_ptr<IGroupRepository>& groupRepository,
    const std::shared_ptr<IRepository<domain::Team, std::string_view>>& teamRepository
)
    : tournamentRepository(tournamentRepository),
      groupRepository(groupRepository),
      teamRepository(teamRepository) {}

inline std::expected<std::string, std::string>
GroupDelegate::CreateGroup(const std::string_view& tournamentId, const domain::Group& group) {
    // 1) Validar torneo
    auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
    if (tournament == nullptr) {
        return std::unexpected("Tournament doesn't exist");
    }

    // 2) Validar nombre duplicado dentro del torneo
    try {
        auto existingGroups = groupRepository->FindByTournamentId(tournamentId);
        for (const auto& gPtr : existingGroups) {
            if (gPtr && gPtr->Name() == group.Name()) {
                return std::unexpected("Group name already exists");
            }
        }
    } catch (const std::exception&) {
        // si hay error de DB, lo podemos reportar generico
        return std::unexpected("Error when reading to DB");
    }

    // 3) Validar capacidad del grupo con el formato del torneo
    const auto maxPerGroup = tournament->Format().MaxTeamsPerGroup();
    if (group.Teams().size() > static_cast<std::size_t>(maxPerGroup)) {
        return std::unexpected("Group at max capacity");
    }

    // 4) Preparar grupo con tournamentId correcto
    domain::Group g = group;
    g.TournamentId() = tournament->Id();

    // 5) Validar equipos (si vienen pre-asignados al crear grupo)
    if (!g.Teams().empty()) {
        for (const auto& t : g.Teams()) {
            // 5.1) Verificar que el equipo exista
            auto team = teamRepository->ReadById(t.Id);
            if (team == nullptr) {
                // Los tests esperan EXACTAMENTE este mensaje para CreateGroup_TeamMissing
                return std::unexpected("Team doesn't exist");
            }

            // 5.2) Verificar que no este ya en algun grupo del mismo torneo
            auto existingGroup = groupRepository->FindByTournamentIdAndTeamId(tournamentId, t.Id);
            if (existingGroup != nullptr) {
                // Los tests usan HasSubstr con "Team E1 already exists in tournament T1"
                return std::unexpected(
                    "Team " + t.Id + " already exists in tournament " + std::string(tournamentId)
                );
            }
        }
    }

    // 6) Crear grupo
    auto id = groupRepository->Create(g);
    return id;
}

inline std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>
GroupDelegate::GetGroups(const std::string_view& tournamentId) {
    try {
        // Primero validar torneo (los tests esperan este ReadById en muchos casos)
        auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
        if (tournament == nullptr) {
            return std::unexpected("Tournament doesn't exist");
        }

        auto groups = groupRepository->FindByTournamentId(tournamentId);
        return groups;
    } catch (const std::exception&) {
        return std::unexpected("Error when reading to DB");
    }
}

inline std::expected<std::shared_ptr<domain::Group>, std::string>
GroupDelegate::GetGroup(const std::string_view& tournamentId, const std::string_view& groupId) {
    try {
        // Validar torneo primero
        auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
        if (tournament == nullptr) {
            return std::unexpected("Tournament doesn't exist");
        }

        auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
        if (group == nullptr) {
            return std::unexpected("Group doesn't exist");
        }

        return group;
    } catch (const std::exception&) {
        return std::unexpected("Error when reading to DB");
    }
}

inline std::expected<void, std::string>
GroupDelegate::UpdateGroup(const std::string_view& tournamentId, const domain::Group& group) {
    try {
        // Validaciones de campos primero (por los tests UpdateGroup_IdRequiredAndNameRequired)
        if (group.Id().empty()) {
            return std::unexpected("Group id required");
        }
        if (group.Name().empty()) {
            return std::unexpected("Group name required");
        }

        // Ahora si, validar torneo
        auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
        if (tournament == nullptr) {
            return std::unexpected("Tournament doesn't exist");
        }

        // Validar que el grupo exista
        auto existingGroup = groupRepository->FindByTournamentIdAndGroupId(tournamentId, group.Id());
        if (existingGroup == nullptr) {
            return std::unexpected("Group doesn't exist");
        }

        // Intentar actualizar
        auto updatedId = groupRepository->Update(group);
        if (updatedId.empty()) {
            // Tests UpdateGroup_UpdateReturnsEmpty_ReturnsError
            return std::unexpected("Group doesn't exist");
        }

        return {};
    } catch (const std::exception& e) {
        return std::unexpected(std::string("Error updating group: ") + e.what());
    }
}

inline std::expected<void, std::string>
GroupDelegate::RemoveGroup(const std::string_view& tournamentId, const std::string_view& groupId) {
    try {
        // Primero torneo (tests RemoveGroup_TournamentMissing)
        auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
        if (tournament == nullptr) {
            return std::unexpected("Tournament doesn't exist");
        }

        // Luego grupo
        auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
        if (group == nullptr) {
            return std::unexpected("Group doesn't exist");
        }

        groupRepository->Delete(std::string(groupId));
        return {};
    } catch (const std::exception& e) {
        return std::unexpected(std::string("Error deleting group: ") + e.what());
    }
}

inline std::expected<void, std::string>
GroupDelegate::UpdateTeams(const std::string_view& tournamentId, const std::string_view& groupId,
                           const std::vector<domain::Team>& teams) {
    // 1) Validar torneo
    auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
    if (tournament == nullptr) {
        return std::unexpected("Tournament doesn't exist");
    }

    // 2) Validar grupo
    const auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    if (group == nullptr) {
        return std::unexpected("Group doesn't exist");
    }

    // 3) Validar que los teams tengan Id
    for (const auto& t : teams) {
        if (t.Id.empty()) {
            return std::unexpected("Team id required");
        }
    }

    // 4) Validar capacidad ANTES de revisar duplicados / existencia
    const auto maxPerGroup = tournament->Format().MaxTeamsPerGroup();
    const auto currentSize  = group->Teams().size();
    if (currentSize + teams.size() > static_cast<std::size_t>(maxPerGroup)) {
        // Tests UpdateTeams_ExceedsCapacity_ReturnsError
        return std::unexpected("Group at max capacity");
    }

    // 5) Validar que no esten ya en algun grupo del torneo
    for (const auto& t : teams) {
        if (auto existingGroup = groupRepository->FindByTournamentIdAndTeamId(tournamentId, t.Id)) {
            // Tests UpdateTeams_DuplicateInTournament_ReturnsError usan HasSubstr con:
            // "Team E1 already exists in tournament T1"
            return std::unexpected(
                "Team " + t.Id + " already exists in tournament " + std::string(tournamentId)
            );
        }
    }

    // 6) Validar que cada team exista en DB
    for (const auto& t : teams) {
        const auto persistedTeam = teamRepository->ReadById(t.Id);
        if (persistedTeam == nullptr) {
            // Tests UpdateTeams_TeamMissing_ReturnsError
            return std::unexpected("Team " + t.Id + " doesn't exist");
        }
    }

    // 7) Ahora si, agregarlos
    for (const auto& t : teams) {
        const auto persistedTeam = teamRepository->ReadById(t.Id);
        groupRepository->UpdateGroupAddTeam(groupId, persistedTeam);
    }

    return {};
}

inline std::expected<void, std::string>
GroupDelegate::AddTeamToGroup(const std::string_view& tournamentId,
                              const std::string_view& groupId,
                              const std::string_view& teamId) {
    // 1) Validar torneo (los tests esperan ReadById("T*"))
    auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
    if (tournament == nullptr) {
        return std::unexpected("Tournament doesn't exist");
    }

    // 2) Leer team primero (para los tests AlreadyExists que esperan ReadById de team)
    auto team = teamRepository->ReadById(teamId);
    if (team == nullptr) {
        // Tests AddTeamToGroup_TeamNotExist_ReturnsError esperan EXACTO:
        // "Team doesn't exist"
        return std::unexpected("Team doesn't exist");
    }

    // 3) Validar grupo
    const auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    if (group == nullptr) {
        return std::unexpected("Group doesn't exist");
    }

    // 4) Capacidad
    const auto maxPerGroup = tournament->Format().MaxTeamsPerGroup();
    if (group->Teams().size() >= static_cast<std::size_t>(maxPerGroup)) {
        return std::unexpected("Group at max capacity");
    }

    // 5) Validar que no este ya en algun grupo del torneo
    if (auto existingGroup = groupRepository->FindByTournamentIdAndTeamId(tournamentId, std::string(teamId))) {
        // Tests AddTeamToGroup_AlreadyExists_ReturnsError usan HasSubstr con:
        // "Team E1 already exists in tournament T1"
        return std::unexpected(
            "Team " + std::string(teamId) + " already exists in tournament " + std::string(tournamentId)
        );
    }

    // 6) Agregar al grupo
    try {
        groupRepository->UpdateGroupAddTeam(groupId, team);
        return {};
    } catch (const std::exception& e) {
        return std::unexpected(std::string("Error adding team to group: ") + e.what());
    }
}

#endif /* SERVICE_GROUP_DELEGATE_HPP */
