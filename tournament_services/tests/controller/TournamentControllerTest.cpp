#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>
#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <vector>

#include "controller/TournamentController.hpp"
// usamos el mock de delegate que ya pusiste en tests/mocks
#include "TournamentDelegateMock.hpp"
#include "domain/Tournament.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::Throw;

class TournamentControllerTest : public ::testing::Test {
protected:
    std::shared_ptr<TournamentDelegateMock> mockDelegate;
    std::shared_ptr<TournamentController>   controller;

    void SetUp() override {
        mockDelegate = std::make_shared<TournamentDelegateMock>();
        controller   = std::make_shared<TournamentController>(mockDelegate);
    }
};

// POST /tournaments -> 201 + Location
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

// POST /tournaments -> 409 si el delegate reporta duplicado
TEST_F(TournamentControllerTest, CreateTournament_Conflict_409) {
    nlohmann::json body = {{"name", "Duplicado"}};
    crow::request req; req.body = body.dump();

    EXPECT_CALL(*mockDelegate, CreateTournament(_))
        .WillOnce(Throw(std::runtime_error("duplicate")));

    auto resp = controller->CreateTournament(req);
    EXPECT_EQ(resp.code, crow::CONFLICT);
}

// GET /tournaments -> 200 con elementos
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

// GET /tournaments -> 200 lista vacía
TEST_F(TournamentControllerTest, ReadAll_200_Empty) {
    EXPECT_CALL(*mockDelegate, ReadAll())
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Tournament>>{}));

    auto resp = controller->ReadAll();
    EXPECT_EQ(resp.code, crow::OK);

    auto j = nlohmann::json::parse(resp.body);
    EXPECT_TRUE(j.is_array());
    EXPECT_TRUE(j.empty());
}

// PUT/PATCH /tournaments/<id> -> OK (tu controller expone UpdateTournament)
TEST_F(TournamentControllerTest, UpdateTournament_200_OK) {
    nlohmann::json body = { {"name","TOURNAMENT NAME UPDATE"} };
    crow::request req; req.body = body.dump();

    EXPECT_CALL(*mockDelegate, UpdateTournament("tid-42", _)).Times(1);

    auto resp = controller->UpdateTournament(req, "tid-42");
    // tu controlador actualmente responde 200 OK
    EXPECT_EQ(resp.code, crow::OK);
}

// PUT/PATCH /tournaments/<id> -> 404 si el delegate avisa not found
TEST_F(TournamentControllerTest, UpdateTournament_404_NotFound) {
    nlohmann::json body = { {"name","X"} };
    crow::request req; req.body = body.dump();

    EXPECT_CALL(*mockDelegate, UpdateTournament("nope", _))
        .WillOnce(Throw(std::runtime_error("not found")));

    auto resp = controller->UpdateTournament(req, "nope");
    EXPECT_EQ(resp.code, crow::NOT_FOUND);
}
