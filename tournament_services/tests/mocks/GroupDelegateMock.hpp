#pragma once
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <expected>

#include "../../include/delegate/IGroupDelegate.hpp"
#include "../../../tournament_common/include/domain/Group.hpp"
#include "../../../tournament_common/include/domain/Team.hpp"

/**
 * Mock de IGroupDelegate para pruebas unitarias del GroupController
 * Respeta exactamente las firmas definidas en IGroupDelegate.hpp
 *
 * NOTA: Los tipos de retorno con comas (std::expected) deben estar entre paréntesis extra
 */
class GroupDelegateMock : public IGroupDelegate {
public:
    // CreateGroup: retorna el ID del grupo creado o error
    MOCK_METHOD((std::expected<std::string, std::string>),
                CreateGroup,
                (const std::string_view& tournamentId, const domain::Group& group),
                (override));

    // GetGroups: retorna lista de grupos de un torneo o error
    MOCK_METHOD((std::expected<std::vector<std::shared_ptr<domain::Group>>, std::string>),
                GetGroups,
                (const std::string_view& tournamentId),
                (override));

    // GetGroup: retorna un grupo específico o error
    MOCK_METHOD((std::expected<std::shared_ptr<domain::Group>, std::string>),
                GetGroup,
                (const std::string_view& tournamentId, const std::string_view& groupId),
                (override));

    // UpdateGroup: actualiza un grupo, retorna void o error
    MOCK_METHOD((std::expected<void, std::string>),
                UpdateGroup,
                (const std::string_view& tournamentId, const domain::Group& group),
                (override));

    // RemoveGroup: elimina un grupo, retorna void o error
    MOCK_METHOD((std::expected<void, std::string>),
                RemoveGroup,
                (const std::string_view& tournamentId, const std::string_view& groupId),
                (override));

    // UpdateTeams: actualiza equipos de un grupo, retorna void o error
    MOCK_METHOD((std::expected<void, std::string>),
                UpdateTeams,
                (const std::string_view& tournamentId,
                 const std::string_view& groupId,
                 const std::vector<domain::Team>& teams),
                (override));
};