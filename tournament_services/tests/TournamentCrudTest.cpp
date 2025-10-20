#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include "controller/TournamentController.hpp"
#include "delegate/TournamentDelegate.hpp"
#include "persistence/repository/IRepository.hpp"
#include "cms/QueueMessageProducer.hpp"
#include "domain/Tournament.hpp"

using ::testing::Mock;
using ::testing::Return;
using ::testing::Throw;
using ::testing::_;

// Mock Repository
class MockTournamentRepository : public IRepository<domain::Tournament, std::string> {
public:
    MockTournamentRepository() = default;
    virtual ~MockTournamentRepository() = default;
    MOCK_METHOD(std::string, Create, (const domain::Tournament&), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, ReadById, (const std::string&), (override));
    MOCK_METHOD(std::string, Update, (const domain::Tournament&), (override));
    MOCK_METHOD(void, Delete, (std::string), (override));
};

// Mock Queue Producer
class MockQueueMessageProducer : public QueueMessageProducer {
public:
    MockQueueMessageProducer() = default;
    virtual ~MockQueueMessageProducer() = default;
    MOCK_METHOD(void, SendMessage, (const std::string&, const std::string&), (override));
};

// Mock Delegate for Controller testing
class MockTournamentDelegate : public ITournamentDelegate {
public:
    MOCK_METHOD(std::string, CreateTournament, (std::shared_ptr<domain::Tournament>), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
    MOCK_METHOD(void, UpdateTournament, (const std::string&, std::shared_ptr<domain::Tournament>), (override));
    MOCK_METHOD(void, DeleteTournament, (const std::string&), (override));
};

// ==================== DELEGATE TESTS ====================
class TournamentDelegateTest : public ::testing::Test {
protected:
    std::shared_ptr<MockTournamentRepository> mockRepository;
    std::shared_ptr<MockQueueMessageProducer> mockProducer;
    std::shared_ptr<TournamentDelegate> delegate;

    void SetUp() override {
        // En lugar de make_shared, usa new directamente:
        mockRepository = std::shared_ptr<MockTournamentRepository>(
            new MockTournamentRepository()
        );
        mockProducer = std::shared_ptr<MockQueueMessageProducer>(
            new MockQueueMessageProducer()
        );
        delegate = std::make_shared<TournamentDelegate>(mockRepository, mockProducer);
    }
};

TEST_F(TournamentDelegateTest, CreateTournamentSuccess) {
    auto tournament = std::make_shared<domain::Tournament>("Test Tournament");
    std::string expectedId = "123e4567-e89b-12d3-a456-426614174000";

    EXPECT_CALL(*mockRepository, Create(_))
        .WillOnce(Return(expectedId));
    EXPECT_CALL(*mockProducer, SendMessage(expectedId, "tournament.created"))
        .Times(1);

    std::string result = delegate->CreateTournament(tournament);

    EXPECT_EQ(result, expectedId);
}

TEST_F(TournamentDelegateTest, ReadAllTournaments) {
    auto tournament1 = std::make_shared<domain::Tournament>("Tournament 1");
    auto tournament2 = std::make_shared<domain::Tournament>("Tournament 2");
    std::vector<std::shared_ptr<domain::Tournament>> tournaments = {tournament1, tournament2};

    EXPECT_CALL(*mockRepository, ReadAll())
        .WillOnce(Return(tournaments));

    auto result = delegate->ReadAll();

    EXPECT_EQ(result.size(), 2);
}

TEST_F(TournamentDelegateTest, UpdateTournamentSuccess) {
    std::string id = "123e4567-e89b-12d3-a456-426614174000";
    auto tournament = std::make_shared<domain::Tournament>("Updated Tournament");
    tournament->Id() = id;

    EXPECT_CALL(*mockRepository, Update(_))
        .WillOnce(Return(id));
    EXPECT_CALL(*mockProducer, SendMessage(id, "tournament.updated"))
        .Times(1);

    delegate->UpdateTournament(id, tournament);
}

TEST_F(TournamentDelegateTest, DeleteTournamentSuccess) {
    std::string id = "123e4567-e89b-12d3-a456-426614174000";

    EXPECT_CALL(*mockRepository, Delete(id))
        .Times(1);
    EXPECT_CALL(*mockProducer, SendMessage(id, "tournament.deleted"))
        .Times(1);

    delegate->DeleteTournament(id);
}

TEST_F(TournamentDelegateTest, DeleteTournamentNotFound) {
    std::string id = "non-existent-id";

    EXPECT_CALL(*mockRepository, Delete(id))
        .WillOnce(Throw(std::runtime_error("Tournament not found")));

    EXPECT_THROW(delegate->DeleteTournament(id), std::runtime_error);
}

// ==================== CONTROLLER TESTS ====================
class TournamentControllerTest : public ::testing::Test {
protected:
    std::shared_ptr<MockTournamentDelegate> mockDelegate;
    std::shared_ptr<TournamentController> controller;

    void SetUp() override {
        mockDelegate = std::make_shared<MockTournamentDelegate>();
        controller = std::make_shared<TournamentController>(mockDelegate);
    }
};

TEST_F(TournamentControllerTest, CreateTournamentReturnsCreated) {
    crow::request request;
    request.body = R"({"name":"Test Tournament","format":{"type":"ROUND_ROBIN","numberOfGroups":1,"maxTeamsPerGroup":16}})";

    std::string tournamentId = "123e4567-e89b-12d3-a456-426614174000";
    EXPECT_CALL(*mockDelegate, CreateTournament(_))
        .WillOnce(Return(tournamentId));

    auto response = controller->CreateTournament(request);

    EXPECT_EQ(response.code, crow::CREATED);
    EXPECT_EQ(response.get_header_value("location"), tournamentId);
}

TEST_F(TournamentControllerTest, ReadAllReturnsOk) {
    auto tournament1 = std::make_shared<domain::Tournament>("Tournament 1");
    auto tournament2 = std::make_shared<domain::Tournament>("Tournament 2");
    std::vector<std::shared_ptr<domain::Tournament>> tournaments = {tournament1, tournament2};

    EXPECT_CALL(*mockDelegate, ReadAll())
        .WillOnce(Return(tournaments));

    auto response = controller->ReadAll();

    EXPECT_EQ(response.code, crow::OK);
    EXPECT_FALSE(response.body.empty());
}

TEST_F(TournamentControllerTest, UpdateTournamentReturnsOk) {
    std::string id = "123e4567-e89b-12d3-a456-426614174000";
    crow::request request;
    request.body = R"({"name":"Updated Tournament","format":{"type":"ROUND_ROBIN","numberOfGroups":1,"maxTeamsPerGroup":16}})";

    EXPECT_CALL(*mockDelegate, UpdateTournament(id, _))
        .Times(1);

    auto response = controller->UpdateTournament(request, id);

    EXPECT_EQ(response.code, crow::OK);
}

TEST_F(TournamentControllerTest, UpdateTournamentWithInvalidJson) {
    std::string id = "123e4567-e89b-12d3-a456-426614174000";
    crow::request request;
    request.body = "invalid json {";

    auto response = controller->UpdateTournament(request, id);

    EXPECT_EQ(response.code, crow::BAD_REQUEST);
}

TEST_F(TournamentControllerTest, DeleteTournamentReturnsNoContent) {
    std::string id = "123e4567-e89b-12d3-a456-426614174000";

    EXPECT_CALL(*mockDelegate, DeleteTournament(id))
        .Times(1);

    auto response = controller->DeleteTournament(id);

    EXPECT_EQ(response.code, crow::NO_CONTENT);
}

TEST_F(TournamentControllerTest, DeleteTournamentNotFound) {
    std::string id = "non-existent-id";

    EXPECT_CALL(*mockDelegate, DeleteTournament(id))
        .WillOnce(Throw(std::runtime_error("Tournament not found")));

    auto response = controller->DeleteTournament(id);

    EXPECT_EQ(response.code, crow::NOT_FOUND);
}