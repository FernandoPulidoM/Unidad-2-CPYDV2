//
// Adapted test suite for TeamController
// Author: fmendivil - 10/20/25
//
// Esta versión modifica nombres, estilo de comentarios y estructura,
// pero conserva la validación funcional original.
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>

#include "controller/TeamController.hpp"
#include "mocks/TeamDelegateMock.hpp"

using ::testing::StrictMock;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::Invoke;
using nlohmann::json;
using namespace std::literals;

// ======== utilidades ========
static crow::request makeRequest(const std::string& body) {
    crow::request req;
    req.body = body;
    return req;
}

static std::shared_ptr<domain::Team> fakeTeam(std::string id, std::string name) {
    return std::make_shared<domain::Team>(domain::Team{std::move(id), std::move(name)});
}

// =========================================================
// CASOS DE PRUEBA - CREACIÓN DE EQUIPOS
// =========================================================

// Caso 1: Inserción exitosa
TEST(TeamControllerSpec, CreateTeam_ValidJson_Returns201) {
    auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
    TeamController controller{mock};

    EXPECT_CALL(*mock, GetAllTeams())
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Team>>{}));
    EXPECT_CALL(*mock, SaveTeam(::testing::_))
        .WillOnce(Return(std::string_view{"NEW-99"}));

    auto req = makeRequest(R"({"name":"Falcons"})");
    auto res = controller.SaveTeam(req);

    EXPECT_EQ(res.code, crow::CREATED);
    EXPECT_EQ(res.get_header_value("location"), "NEW-99");
    EXPECT_THAT(res.get_header_value("content-type"), ::testing::HasSubstr("application/json"));
    EXPECT_THAT(std::string(res.body), ::testing::HasSubstr(R"("id":"NEW-99")"));
}

// Caso 2: Nombre duplicado → conflicto
TEST(TeamControllerSpec, CreateTeam_DuplicateName_409Conflict) {
    auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
    TeamController controller{mock};

    EXPECT_CALL(*mock, GetAllTeams())
        .WillOnce(Return(std::vector{
            fakeTeam("X1", "Panthers")
        }));

    auto req = makeRequest(R"({"name":"Panthers"})");
    auto res = controller.SaveTeam(req);

    EXPECT_EQ(res.code, crow::CONFLICT);
    EXPECT_THAT(std::string(res.body), ::testing::HasSubstr("already exists"));
}

// Caso 3: ID inválido en creación → 400
TEST(TeamControllerSpec, CreateTeam_InvalidClientId_Returns400) {
    auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
    TeamController controller{mock};

    EXPECT_CALL(*mock, GetAllTeams())
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Team>>{}));

    auto req = makeRequest(R"({"id":"bad id","name":"Jets"})");
    auto res = controller.SaveTeam(req);
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

// Caso 4: Inserción lanza excepción → 500
TEST(TeamControllerSpec, CreateTeam_InsertThrows_InternalError500) {
    auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
    TeamController controller{mock};

    EXPECT_CALL(*mock, GetAllTeams())
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Team>>{}));
    EXPECT_CALL(*mock, SaveTeam(::testing::_))
        .WillOnce(Invoke([](const domain::Team&) -> std::string_view {
            throw std::runtime_error("insertion error");
        }));

    auto req = makeRequest(R"({"name":"Titans"})");
    auto res = controller.SaveTeam(req);
    EXPECT_EQ(res.code, crow::INTERNAL_SERVER_ERROR);
    EXPECT_THAT(std::string(res.body), ::testing::HasSubstr("error creating team"));
}

// Caso 5: JSON inválido → 400
TEST(TeamControllerSpec, CreateTeam_InvalidJsonSyntax_400) {
    auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
    TeamController ctl{mock};
    auto res = ctl.SaveTeam(makeRequest("{invalid"));
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

// =========================================================
// BÚSQUEDAS INDIVIDUALES Y COLECTIVAS
// =========================================================

// Caso 6: Buscar por ID válido → 200
TEST(TeamControllerSpec, GetTeam_Found_ReturnsJson200) {
    auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
    EXPECT_CALL(*mock, GetTeam("T01"sv))
        .WillOnce(Return(fakeTeam("T01", "Bulls")));

    TeamController controller{mock};
    auto res = controller.getTeam("T01");

    EXPECT_EQ(res.code, crow::OK);
    EXPECT_THAT(res.get_header_value("content-type"), ::testing::HasSubstr("application/json"));
    json parsed = json::parse(res.body);
    EXPECT_EQ(parsed.at("id"), "T01");
    EXPECT_EQ(parsed.at("name"), "Bulls");
}

// Caso 7: Buscar por ID no existente → 404
TEST(TeamControllerSpec, GetTeam_NotFound_Returns404) {
    auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
    EXPECT_CALL(*mock, GetTeam("NF"sv)).WillOnce(Return(nullptr));

    TeamController controller{mock};
    auto res = controller.getTeam("NF");
    EXPECT_EQ(res.code, crow::NOT_FOUND);
}

// Caso 8: ID inválido → 400
TEST(TeamControllerSpec, GetTeam_InvalidFormat_Returns400) {
    auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
    TeamController controller{mock};
    auto res = controller.getTeam("bad id");
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

// Caso 9: Lista vacía → 200 con array vacío
TEST(TeamControllerSpec, GetAllTeams_Empty_ReturnsEmptyJsonArray) {
    auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
    EXPECT_CALL(*mock, GetAllTeams())
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Team>>{}));

    TeamController ctl{mock};
    auto res = ctl.getAllTeams();

    EXPECT_EQ(res.code, crow::OK);
    EXPECT_EQ(res.body, "[]");
    EXPECT_THAT(res.get_header_value("content-type"), ::testing::HasSubstr("application/json"));
}

// Caso 10: Lista con elementos → 200 y array JSON
TEST(TeamControllerSpec, GetAllTeams_WithData_ReturnsJsonArray200) {
    auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
    std::vector<std::shared_ptr<domain::Team>> fakeData{
        fakeTeam("A1", "Eagles"),
        fakeTeam("B2", "Wolves")
    };
    EXPECT_CALL(*mock, GetAllTeams()).WillOnce(Return(fakeData));

    TeamController ctl{mock};
    auto res = ctl.getAllTeams();

    EXPECT_EQ(res.code, crow::OK);
    json arr = json::parse(res.body);
    ASSERT_EQ(arr.size(), 2u);
    EXPECT_EQ(arr[1].at("name"), "Wolves");
}

// =========================================================
// ACTUALIZACIÓN Y ELIMINACIÓN
// =========================================================

// Caso 11: ID inválido → 400
TEST(TeamControllerSpec, UpdateTeam_InvalidId_400) {
    auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
    TeamController ctl{mock};
    auto res = ctl.UpdateTeam(makeRequest(R"({"name":"X"})"), "bad id");
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

// Caso 12: JSON inválido → 400
TEST(TeamControllerSpec, UpdateTeam_BadJson_400) {
    auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
    TeamController ctl{mock};
    auto res = ctl.UpdateTeam(makeRequest("{wrong json"), "U1");
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

// Caso 13: Eliminación con ID inválido → 400
TEST(TeamControllerSpec, DeleteTeam_BadId_Returns400) {
    auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
    TeamController ctl{mock};
    auto res = ctl.DeleteTeam("invalid id");
    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

// Caso 14: Excepción interna al eliminar → 500
TEST(TeamControllerSpec, DeleteTeam_InternalException_500) {
    auto mock = std::make_shared<StrictMock<TeamDelegateMock>>();
    TeamController ctl{mock};

    EXPECT_CALL(*mock, DeleteTeam("X1"sv))
        .WillOnce(Invoke([](std::string_view) -> void {
            throw std::runtime_error("db failure");
        }));

    auto res = ctl.DeleteTeam("X1");
    EXPECT_EQ(res.code, crow::INTERNAL_SERVER_ERROR);
}
