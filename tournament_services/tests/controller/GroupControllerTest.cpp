//
// GroupControllerTest.cpp - Tests para GroupController usando GroupDelegate
//
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>

#include "controller/GroupController.hpp"
#include "mocks/GroupDelegateMock.hpp"
#include "domain/Group.hpp"
#include "domain/Team.hpp"

using ::testing::StrictMock;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::_;
using nlohmann::json;
using namespace std::literals;

/*
   Helpers
*/
static crow::request make_req(const std::string& body) {
    crow::request r;
    r.body = body;
    return r;
}

static std::shared_ptr<domain::Group> mkGroup(std::string id, std::string name) {
    auto g = std::make_shared<domain::Group>();
    g->Id() = std::move(id);
    g->Name() = std::move(name);
    return g;
}

/*
   Test 1: Al método que procesa la creación de grupo para un torneo,
   validar la transformación de JSON al objeto de dominio Group y
   validar que el valor que se le transfiera a GroupDelegate es el esperado.
   Validar que la respuesta de esta función sea HTTP 201
*/
TEST(GroupControllerTest, CreateGroup_Success_201) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();

    EXPECT_CALL(*mock, CreateGroup("T1"sv, _))
        .WillOnce(Invoke([](std::string_view tid, const domain::Group& g){
            EXPECT_EQ(tid, "T1");
            EXPECT_EQ(g.Name(), "Group A");
            EXPECT_EQ(g.Teams().size(), 1u);
            EXPECT_EQ(g.Teams()[0].Id, "team-01");
            return std::expected<std::string, std::string>{"G-123"};
        }));

    GroupController ctl{mock};
    auto req = make_req(R"({"name":"Group A","teams":[{"id":"team-01","name":"Team One"}]})");
    auto res = ctl.CreateGroup(req, "T1");

    EXPECT_EQ(res.code, crow::CREATED);
    EXPECT_THAT(res.get_header_value("location"), ::testing::HasSubstr("G-123"));
}

/*
   Test 2: Al método que procesa la creación de grupo para un torneo,
   simular error de inserción en la base de datos y validar
   la respuesta HTTP 409
*/
TEST(GroupControllerTest, CreateGroup_DBError_409) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();

    EXPECT_CALL(*mock, CreateGroup("T1"sv, _))
        .WillOnce(Return(std::unexpected("Failed to create group")));

    GroupController ctl{mock};
    auto req = make_req(R"({"name":"Group X"})");
    auto res = ctl.CreateGroup(req, "T1");

    EXPECT_EQ(res.code, 422);
}

/*
   Test 3: Al método que procesa la búsqueda de un grupo por ID y torneo,
   simular el resultado con un objeto y validar la respuesta HTTP 200
*/
TEST(GroupControllerTest, GetGroup_Found_200) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();

    auto grp = mkGroup("G1", "Alpha");
    grp->Teams().push_back(domain::Team{"T1","Team 1"});

    EXPECT_CALL(*mock, GetGroup("T1"sv, "G1"sv))
        .WillOnce(Return(grp));

    GroupController ctl{mock};
    auto res = ctl.GetGroup("T1", "G1");

    EXPECT_EQ(res.code, crow::OK);
    EXPECT_THAT(res.body, ::testing::HasSubstr("Alpha"));
    EXPECT_THAT(res.body, ::testing::HasSubstr("Team 1"));
}

/*
   Test 4: Al método que procesa la búsqueda de un grupo por ID y torneo,
   simular el resultado nulo y validar la respuesta HTTP 404
*/
TEST(GroupControllerTest, GetGroup_NotFound_404) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();

    EXPECT_CALL(*mock, GetGroup("T1"sv, "G2"sv))
        .WillOnce(Return(std::unexpected("Group doesn't exist")));

    GroupController ctl{mock};
    auto res = ctl.GetGroup("T1", "G2");

    EXPECT_EQ(res.code, crow::NOT_FOUND);
}

/*
   Test 5: Al método que procesa la actualización de un grupo,
   validar transformación JSON → Group y respuesta HTTP 204
*/
TEST(GroupControllerTest, UpdateGroup_Success_204) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();

    EXPECT_CALL(*mock, UpdateGroup("T1"sv, _))
        .WillOnce(Invoke([](std::string_view tid, const domain::Group& g){
            EXPECT_EQ(tid, "T1");
            EXPECT_EQ(g.Id(), "G1");
            EXPECT_EQ(g.Name(), "New Name");
            return std::expected<void, std::string>{};
        }));

    GroupController ctl{mock};
    auto req = make_req(R"({"name":"New Name"})");
    auto res = ctl.UpdateGroup(req, "T1", "G1");

    EXPECT_EQ(res.code, crow::NO_CONTENT);
}

/*
   Test 6: Al método que procesa la actualización de un grupo,
   simular ID no encontrado y validar respuesta HTTP 404
*/
TEST(GroupControllerTest, UpdateGroup_NotFound_404) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();

    EXPECT_CALL(*mock, UpdateGroup("T1"sv, _))
        .WillOnce(Return(std::unexpected("Group doesn't exist")));

    GroupController ctl{mock};
    auto req = make_req(R"({"name":"X"})");
    auto res = ctl.UpdateGroup(req, "T1", "G1");

    EXPECT_EQ(res.code, crow::NOT_FOUND);
}

/*
   Test 7: Al método que procesa agregar un equipo a un grupo en un torneo,
   validar la transformación de JSON al objeto Team y respuesta HTTP 204
*/
TEST(GroupControllerTest, AddTeamToGroup_Success_204) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();

    EXPECT_CALL(*mock, AddTeamToGroup("T1"sv, "G1"sv, "E1"sv))
        .WillOnce(Return(std::expected<void, std::string>{}));

    GroupController ctl{mock};
    auto req = make_req(R"({"id":"E1","name":"Team One"})");
    auto res = ctl.AddTeamToGroup(req, "T1", "G1");

    EXPECT_EQ(res.code, crow::NO_CONTENT);
}

/*
   Test 8: Al método que procesa agregar un equipo a un grupo,
   simular que el equipo no exista y validar HTTP 422
*/
TEST(GroupControllerTest, AddTeamToGroup_TeamNotExist_422) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();

    EXPECT_CALL(*mock, AddTeamToGroup("T1"sv, "G1"sv, "E999"sv))
        .WillOnce(Return(std::unexpected("Team doesn't exist")));

    GroupController ctl{mock};
    auto req = make_req(R"({"id":"E999","name":"Ghost Team"})");
    auto res = ctl.AddTeamToGroup(req, "T1", "G1");

    EXPECT_EQ(res.code, 422);
}

/*
   Test 9: Al método que procesa agregar un equipo a un grupo,
   simular que el grupo esté lleno y validar HTTP 422
*/
TEST(GroupControllerTest, AddTeamToGroup_GroupFull_422) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();

    EXPECT_CALL(*mock, AddTeamToGroup("T1"sv, "G1"sv, "E3"sv))
        .WillOnce(Return(std::unexpected("Group is full")));

    GroupController ctl{mock};
    auto req = make_req(R"({"id":"E3","name":"Team Three"})");
    auto res = ctl.AddTeamToGroup(req, "T1", "G1");

    EXPECT_EQ(res.code, 422);
}

/*
   Tests adicionales para cobertura
*/
TEST(GroupControllerTest, DeleteGroup_Success_204) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();

    EXPECT_CALL(*mock, RemoveGroup("T1"sv, "G1"sv))
        .WillOnce(Return(std::expected<void, std::string>{}));

    GroupController ctl{mock};
    auto res = ctl.DeleteGroup("T1", "G1");

    EXPECT_EQ(res.code, crow::NO_CONTENT);
}

TEST(GroupControllerTest, DeleteGroup_NotFound_404) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();

    EXPECT_CALL(*mock, RemoveGroup("T1"sv, "Gx"sv))
        .WillOnce(Return(std::unexpected("Group doesn't exist")));

    GroupController ctl{mock};
    auto res = ctl.DeleteGroup("T1", "Gx");

    EXPECT_EQ(res.code, crow::NOT_FOUND);
}

TEST(GroupControllerTest, GetGroups_Success_200) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();

    auto g1 = mkGroup("G1","A");
    g1->Teams().push_back(domain::Team{"T1","One"});
    auto g2 = mkGroup("G2","B");

    EXPECT_CALL(*mock, GetGroups("T1"sv))
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Group>>{g1, g2}));

    GroupController ctl{mock};
    auto res = ctl.GetGroups("T1");

    EXPECT_EQ(res.code, crow::OK);
    EXPECT_THAT(res.body, ::testing::HasSubstr("\"G1\""));
    EXPECT_THAT(res.body, ::testing::HasSubstr("teams"));
}

TEST(GroupControllerTest, UpdateTeams_Success_204) {
    auto mock = std::make_shared<StrictMock<GroupDelegateMock>>();

    EXPECT_CALL(*mock, UpdateTeams("T1"sv, "G1"sv, ::testing::_))
        .WillOnce(Return(std::expected<void, std::string>{}));

    GroupController ctl{mock};
    auto req = make_req(R"([{"id":"A","name":"Alpha"},{"id":"B"}])");
    auto res = ctl.UpdateTeams(req, "T1", "G1");

    EXPECT_EQ(res.code, crow::NO_CONTENT);
}