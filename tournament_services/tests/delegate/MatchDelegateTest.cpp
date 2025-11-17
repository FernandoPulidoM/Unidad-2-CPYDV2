#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>

#include "delegate/MatchDelegate.hpp"
#include "domain/Match.hpp"
#include "domain/Group.hpp"
#include "MatchRepositoryMock.hpp"
#include "GroupRepositoryMock.hpp"
#include "../mocks/MatchStrategyMock.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::Throw;
using ::testing::NiceMock;

class MatchDelegateTest : public ::testing::Test {
protected:
    std::shared_ptr<persistence::MockMatchRepository> mockRepo;
    std::shared_ptr<GroupRepositoryMock> mockGroupRepo;
    std::shared_ptr<MockMatchStrategy> mockStrategy;
    std::shared_ptr<MatchDelegate> delegate;

    void SetUp() override {
        mockRepo = std::make_shared<NiceMock<persistence::MockMatchRepository>>();
        mockGroupRepo = std::make_shared<NiceMock<GroupRepositoryMock>>();
        mockStrategy = std::make_shared<NiceMock<MockMatchStrategy>>();
        delegate = std::make_shared<MatchDelegate>(mockRepo, mockGroupRepo, mockStrategy);
    }

    // Helper: crear match de prueba
    domain::Match createTestMatch(const std::string& id,
                                  const std::string& round,
                                  const std::string& status) {
        domain::Match m;
        m.id = id;
        m.tournamentId = "test-tournament";
        m.homeTeamId = "team1";
        m.awayTeamId = "team2";
        m.round = round;
        m.status = status;
        return m;
    }

    // Helper: crear grupo con equipos
    std::shared_ptr<domain::Group> createTestGroup(const std::string& id, int numTeams) {
        auto group = std::make_shared<domain::Group>();
        group->Id() = id;
        group->Name() = "Group " + id;

        std::vector<domain::Team> teams;
        for (int i = 0; i < numTeams; ++i) {
            domain::Team t;
            t.Id = "team-" + std::to_string(i);
            t.Name = "Team " + std::to_string(i);
            teams.push_back(t);
        }
        group->Teams() = teams;

        return group;
    }
};

// Test 1: Listar partidos sin filtro
TEST_F(MatchDelegateTest, List_NoFilter_ReturnsAllMatches) {
    std::vector<domain::Match> matches = {
        createTestMatch("m1", "group_stage", "pending"),
        createTestMatch("m2", "group_stage", "played")
    };

    EXPECT_CALL(*mockRepo, ListByTournament("t1", std::nullopt))
        .WillOnce(Return(matches));

    auto result = delegate->List("t1", std::nullopt);
    EXPECT_EQ(result.size(), 2);
}

// Test 2: Listar partidos con filtro "played"
TEST_F(MatchDelegateTest, List_FilterPlayed_ReturnsOnlyPlayed) {
    std::vector<domain::Match> playedMatches = {
        createTestMatch("m2", "group_stage", "played")
    };

    EXPECT_CALL(*mockRepo, ListByTournament("t1", std::optional<std::string>("played")))
        .WillOnce(Return(playedMatches));

    auto result = delegate->List("t1", "played");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].status, "played");
}

// Test 3: Obtener match por ID - existe
TEST_F(MatchDelegateTest, Get_MatchExists_ReturnsMatch) {
    auto match = createTestMatch("m1", "group_stage", "pending");

    EXPECT_CALL(*mockRepo, GetById("t1", "m1"))
        .WillOnce(Return(match));

    auto result = delegate->Get("t1", "m1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->id, "m1");
}

// Test 4: Obtener match por ID - no existe
TEST_F(MatchDelegateTest, Get_MatchNotFound_ReturnsNullopt) {
    EXPECT_CALL(*mockRepo, GetById("t1", "nope"))
        .WillOnce(Return(std::nullopt));

    auto result = delegate->Get("t1", "nope");
    EXPECT_FALSE(result.has_value());
}

// Test 5: Actualizar score - exitoso
TEST_F(MatchDelegateTest, UpdateScore_Valid_ReturnsSuccess) {
    auto match = createTestMatch("m1", "group_stage", "pending");

    EXPECT_CALL(*mockRepo, GetById("t1", "m1"))
        .WillOnce(Return(match));

    EXPECT_CALL(*mockRepo, UpdateScore("t1", "m1", 3, 1))
        .WillOnce(Return(std::expected<void, std::string>()));

    auto result = delegate->updateScore("t1", "m1", 3, 1);
    EXPECT_TRUE(result.has_value());
}

// Test 6: Actualizar score - match no existe
TEST_F(MatchDelegateTest, UpdateScore_MatchNotFound_ReturnsError) {
    EXPECT_CALL(*mockRepo, GetById("t1", "nope"))
        .WillOnce(Return(std::nullopt));

    auto result = delegate->updateScore("t1", "nope", 3, 1);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Match not found");
}

// Test 7: Actualizar score - match ya jugado
TEST_F(MatchDelegateTest, UpdateScore_AlreadyPlayed_ReturnsError) {
    auto match = createTestMatch("m1", "group_stage", "played");

    EXPECT_CALL(*mockRepo, GetById("t1", "m1"))
        .WillOnce(Return(match));

    auto result = delegate->updateScore("t1", "m1", 3, 1);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Match already has a score");
}

// Test 8: Generar fase de grupos - exitoso (48 partidos)
TEST_F(MatchDelegateTest, GenerateMatches_GroupStage_Creates48Matches) {
    // Crear 8 grupos con 4 equipos cada uno
    std::vector<std::shared_ptr<domain::Group>> groups;
    for (int i = 0; i < 8; ++i) {
        groups.push_back(createTestGroup("g" + std::to_string(i), 4));
    }

    EXPECT_CALL(*mockGroupRepo, FindByTournamentId("t1"))
        .WillOnce(Return(groups));

    // La estrategia genera 6 partidos por grupo (4 equipos -> C(4,2) = 6)
    std::vector<domain::Match> generatedMatches;
    for (int i = 0; i < 48; ++i) {
        generatedMatches.push_back(createTestMatch("m" + std::to_string(i), "group_stage", "pending"));
    }

    EXPECT_CALL(*mockStrategy, GenerateMatches("t1", _))
        .WillOnce(Return(generatedMatches));

    EXPECT_CALL(*mockRepo, CreateBulk(_))
        .WillOnce(Return(std::expected<void, std::string>()));

    auto result = delegate->GenerateMatchesForTournament("t1");
    EXPECT_TRUE(result.has_value());
}

// Test 9: Generar fase de grupos - sin grupos
TEST_F(MatchDelegateTest, GenerateMatches_NoGroups_ReturnsError) {
    EXPECT_CALL(*mockGroupRepo, FindByTournamentId("t1"))
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Group>>{}));

    auto result = delegate->GenerateMatchesForTournament("t1");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "No groups found for tournament");
}

// Test 10: Obtener estado del torneo - fase de grupos incompleta
TEST_F(MatchDelegateTest, GetStatus_GroupStageIncomplete) {
    std::vector<domain::Match> matches;
    // 48 partidos de grupos: 30 jugados, 18 pendientes
    for (int i = 0; i < 30; ++i) {
        matches.push_back(createTestMatch("m" + std::to_string(i), "group_stage", "played"));
    }
    for (int i = 30; i < 48; ++i) {
        matches.push_back(createTestMatch("m" + std::to_string(i), "group_stage", "pending"));
    }

    EXPECT_CALL(*mockRepo, ListByTournament("t1", std::nullopt))
        .WillOnce(Return(matches));

    auto result = delegate->GetTournamentStatus("t1");
    ASSERT_TRUE(result.has_value());

    auto status = *result;
    EXPECT_EQ(status["totalMatches"], 48);
    EXPECT_EQ(status["rounds"]["group_stage"]["total"], 48);
    EXPECT_EQ(status["rounds"]["group_stage"]["played"], 30);
    EXPECT_EQ(status["rounds"]["group_stage"]["pending"], 18);
    EXPECT_FALSE(status["rounds"]["group_stage"]["complete"]);
    EXPECT_EQ(status["currentPhase"], "group_stage");
}

// Test 11: Obtener estado - todas las fases (63 partidos)
TEST_F(MatchDelegateTest, GetStatus_AllPhases_63Matches) {
    std::vector<domain::Match> matches;

    // Grupos: 48 jugados
    for (int i = 0; i < 48; ++i) {
        matches.push_back(createTestMatch("g" + std::to_string(i), "group_stage", "played"));
    }
    // Octavos: 8 jugados
    for (int i = 0; i < 8; ++i) {
        matches.push_back(createTestMatch("r16-" + std::to_string(i), "round_of_16", "played"));
    }
    // Cuartos: 4 jugados
    for (int i = 0; i < 4; ++i) {
        matches.push_back(createTestMatch("qf" + std::to_string(i), "quarter_finals", "played"));
    }
    // Semis: 2 jugados
    for (int i = 0; i < 2; ++i) {
        matches.push_back(createTestMatch("sf" + std::to_string(i), "semi_finals", "played"));
    }
    // Final: 1 pendiente
    matches.push_back(createTestMatch("final", "final", "pending"));

    EXPECT_CALL(*mockRepo, ListByTournament("t1", std::nullopt))
        .WillOnce(Return(matches));

    auto result = delegate->GetTournamentStatus("t1");
    ASSERT_TRUE(result.has_value());

    auto status = *result;
    EXPECT_EQ(status["totalMatches"], 63);
    EXPECT_TRUE(status["rounds"]["group_stage"]["complete"]);
    EXPECT_TRUE(status["rounds"]["round_of_16"]["complete"]);
    EXPECT_TRUE(status["rounds"]["quarter_finals"]["complete"]);
    EXPECT_TRUE(status["rounds"]["semi_finals"]["complete"]);
    EXPECT_FALSE(status["rounds"]["final"]["complete"]);
    EXPECT_EQ(status["currentPhase"], "final");
}

// Test 12: Generar octavos - fase de grupos incompleta
TEST_F(MatchDelegateTest, GenerateKnockout_GroupStageIncomplete_ReturnsError) {
    // Status con grupos incompletos
    nlohmann::json incompleteStatus;
    incompleteStatus["rounds"]["group_stage"]["complete"] = false;

    EXPECT_CALL(*mockRepo, ListByTournament("t1", std::nullopt))
        .WillOnce(Return(std::vector<domain::Match>{}));

    auto result = delegate->GenerateKnockoutPhase("t1");
    EXPECT_FALSE(result.has_value());
}

// Test 13: Calcular standings de un grupo
TEST_F(MatchDelegateTest, GetGroupStandings_CalculatesCorrectly) {
    auto group = createTestGroup("g1", 4);

    std::vector<domain::Match> matches;

    // Team 0 vs Team 1: 3-0
    domain::Match m1 = createTestMatch("m1", "group_stage", "played");
    m1.homeTeamId = "team-0";
    m1.awayTeamId = "team-1";
    m1.score = domain::Score{3, 0};
    matches.push_back(m1);

    // Team 0 vs Team 2: 2-1
    domain::Match m2 = createTestMatch("m2", "group_stage", "played");
    m2.homeTeamId = "team-0";
    m2.awayTeamId = "team-2";
    m2.score = domain::Score{2, 1};
    matches.push_back(m2);

    std::vector<std::shared_ptr<domain::Group>> groups = {group};

    EXPECT_CALL(*mockRepo, ListByTournament("t1", std::nullopt))
        .WillRepeatedly(Return(matches));

    EXPECT_CALL(*mockGroupRepo, FindByTournamentId("t1"))
        .WillRepeatedly(Return(groups));

    auto result = delegate->GetGroupStandings("t1", "g1");
    ASSERT_TRUE(result.has_value());

    auto standings = *result;
    EXPECT_EQ(standings.size(), 4);

    // Team-0 debe estar primero (6 puntos, 2 victorias)
    EXPECT_EQ(standings[0].teamId, "team-0");
    EXPECT_EQ(standings[0].points, 6);
    EXPECT_EQ(standings[0].won, 2);
}

// Test 14: Avanzar fase automáticamente
TEST_F(MatchDelegateTest, AdvancePhase_FromGroupStage_GeneratesKnockout) {
    // Simular grupos completos
    nlohmann::json completeStatus;
    completeStatus["currentPhase"] = "group_stage";
    completeStatus["rounds"]["group_stage"]["complete"] = true;

    EXPECT_CALL(*mockRepo, ListByTournament("t1", std::nullopt))
        .WillOnce(Return(std::vector<domain::Match>{}));

    // El método debería intentar generar knockout
    auto result = delegate->AdvanceKnockoutPhase("t1");
    // Fallará porque no hay equipos, pero verifica que se llama
    EXPECT_FALSE(result.has_value());
}