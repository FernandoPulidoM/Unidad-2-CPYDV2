#ifndef SERVICE_GROUP_DELEGATE_HPP
#define SERVICE_GROUP_DELEGATE_HPP

#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <expected>
#include <format>
#include <exception>

// Interfaz del delegate
#include "IGroupDelegate.hpp"

// Dominios
#include "../../../tournament_common/include/domain/Group.hpp"
#include "../../../tournament_common/include/domain/Team.hpp"
#include "../../../tournament_common/include/domain/Tournament.hpp"

// Repos (interfaces y defs)
#include "persistence/repository/IRepository.hpp"
#include "persistence/repository/GroupRepository.hpp" // define IGroupRepository/GroupRepository

class GroupDelegate : public IGroupDelegate {
    // OJO: IRepository e IGroupRepository estan en el global namespace (no uses persistence::)
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
    UpdateTeams(const std::string_view& tournamentId, const std::string_view& groupId, const std::vector<domain::Team>& teams) override;

    // NUEVO: Implementar AddTeamToGroup
    std::expected<void, std::string>
    AddTeamToGroup(const std::string_view& tournamentId, const std::string_view& groupId,
                   const std::string_view& teamId) override;
};

// ===== Implementacion inline =====

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
    auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
    if (tournament == nullptr) {
        return std::unexpected("Tournament doesn't exist");
    }

    domain::Group g = group;
    g.TournamentId() = tournament->Id();

    if (!g.Teams().empty()) {
        for (auto& t : g.Teams()) {
            auto team = teamRepository->ReadById(t.Id);
            if (team == nullptr) {
                return std::unexpected("Team doesn't exist");
            }
        }
    }

    auto id = groupRepository->Create(g);
    return id;
}

inline std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>
GroupDelegate::GetGroups(const std::string_view& tournamentId) {
    try {
        return groupRepository->FindByTournamentId(tournamentId);
    } catch (const std::exception&) {
        return std::unexpected("Error when reading to DB");
    }
}

inline std::expected<std::shared_ptr<domain::Group>, std::string>
GroupDelegate::GetGroup(const std::string_view& tournamentId, const std::string_view& groupId) {
    try {
        return groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    } catch (const std::exception&) {
        return std::unexpected("Error when reading to DB");
    }
}

inline std::expected<void, std::string>
GroupDelegate::UpdateGroup(const std::string_view& tournamentId, const domain::Group& group) {
    try {
        auto tournament = tournamentRepository->ReadById(std::string(tournamentId));
        if (tournament == nullptr) {
            return std::unexpected("Tournament doesn't exist");
        }

        auto existingGroup = groupRepository->FindByTournamentIdAndGroupId(tournamentId, group.Id());
        if (existingGroup == nullptr) {
            return std::unexpected("Group doesn't exist");
        }

        groupRepository->Update(group);
        return {};
    } catch (const std::exception& e) {
        return std::unexpected(std::string("Error updating group: ") + e.what());
    }
}

inline std::expected<void, std::string>
GroupDelegate::RemoveGroup(const std::string_view& tournamentId, const std::string_view& groupId) {
    try {
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
GroupDelegate::UpdateTeams(const std::string_view& tournamentId, const std::string_view& groupId, const std::vector<domain::Team>& teams) {
    for (const auto& t : teams) {
        if (t.Id.empty()) return std::unexpected("Team id requerido");
    }

    const auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    if (group == nullptr) {
        return std::unexpected("Group doesn't exist");
    }

    if (group->Teams().size() + teams.size() >= 16) {
        return std::unexpected("Group at max capacity");
    }

    for (const auto& team : teams) {
        if (const auto groupTeams = groupRepository->FindByTournamentIdAndTeamId(tournamentId, team.Id)) {
            return std::unexpected("Team {} already exist" + team.Id);
        }
    }

    for (const auto& team : teams) {
        const auto persistedTeam = teamRepository->ReadById(team.Id);
        if (persistedTeam == nullptr) {
            return std::unexpected("Team {} doesn't exist" + team.Id);
        }
        groupRepository->UpdateGroupAddTeam(groupId, persistedTeam);
    }

    return {};
}


// NUEVO: Implementación de AddTeamToGroup
inline std::expected<void, std::string>
GroupDelegate::AddTeamToGroup(const std::string_view& tournamentId, const std::string_view& groupId,
                              const std::string_view& teamId) {
    // Verificar que el grupo existe
    const auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    if (group == nullptr) {
        return std::unexpected("Group doesn't exist");
    }

    // Verificar capacidad
    if (group->Teams().size() >= 16) {
        return std::unexpected("Group at max capacity");
    }

    // Verificar que el equipo no esté ya en un grupo
    if (const auto existingGroup = groupRepository->FindByTournamentIdAndTeamId(tournamentId, std::string(teamId))) {
        return std::unexpected("Team {} already in a group" +  std::string(teamId));
    }

    // Verificar que el equipo existe
    const auto team = teamRepository->ReadById(teamId);
    if (team == nullptr) {
        return std::unexpected("Team {} doesn't exist" +  std::string(teamId));
    }

    // Agregar equipo al grupo
    try {
        groupRepository->UpdateGroupAddTeam(groupId, team);
        return {};
    } catch (const std::exception& e) {
        return std::unexpected(std::string("Error adding team to group: ") + e.what());
    }
}


#endif /* SERVICE_GROUP_DELEGATE_HPP */
