#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>

#include "delegate/TournamentDelegate.hpp"
#include "domain/Tournament.hpp"
#include "persistence/repository/IRepository.hpp"

// Mock del repositorio (el que ya tienes en tests/mocks)
#include "TournamentRepositoryMock.hpp"

// ⚠️ Importante: mockeamos la CLASE CONCRETA que usa el delegate:
#include "cms/QueueMessageProducer.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::Throw;
using ::testing::StrEq;

// -----------------------------------------------------------------------------
// Mock del QueueMessageProducer real.
// Fírmalo EXACTO como en la interfaz/base (con referencias &).
// Y llama al ctor del base con nullptr si tu base lo permite.
// -----------------------------------------------------------------------------
class MockQueueMessageProducer : public QueueMessageProducer {
public:
    MockQueueMessageProducer() : QueueMessageProducer(nullptr) {}

    // Firma EXACTA: (const std::string_view&, const std::string_view&)
    MOCK_METHOD(void, SendMessage,
                (const std::string_view& message, const std::string_view& queue),
                (override));
};

class TournamentDelegateTest : public ::testing::Test {
protected:
    std::shared_ptr<MockTournamentRepository> repo;
    std::shared_ptr<MockQueueMessageProducer> mockProducer; // para EXPECT_CALL
    std::shared_ptr<TournamentDelegate>       delegate;

    void SetUp() override {
        repo         = std::make_shared<MockTournamentRepository>();
        mockProducer = std::make_shared<MockQueueMessageProducer>();
        // Se convierte implícitamente a std::shared_ptr<QueueMessageProducer>
        delegate     = std::make_shared<TournamentDelegate>(repo, mockProducer);
    }
};

// Crear OK -> regresa ID y envía evento
TEST_F(TournamentDelegateTest, CreateTournament_Valid_SendsMessageAndReturnsId) {
    auto t = std::make_shared<domain::Tournament>(
        "Torneo X",
        domain::TournamentFormat(2, 8, domain::TournamentType::ROUND_ROBIN)
    );

    EXPECT_CALL(*repo, Create(_)).WillOnce(Return("gen-id-1"));
    EXPECT_CALL(*mockProducer, SendMessage(StrEq("gen-id-1"), StrEq("tournament.created"))).Times(1);

    auto id = delegate->CreateTournament(t);
    EXPECT_EQ(id, "gen-id-1");
    EXPECT_EQ(t->Groups().size(), 2);
}

// Crear falla (repo lanza) -> delega devuelve ""
TEST_F(TournamentDelegateTest, CreateTournament_Failed_ReturnsEmpty) {
    auto t = std::make_shared<domain::Tournament>(
        "DUPLICADO",
        domain::TournamentFormat(1, 16, domain::TournamentType::ROUND_ROBIN)
    );

    EXPECT_CALL(*repo, Create(_)).WillOnce(Throw(std::runtime_error("duplicate")));

    auto id = delegate->CreateTournament(t);
    EXPECT_TRUE(id.empty());
}

// GET por id OK (verifica mock del repo)
TEST_F(TournamentDelegateTest, ReadById_ReturnsObject) {
    auto t = std::make_shared<domain::Tournament>("Torneo A");
    t->Id() = "id-123";

    EXPECT_CALL(*repo, ReadById("id-123")).WillOnce(Return(t));

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

// ReadAll vacío
TEST_F(TournamentDelegateTest, ReadAll_EmptyList) {
    EXPECT_CALL(*repo, ReadAll()).WillOnce(Return(std::vector<std::shared_ptr<domain::Tournament>>{}));
    auto list = delegate->ReadAll();
    EXPECT_TRUE(list.empty());
}

// Update OK: llama repo y emite evento
TEST_F(TournamentDelegateTest, UpdateTournament_Ok_CallsRepoAndSendsEvent) {
    auto t = std::make_shared<domain::Tournament>("Nuevo Nombre");
    t->Id() = "id-999";

    EXPECT_CALL(*repo, Update(_)).WillOnce(Return("id-999"));
    EXPECT_CALL(*mockProducer, SendMessage(StrEq("id-999"), StrEq("tournament.updated"))).Times(1);

    delegate->UpdateTournament("id-999", t);
    SUCCEED();
}
