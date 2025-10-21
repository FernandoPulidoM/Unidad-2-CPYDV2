#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>
#include "controller/TournamentController.hpp"
#include "delegate/ITournamentDelegate.hpp"
#include "domain/Tournament.hpp"

using ::testing::Return;
using ::testing::_;

// Mock del ITournamentDelegate
class MockTournamentDelegate : public ITournamentDelegate {
public:
    MOCK_METHOD(std::string, CreateTournament, (std::shared_ptr<domain::Tournament>), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
    MOCK_METHOD(void, UpdateTournament, (const std::string&, std::shared_ptr<domain::Tournament>), (override));
    MOCK_METHOD(void, DeleteTournament, (const std::string&), (override));
};

class TournamentControllerTest : public ::testing::Test {
protected:
    std::shared_ptr<MockTournamentDelegate> mockDelegate;
    std::shared_ptr<TournamentController> controller;

    void SetUp() override {
        mockDelegate = std::make_shared<MockTournamentDelegate>();
        controller = std::make_shared<TournamentController>(mockDelegate);
    }
};

TEST_F(TournamentControllerTest, CreateTournament_ValidRequest_ReturnsCreated) {
    // Arrange
    nlohmann::json requestBody = {
        {"name", "Test Tournament"},
        {"format", {
            {"type", "ROUND_ROBIN"},
            {"numberOfGroups", 2},
            {"maxTeamsPerGroup", 8}
        }}
    };

    crow::request req;
    req.body = requestBody.dump();

    std::string expectedId = "test-tournament-id-123";
    EXPECT_CALL(*mockDelegate, CreateTournament(_))
        .WillOnce(Return(expectedId));

    // Act
    crow::response response = controller->CreateTournament(req);

    // Assert
    EXPECT_EQ(response.code, crow::CREATED);
    EXPECT_EQ(response.get_header_value("location"), expectedId);
}

TEST_F(TournamentControllerTest, CreateTournament_InvalidJSON_ReturnsBadRequest) {
    // Arrange
    crow::request req;
    req.body = "invalid json {";

    // Act & Assert
    EXPECT_THROW(controller->CreateTournament(req), std::exception);
}

TEST_F(TournamentControllerTest, ReadAll_ReturnsAllTournaments) {
    // Arrange
    auto tournament1 = std::make_shared<domain::Tournament>("Tournament 1");
    tournament1->Id() = "id-1";

    auto tournament2 = std::make_shared<domain::Tournament>("Tournament 2");
    tournament2->Id() = "id-2";

    std::vector<std::shared_ptr<domain::Tournament>> tournaments = {tournament1, tournament2};

    EXPECT_CALL(*mockDelegate, ReadAll())
        .WillOnce(Return(tournaments));

    // Act
    crow::response response = controller->ReadAll();

    // Assert
    EXPECT_EQ(response.code, crow::OK);
    EXPECT_EQ(response.get_header_value("content-type"), "application/json");

    nlohmann::json body = nlohmann::json::parse(response.body);
    EXPECT_EQ(body.size(), 2);
}

TEST_F(TournamentControllerTest, UpdateTournament_ValidRequest_ReturnsOK) {
    // Arrange
    std::string tournamentId = "test-id-123";
    nlohmann::json requestBody = {
        {"name", "Updated Tournament"},
        {"format", {
            {"type", "ROUND_ROBIN"},
            {"numberOfGroups", 1},
            {"maxTeamsPerGroup", 16}
        }}
    };

    crow::request req;
    req.body = requestBody.dump();

    EXPECT_CALL(*mockDelegate, UpdateTournament(tournamentId, _))
        .Times(1);

    // Act
    crow::response response = controller->UpdateTournament(req, tournamentId);

    // Assert
    EXPECT_EQ(response.code, crow::OK);
}

TEST_F(TournamentControllerTest, UpdateTournament_InvalidJSON_ReturnsBadRequest) {
    // Arrange
    crow::request req;
    req.body = "invalid json";
    std::string tournamentId = "test-id";

    // Act
    crow::response response = controller->UpdateTournament(req, tournamentId);

    // Assert
    EXPECT_EQ(response.code, crow::BAD_REQUEST);
}

TEST_F(TournamentControllerTest, UpdateTournament_NotFound_ReturnsNotFound) {
    // Arrange
    std::string tournamentId = "non-existent-id";
    nlohmann::json requestBody = {
        {"name", "Test"}
    };

    crow::request req;
    req.body = requestBody.dump();

    EXPECT_CALL(*mockDelegate, UpdateTournament(tournamentId, _))
        .WillOnce(testing::Throw(std::runtime_error("Tournament not found")));

    // Act
    crow::response response = controller->UpdateTournament(req, tournamentId);

    // Assert
    EXPECT_EQ(response.code, crow::NOT_FOUND);
}

TEST_F(TournamentControllerTest, DeleteTournament_ValidId_ReturnsNoContent) {
    // Arrange
    std::string tournamentId = "test-id-123";

    EXPECT_CALL(*mockDelegate, DeleteTournament(tournamentId))
        .Times(1);

    // Act
    crow::response response = controller->DeleteTournament(tournamentId);

    // Assert
    EXPECT_EQ(response.code, crow::NO_CONTENT);
}

TEST_F(TournamentControllerTest, DeleteTournament_NotFound_ReturnsNotFound) {
    // Arrange
    std::string tournamentId = "non-existent-id";

    EXPECT_CALL(*mockDelegate, DeleteTournament(tournamentId))
        .WillOnce(testing::Throw(std::runtime_error("Tournament not found")));

    // Act
    crow::response response = controller->DeleteTournament(tournamentId);

    // Assert
    EXPECT_EQ(response.code, crow::NOT_FOUND);
}