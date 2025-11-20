#pragma once

#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "../../../tournament_common/include/persistence/repository/TeamRepository.hpp"
#include "../../../tournament_common/include/persistence/configuration/IDbConnectionProvider.hpp"
#include "../../../tournament_common/include/domain/Team.hpp"

class TeamRepositoryMock : public persistence::TeamRepository {
public:
    TeamRepositoryMock()
        : persistence::TeamRepository(
              std::shared_ptr<IDbConnectionProvider>{}) {}  // << aqui sin persistence::

    MOCK_METHOD(std::string_view, Create,
                (const domain::Team &team), (override));

    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, ReadAll,
                (), (override));

    MOCK_METHOD(std::shared_ptr<domain::Team>, ReadById,
                (std::string_view id), (override));

    MOCK_METHOD(std::string_view, Update,
                (const domain::Team &team), (override));

    MOCK_METHOD(void, Delete,
                (std::string_view id), (override));
};

using MockTeamRepository = TeamRepositoryMock;
