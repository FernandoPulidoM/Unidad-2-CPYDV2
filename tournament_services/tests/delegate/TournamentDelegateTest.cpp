#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "delegate/TournamentDelegate.hpp"
#include "persistence/repository/IRepository.hpp"
#include "cms/QueueMessageProducer.hpp"
#include "domain/Tournament.hpp"

using ::testing::Return;
using ::testing::_;
using ::testing::NiceMock;

// Mock del Repository
class MockTournamentRepository : public IRepository<domain::Tournament, std::string> {
public:
    MOCK_METHOD(std::string, Create, (const domain::Tournament&), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, ReadById, (std::string), (override));
    MOCK_METHOD(std::string, Update, (const domain::Tournament&), (override));
    MOCK_METHOD(void, Delete, (std::string), (override));
};

// Mock del QueueMessageProducer
class MockQueueMessageProducer : public QueueMessageProducer {
public:
    MOCK_METHOD(void, SendMessage, (const std::string&, const std::string&), (override));
};

class TournamentDelegateTest : public ::testing::Test {
protected:
    std::shared_ptr<MockTournamentRepository> mockRepository;
    std::shared_ptr<NiceMock<MockQueueMessageProducer>> mockProducer;
    std::shared_ptr<TournamentDelegate> delegate;

    void SetUp() override {
        mockRepository = std::make_shared<MockTournamentRepository>();
        mockProducer = std::make_shared<NiceMock<MockQueueMessageProducer>>();
        delegate = std::make_shared<TournamentDelegate>(mockRepository, mockProducer);
    }
};

TEST_F(TournamentDelegateTest, CreateTournament_ValidTournament_ReturnsId) {
    // Arrange
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament",
        domain::TournamentFormat(2, 8, domain::TournamentType::ROUND_ROBIN));

    std::string expectedId = "generated-id-123";

    EXPECT_CALL(*mockRepository, Create(_))
        .WillOnce(Return(expectedId));

    EXPECT_CALL(*mockProducer, SendMessage(expectedId, "tournament.created"))
        .Times(1);

    // Act
    std::string resultId = delegate->CreateTournament(tournament);

    // Assert
    EXPECT_EQ(resultId, expectedId);
    EXPECT_EQ(tournament->Groups().size(), 2); // Verificar que se crearon 2 grupos
    EXPECT_EQ(tournament->Groups()[0].Name(), "Tournament A");
    EXPECT_EQ(tournament->Groups()[1].Name(), "Tournament B");
}

TEST_F(TournamentDelegateTest, CreateTournament_NullTournament_ReturnsEmptyString) {
    // Arrange
    std::shared_ptr<domain::Tournament> nullTournament = nullptr;

    // Act
    std::string resultId = delegate->CreateTournament(nullTournament);

    // Assert
    EXPECT_EQ(resultId, "");
}

TEST_F(TournamentDelegateTest, CreateTournament_CreatesCorrectNumberOfGroups) {
    // Arrange
    int numberOfGroups = 4;
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament",
        domain::TournamentFormat(numberOfGroups, 16, domain::TournamentType::ROUND_ROBIN));

    EXPECT_CALL(*mockRepository, Create(_))
        .WillOnce(Return("some-id"));

    // Act
    delegate->CreateTournament(tournament);

    // Assert
    EXPECT_EQ(tournament->Groups().size(), numberOfGroups);
    EXPECT_EQ(tournament->Groups()[0].Name(), "Tournament A");
    EXPECT_EQ(tournament->Groups()[1].Name(), "Tournament B");
    EXPECT_EQ(tournament->Groups()[2].Name(), "Tournament C");
    EXPECT_EQ(tournament->Groups()[3].Name(), "Tournament D");
}

TEST_F(TournamentDelegateTest, ReadAll_ReturnsAllTournaments) {
    // Arrange
    auto tournament1 = std::make_shared<domain::Tournament>("Tournament 1");
    auto tournament2 = std::make_shared<domain::Tournament>("Tournament 2");

    std::vector<std::shared_ptr<domain::Tournament>> expectedTournaments = {
        tournament1, tournament2
    };

    EXPECT_CALL(*mockRepository, ReadAll())
        .WillOnce(Return(expectedTournaments));

    // Act
    auto result = delegate->ReadAll();

    // Assert
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0]->Name(), "Tournament 1");
    EXPECT_EQ(result[1]->Name(), "Tournament 2");
}

TEST_F(TournamentDelegateTest, ReadAll_EmptyRepository_ReturnsEmptyVector) {
    // Arrange
    std::vector<std::shared_ptr<domain::Tournament>> emptyVector;

    EXPECT_CALL(*mockRepository, ReadAll())
        .WillOnce(Return(emptyVector));

    // Act
    auto result = delegate->ReadAll();

    // Assert
    EXPECT_TRUE(result.empty());
}

TEST_F(TournamentDelegateTest, UpdateTournament_ValidTournament_CallsRepository) {
    // Arrange
    std::string tournamentId = "test-id";
    auto tournament = std::make_shared<domain::Tournament>("Updated Tournament");
    tournament->Id() = tournamentId;

    EXPECT_CALL(*mockRepository, Update(_))
        .WillOnce(Return(tournamentId));

    EXPECT_CALL(*mockProducer, SendMessage(tournamentId, "tournament.updated"))
        .Times(1);

    // Act
    delegate->UpdateTournament(tournamentId, tournament);

    // Assert - Verificaciones hechas por los EXPECT_CALL
}

TEST_F(TournamentDelegateTest, DeleteTournament_ValidId_CallsRepositoryAndProducer) {
    // Arrange
    std::string tournamentId = "test-id-to-delete";

    EXPECT_CALL(*mockRepository, Delete(tournamentId))
        .Times(1);

    EXPECT_CALL(*mockProducer, SendMessage(tournamentId, "tournament.deleted"))
        .Times(1);

    // Act
    delegate->DeleteTournament(tournamentId);

    // Assert - Verificaciones hechas por los EXPECT_CALL
}