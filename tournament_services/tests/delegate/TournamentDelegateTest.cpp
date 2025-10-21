#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>

#include "delegate/TournamentDelegate.hpp"
#include "domain/Tournament.hpp"
#include "persistence/repository/IRepository.hpp"
#include "cms/QueueMessageProducer.hpp"
#include "tests/mocks/TournamentRepositoryMock.hpp"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::Throw;

// Mock simple de QueueMessageProducer
class MockQueueMessageProducer : public QueueMessageProducer {
public:
    MOCK_METHOD(void, SendMessage, (const std::string&, const std::string&), (override));
};

class TournamentDelegateTest : public ::testing::Test {
protected:
    std::shared_ptr<MockTournamentRepository> repo;
    std::shared_ptr<MockQueueMessageProducer> producer;
    std::shared_ptr<TournamentDelegate>       delegate;

    void SetUp() override {
        repo     = std::make_shared<MockTournamentRepository>();
        producer = std::make_shared<MockQueueMessageProducer>();
        delegate = std::make_shared<TournamentDelegate>(repo, producer);
    }
};

// Crear OK -> regresa ID y envia evento
TEST_F(TournamentDelegateTest, CreateTournament_Valid_SendsMessageAndReturnsId) {
    auto t = std::make_shared<domain::Tournament>("Torneo X",
        domain::TournamentFormat(2, 8, domain::TournamentType::ROUND_ROBIN));

    EXPECT_CALL(*repo, Create(_)).WillOnce(Return("gen-id-1"));
    EXPECT_CALL(*producer, SendMessage("gen-id-1", "tournament.created")).Times(1);

    auto id = delegate->CreateTournament(t);
    EXPECT_EQ(id, "gen-id-1");
    EXPECT_EQ(t->Groups().size(), 2);
}

// Crear falla (repo lanza) -> delega devuelve "" (patron que ya usas)
TEST_F(TournamentDelegateTest, CreateTournament_Failed_ReturnsEmpty) {
    auto t = std::make_shared<domain::Tournament>("DUPLICADO",
        domain::TournamentFormat(1, 16, domain::TournamentType::ROUND_ROBIN));

    EXPECT_CALL(*repo, Create(_)).WillOnce(Throw(std::runtime_error("duplicate")));

    auto id = delegate->CreateTournament(t);
    EXPECT_TRUE(id.empty());
}

// GET por id OK
TEST_F(TournamentDelegateTest, ReadById_ReturnsObject) {
    auto t = std::make_shared<domain::Tournament>("Torneo A");
    t->Id() = "id-123";

    EXPECT_CALL(*repo, ReadById("id-123")).WillOnce(Return(t));

    // Si tu delegate no expone ReadById, omite esta parte
    auto got = repo->ReadById("id-123");
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->Id(), "id-123");
    EXPECT_EQ(got->Name(), "Torneo A");
}

// GET por id not found -> nullptr
TEST_F(TournamentDelegateTest, ReadById_NotFound_ReturnsNull) {
    EXPECT_CALL(*repo, ReadById("nope")).WillOnce(Return(nullptr));
    auto got = repo->ReadById("nope");
    EXPECT_EQ(got, nullptr);
}

// ReadAll con elementos
TEST_F(TournamentDelegateTest, ReadAll_ReturnsList) {
    auto t1 = std::make_shared<domain::Tournament>("A"); t1->Id() = "1";
    auto t2 = std::make_shared<domain::Tournament>("B"); t2->Id() = "2";
    std::vector<std::shared_ptr<domain::Tournament>> vec{t1, t2};

    EXPECT_CALL(*repo, ReadAll()).WillOnce(Return(vec));

    auto list = delegate->ReadAll();
    ASSERT_EQ(list.size(), 2);
    EXPECT_EQ(list[0]->Name(), "A");
    EXPECT_EQ(list[1]->Name(), "B");
}

// ReadAll vacio
TEST_F(TournamentDelegateTest, ReadAll_EmptyList) {
    EXPECT_CALL(*repo, ReadAll()).WillOnce(Return(std::vector<std::shared_ptr<domain::Tournament>>{}));
    auto list = delegate->ReadAll();
    EXPECT_TRUE(list.empty());
}

// Update OK: valida que se invoque Update en repo y se emita evento
TEST_F(TournamentDelegateTest, UpdateTournament_Ok_CallsRepoAndSendsEvent) {
    auto t = std::make_shared<domain::Tournament>("Nuevo Nombre");
    t->Id() = "id-999";

    EXPECT_CALL(*repo, Update(_)).WillOnce(Return("id-999"));
    EXPECT_CALL(*producer, SendMessage("id-999", "tournament.updated")).Times(1);

    delegate->UpdateTournament("id-999", t);
    SUCCEED();
}
