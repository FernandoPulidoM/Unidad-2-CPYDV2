#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>
#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <string_view>

#include "domain/Team.hpp"
#include "delegate/ITeamDelegate.hpp"
#include "controller/TeamController.hpp"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SaveArg;

class TeamDelegateMock : public ITeamDelegate {
public:
    MOCK_METHOD(std::shared_ptr<domain::Team>, GetTeam, (const std::string_view id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, GetAllTeams, (), (override));
    MOCK_METHOD(std::string_view, SaveTeam, (const domain::Team&), (override));
};

class TeamControllerTest : public ::testing::Test {
protected:
    std::shared_ptr<TeamDelegateMock> teamDelegateMock;
    std::shared_ptr<TeamController> teamController;

    void SetUp() override {
        teamDelegateMock = std::make_shared<TeamDelegateMock>();
        // Constructor directo con el mock, no copies temporales
        teamController = std::make_shared<TeamController>(teamDelegateMock);
    }

    void TearDown() override {}
};

TEST_F(TeamControllerTest, GetTeamById_ErrorFormat) {
    crow::response badRequest = teamController->getTeam("");
    EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);

    badRequest = teamController->getTeam("mfasd#*");
    EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);
}

TEST_F(TeamControllerTest, GetTeamById) {
    auto expectedTeam = std::make_shared<domain::Team>(domain::Team{"my-id", "Team Name"});

    // Evita problemas con string_view en matchers; valida por lambda
    EXPECT_CALL(*teamDelegateMock, GetTeam(::testing::Truly([](std::string_view v) {
                    return v == "my-id";
                })))
        .WillOnce(Return(expectedTeam));

    crow::response response = teamController->getTeam("my-id");
    EXPECT_EQ(crow::OK, response.code);

    auto jsonResponse = crow::json::load(response.body);
    ASSERT_TRUE(jsonResponse);
    // crow::json::rvalue: usa .s() para string
    EXPECT_EQ(std::string("my-id"), std::string(jsonResponse["id"].s()));
    EXPECT_EQ(std::string("Team Name"), std::string(jsonResponse["name"].s()));
}

TEST_F(TeamControllerTest, GetTeamNotFound) {
    EXPECT_CALL(*teamDelegateMock, GetTeam(::testing::Truly([](std::string_view v) {
                    return v == "my-id";
                })))
        .WillOnce(Return(nullptr));

    crow::response response = teamController->getTeam("my-id");
    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

TEST_F(TeamControllerTest, SaveTeamTest) {
    domain::Team capturedTeam;

    EXPECT_CALL(*teamDelegateMock, SaveTeam(_))
        .WillOnce(DoAll(
            SaveArg<0>(&capturedTeam),
            // string literal con storage estatico es seguro para string_view
            Return(std::string_view("new-id"))
        ));

    nlohmann::json teamRequestBody = {{"id", "new-id"}, {"name", "new team"}};
    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->SaveTeam(teamRequest);

    // Verifica expectativas del mock correcto (no del shared_ptr)
    testing::Mock::VerifyAndClearExpectations(teamDelegateMock.get());

    EXPECT_EQ(crow::CREATED, response.code);
    EXPECT_EQ(teamRequestBody.at("id").get<std::string>(), capturedTeam.Id);
    EXPECT_EQ(teamRequestBody.at("name").get<std::string>(), capturedTeam.Name);

    // Opcional: si tu SaveTeam agrega header Location
    // EXPECT_EQ(std::string("new-id"), response.get_header_value("location"));
}
