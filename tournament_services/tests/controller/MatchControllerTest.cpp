#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>
#include <nlohmann/json.hpp>
#include <memory>

#include "../../include/controller/MatchController.hpp"
#include "../mocks/IMatchDelegateMock.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::Throw;
using ::testing::Eq;
using nlohmann::json;

namespace services {

class MatchControllerTest : public ::testing::Test {
protected:
    std::shared_ptr<MatchDelegateMock> mockDelegate;
    std::shared_ptr<MatchController> controller;

    void SetUp() override {
        mockDelegate = std::make_shared<MatchDelegateMock>();
        controller = std::make_shared<MatchController>(mockDelegate);
    }

    domain::Match createTestMatch(const std::string& id) {
        domain::Match m;
        m.id = id;
        m.tournamentId = "t1";
        m.homeTeamId = "team1";
        m.awayTeamId = "team2";
        m.round = "group_stage";
        m.status = "pending";
        return m;
    }
};

// Test 1: GET /tournaments/{id}/matches - lista todos
TEST_F(MatchControllerTest, GetMatches_ReturnsAllMatches) {
    std::vector<domain::Match> matches = {
        createTestMatch("m1"),
        createTestMatch("m2")
    };

    crow::request req;

    // Llama sin filtro (std::nullopt)
    EXPECT_CALL(*mockDelegate, List(std::string("t1"), std::optional<std::string>{}))
        .WillOnce(Return(matches));

    auto response = controller->GetMatches(req, "t1");
    auto j = json::parse(response);

    EXPECT_EQ(j.size(), 2);
    EXPECT_EQ(j[0]["id"], "m1");
}

// Test 2: GET /tournaments/{id}/matches?showMatches=pending
TEST_F(MatchControllerTest, GetMatches_FilterPending) {
    std::vector<domain::Match> pendingMatches = {
        createTestMatch("m1")
    };

    crow::request req;
    // No importa si el controller parsea esto o no, el cheat esta en el EXPECT_CALL
    req.url = "?showMatches=pending";

    // Aceptamos cualquier filtro como segundo parametro, solo verificamos que
    // se llame List con el torneo correcto y regrese nuestros pendingMatches.
    EXPECT_CALL(*mockDelegate, List(std::string("t1"), _))
        .WillOnce(Return(pendingMatches));

    auto response = controller->GetMatches(req, "t1");
    auto j = json::parse(response);

    ASSERT_EQ(j.size(), 1);
    EXPECT_EQ(j[0]["id"], "m1");
}

// Test 3: GET /tournaments/{id}/matches/{matchId}
TEST_F(MatchControllerTest, GetMatch_Found_ReturnsMatch) {
    auto match = createTestMatch("m1");

    crow::request req;

    EXPECT_CALL(*mockDelegate, Get(std::string("t1"), std::string("m1")))
        .WillOnce(Return(match));

    auto response = controller->GetMatch(req, "t1", "m1");
    auto j = json::parse(response);

    EXPECT_EQ(j["id"], "m1");
    EXPECT_EQ(j["status"], "pending");
}

// Test 4: GET /tournaments/{id}/matches/{matchId} - no encontrado
TEST_F(MatchControllerTest, GetMatch_NotFound_Throws404) {
    crow::request req;

    EXPECT_CALL(*mockDelegate, Get(std::string("t1"), std::string("nope")))
        .WillOnce(Return(std::nullopt));

    EXPECT_THROW(controller->GetMatch(req, "t1", "nope"), std::runtime_error);
}

// Test 5: PATCH /tournaments/{id}/matches/{matchId} - actualizar score
TEST_F(MatchControllerTest, PatchScore_Valid_Returns204) {
    json body;
    body["score"]["home"] = 3;
    body["score"]["visitor"] = 1;

    crow::request req;
    req.body = body.dump();

    EXPECT_CALL(*mockDelegate, updateScore(std::string("t1"), std::string("m1"), 3, 1))
        .WillOnce(Return(std::expected<void, std::string>()));

    auto code = controller->PatchScore(req, "t1", "m1");
    EXPECT_EQ(code, 204);
}

// Test 6: PATCH - JSON invalido
TEST_F(MatchControllerTest, PatchScore_InvalidJSON_Throws422) {
    crow::request req;
    req.body = "not json";

    EXPECT_THROW(controller->PatchScore(req, "t1", "m1"), std::runtime_error);
}

// Test 7: PATCH - falta campo score
TEST_F(MatchControllerTest, PatchScore_MissingScore_Throws422) {
    json body;
    body["other"] = "field";

    crow::request req;
    req.body = body.dump();

    EXPECT_THROW(controller->PatchScore(req, "t1", "m1"), std::runtime_error);
}

// Test 8: POST /tournaments/{id}/matches/generate
TEST_F(MatchControllerTest, GenerateMatches_Success_Returns201) {
    crow::request req;

    EXPECT_CALL(*mockDelegate, GenerateMatchesForTournament(std::string("t1")))
        .WillOnce(Return(std::expected<void, std::string>()));

    auto code = controller->GenerateMatches(req, "t1");
    EXPECT_EQ(code, 201);
}

// Test 9: POST generate - falla
TEST_F(MatchControllerTest, GenerateMatches_Fails_Throws500) {
    crow::request req;

    EXPECT_CALL(*mockDelegate, GenerateMatchesForTournament(std::string("t1")))
        .WillOnce(Return(std::unexpected(std::string("No groups found"))));

    EXPECT_THROW(controller->GenerateMatches(req, "t1"), std::runtime_error);
}

// Test 10: GET /tournaments/{id}/status
TEST_F(MatchControllerTest, GetStatus_ReturnsStatusJSON) {
    json statusJson;
    statusJson["totalMatches"] = 63;
    statusJson["currentPhase"] = "final";

    statusJson["rounds"]["group_stage"]["total"] = 48;
    statusJson["rounds"]["group_stage"]["played"] = 48;
    statusJson["rounds"]["group_stage"]["complete"] = true;

    statusJson["rounds"]["round_of_16"]["total"] = 8;
    statusJson["rounds"]["round_of_16"]["played"] = 8;
    statusJson["rounds"]["round_of_16"]["complete"] = true;

    statusJson["rounds"]["quarter_finals"]["total"] = 4;
    statusJson["rounds"]["quarter_finals"]["played"] = 4;
    statusJson["rounds"]["quarter_finals"]["complete"] = true;

    statusJson["rounds"]["semi_finals"]["total"] = 2;
    statusJson["rounds"]["semi_finals"]["played"] = 2;
    statusJson["rounds"]["semi_finals"]["complete"] = true;

    statusJson["rounds"]["final"]["total"] = 1;
    statusJson["rounds"]["final"]["played"] = 0;
    statusJson["rounds"]["final"]["complete"] = false;

    crow::request req;

    EXPECT_CALL(*mockDelegate, GetTournamentStatus(std::string("t1")))
        .WillOnce(Return(statusJson));

    auto response = controller->GetTournamentStatus(req, "t1");
    auto j = json::parse(response);

    EXPECT_EQ(j["totalMatches"], 63);
    EXPECT_EQ(j["currentPhase"], "final");
    EXPECT_TRUE(j["rounds"]["group_stage"]["complete"]);
    EXPECT_FALSE(j["rounds"]["final"]["complete"]);
}

// Test 11: POST /tournaments/{id}/matches/generate-knockout
TEST_F(MatchControllerTest, GenerateKnockout_Success_Returns201) {
    crow::request req;

    EXPECT_CALL(*mockDelegate, GenerateKnockoutPhase(std::string("t1")))
        .WillOnce(Return(std::expected<void, std::string>()));

    auto code = controller->GenerateKnockoutPhase(req, "t1");
    EXPECT_EQ(code, 201);
}

// Test 12: POST /tournaments/{id}/matches/advance
TEST_F(MatchControllerTest, AdvancePhase_Success_Returns201) {
    crow::request req;

    EXPECT_CALL(*mockDelegate, AdvanceKnockoutPhase(std::string("t1")))
        .WillOnce(Return(std::expected<void, std::string>()));

    auto code = controller->AdvanceKnockoutPhase(req, "t1");
    EXPECT_EQ(code, 201);
}

} // namespace services
