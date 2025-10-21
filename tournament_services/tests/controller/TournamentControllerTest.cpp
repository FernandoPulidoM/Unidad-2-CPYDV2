#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>
#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <vector>

#include "controller/TournamentController.hpp"
#include "tests/mocks/TournamentDelegateMock.hpp"
#include "domain/Tournament.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::Throw;
using ::testing::Truly;

class TournamentControllerTest : public ::testing::Test {
protected:
    std::shared_ptr<TournamentDelegateMock> mockDelegate;
    std::shared_ptr<TournamentController>   controller;

    void SetUp() override {
        mockDelegate = std::make_shared<TournamentDelegateMock>();
        controller   = std::make_shared<TournamentController>(mockDelegate);
    }
};

// POST /tournaments -> 201 + Location, valida transformacion
TEST_F(TournamentControllerTest, CreateTournament_Valid_201_Location) {
    nlohmann::json body = {
        {"name", "Torneo Verano 2025"},
        {"format", {{"type","ROUND_ROBIN"},{"numberOfGroups",2},{"maxTeamsPerGroup",8}}}
    };
    crow::request req; req.body = body.dump();

    EXPECT_CALL(*mockDelegate, CreateTournament(_))
        .WillOnce(Return("new-id-abc"));

    auto resp = controller->CreateTournament(req);
    EXPECT_EQ(resp.code, crow::CREATED);
    EXPECT_EQ(resp.get_header_value("location"), "new-id-abc");
}

// POST /tournaments -> 409 si delegate lanza (duplicado)
TEST_F(TournamentControllerTest, CreateTournament_Conflict_409) {
    nlohmann::json body = {{"name", "Duplicado"}};
    crow::request req; req.body = body.dump();

    EXPECT_CALL(*mockDelegate, CreateTournament(_))
        .WillOnce(Throw(std::runtime_error("duplicate")));

    auto resp = controller->CreateTournament(req);
    EXPECT_EQ(resp.code, crow::CONFLICT);
}

// GET /tournaments/<id> -> 200 (simula objeto devuelto)
TEST_F(TournamentControllerTest, GetTournamentById_200) {
    auto t = std::make_shared<domain::Tournament>("Torneo Invierno");
    t->Id() = "tid-1";

    // Nota: si tu metodo se llama distinto, cambia aquí
    EXPECT_CALL(*mockDelegate, ReadById("tid-1")).WillOnce(Return(t));

    auto resp = controller->GetTournament("tid-1");
    EXPECT_EQ(resp.code, crow::OK);

    auto j = crow::json::load(resp.body);
    ASSERT_TRUE(j);
    EXPECT_EQ(std::string(j["id"].s()), "tid-1");
    EXPECT_EQ(std::string(j["name"].s()), "Torneo Invierno");
}

// GET /tournaments/<id> -> 404
TEST_F(TournamentControllerTest, GetTournamentById_404) {
    EXPECT_CALL(*mockDelegate, ReadById("nope")).WillOnce(Return(nullptr));
    auto resp = controller->GetTournament("nope");
    EXPECT_EQ(resp.code, crow::NOT_FOUND);
}

// GET /tournaments -> 200 lista con elementos
TEST_F(TournamentControllerTest, ReadAll_200_WithItems) {
    auto t1 = std::make_shared<domain::Tournament>("A"); t1->Id() = "1";
    auto t2 = std::make_shared<domain::Tournament>("B"); t2->Id() = "2";
    std::vector<std::shared_ptr<domain::Tournament>> list{t1, t2};

    EXPECT_CALL(*mockDelegate, ReadAll()).WillOnce(Return(list));

    auto resp = controller->ReadAll();
    EXPECT_EQ(resp.code, crow::OK);

    auto j = nlohmann::json::parse(resp.body);
    ASSERT_EQ(j.size(), 2);
}

// GET /tournaments -> 200 lista vacia
TEST_F(TournamentControllerTest, ReadAll_200_Empty) {
    EXPECT_CALL(*mockDelegate, ReadAll())
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Tournament>>{}));

    auto resp = controller->ReadAll();
    EXPECT_EQ(resp.code, crow::OK);

    auto j = nlohmann::json::parse(resp.body);
    EXPECT_TRUE(j.is_array());
    EXPECT_TRUE(j.empty());
}

// PATCH /tournaments/<id> -> 204 (valida transformacion y llamada)
TEST_F(TournamentControllerTest, PatchTournament_204) {
    nlohmann::json body = { {"name","TOURNAMENT NAME UPDATE"} };
    crow::request req; req.body = body.dump();

    EXPECT_CALL(*mockDelegate, UpdateTournament("tid-42", _)).Times(1);
    auto resp = controller->PatchTournament(req, "tid-42"); // cambia a UpdateTournament si tu metodo se llama asi
    EXPECT_EQ(resp.code, crow::NO_CONTENT);
}

// PATCH /tournaments/<id> -> 404 si delegate lanza not found
TEST_F(TournamentControllerTest, PatchTournament_404) {
    nlohmann::json body = { {"name","X"} };
    crow::request req; req.body = body.dump();

    EXPECT_CALL(*mockDelegate, UpdateTournament("nope", _))
        .WillOnce(Throw(std::runtime_error("not found")));

    auto resp = controller->PatchTournament(req, "nope"); // cambia al nombre real
    EXPECT_EQ(resp.code, crow::NOT_FOUND);
}
