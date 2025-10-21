//
// Created by HiramZ04 on 10/21/25.
//
//
// Author: fmendivil - 10/20/25
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "../../include/delegate/TeamDelegate.hpp"
#include "../mocks/TeamRepositoryMock.h"
#include "../../../tournament_common/include/domain/Team.hpp"

using ::testing::NiceMock;
using ::testing::StrictMock;
using ::testing::Return;
using ::testing::InSequence;
using namespace std::literals;

// ====== Utilidad auxiliar ======
// Crea un puntero a objeto Team simulando uno real
static std::shared_ptr<domain::Team> fakeTeam(std::string id, std::string name) {
    return std::make_shared<domain::Team>(domain::Team{std::move(id), std::move(name)});
}

// ===============================================================
// CASOS DE PRUEBA
// ===============================================================

// Caso 1: Inserción exitosa
// Verifica que el ID retornado sea el mismo que simula el repositorio
TEST(TeamDelegateSpec, CreateTeam_ReturnsGeneratedId) {
    auto mockRepo = std::make_shared<StrictMock<MockTeamRepository>>();
    EXPECT_CALL(*mockRepo, Create(::testing::_)).WillOnce(Return("gen-001"sv));

    TeamDelegate target{mockRepo};
    domain::Team t{"", "Nuevo Equipo"};

    EXPECT_EQ(target.SaveTeam(t), "gen-001");
}

// Caso 2: Fallo en inserción
// Cuando el repositorio no genera ID, se espera una respuesta vacía
TEST(TeamDelegateSpec, CreateTeam_Fails_ReturnsEmpty) {
    auto mockRepo = std::make_shared<StrictMock<MockTeamRepository>>();
    EXPECT_CALL(*mockRepo, Create(::testing::_)).WillOnce(Return(""sv));

    TeamDelegate target{mockRepo};
    domain::Team incoming{"", "Fallido"};

    auto res = target.SaveTeam(incoming);
    EXPECT_TRUE(res.empty());
}

// Caso 3: Búsqueda individual exitosa
TEST(TeamDelegateSpec, FindTeamById_ReturnsEntity) {
    auto mockRepo = std::make_shared<StrictMock<MockTeamRepository>>();
    EXPECT_CALL(*mockRepo, ReadById("T1"sv)).WillOnce(Return(fakeTeam("T1", "Delta")));

    TeamDelegate target{mockRepo};
    auto result = target.GetTeam("T1");

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->Name, "Delta");
}

// Caso 4: Búsqueda individual fallida
TEST(TeamDelegateSpec, FindTeamById_NotFound_Nullptr) {
    auto mockRepo = std::make_shared<StrictMock<MockTeamRepository>>();
    EXPECT_CALL(*mockRepo, ReadById("404"sv)).WillOnce(Return(nullptr));

    TeamDelegate target{mockRepo};
    EXPECT_EQ(target.GetTeam("404"), nullptr);
}

// Caso 5: Consulta global con lista vacía
TEST(TeamDelegateSpec, FetchAll_NoRecords_ReturnsEmptyVector) {
    auto mockRepo = std::make_shared<NiceMock<MockTeamRepository>>();
    EXPECT_CALL(*mockRepo, ReadAll())
        .WillOnce(Return(std::vector<std::shared_ptr<domain::Team>>{}));

    TeamDelegate target{mockRepo};
    auto res = target.GetAllTeams();

    EXPECT_TRUE(res.empty());
}

// Caso 6: Consulta global con elementos existentes
TEST(TeamDelegateSpec, FetchAll_WithTeams_ReturnsExpectedList) {
    auto mockRepo = std::make_shared<StrictMock<MockTeamRepository>>();
    std::vector<std::shared_ptr<domain::Team>> dummyList{
        fakeTeam("A1", "Alpha"),
        fakeTeam("B1", "Bravo")
    };

    EXPECT_CALL(*mockRepo, ReadAll()).WillOnce(Return(dummyList));

    TeamDelegate target{mockRepo};
    auto res = target.GetAllTeams();

    ASSERT_EQ(res.size(), 2u);
    EXPECT_EQ(res[0]->Name, "Alpha");
    EXPECT_EQ(res[1]->Name, "Bravo");
}

// Caso 7: Actualización exitosa (redefinido)
TEST(TeamDelegateSpec, Update_ExistingRecord_ReturnsTrue) {
    auto mockRepo = std::make_shared<StrictMock<MockTeamRepository>>();
    EXPECT_CALL(*mockRepo, ReadById("U10"sv)).WillOnce(Return(fakeTeam("U10", "Viejo")));
    EXPECT_CALL(*mockRepo, Update(::testing::_)).WillOnce(Return("U10"sv));

    TeamDelegate target{mockRepo};
    domain::Team updated{"U10", "NuevoNombre"};

    // En nuestro diseño UpdateTeam podría no retornar valor, se asume éxito si no lanza excepción
    target.UpdateTeam("U10", updated);
    SUCCEED();  // marca que la llamada completó sin error
}

// Caso 8: Actualización con ID inexistente
TEST(TeamDelegateSpec, Update_NonexistentId_NoUpdateCall) {
    auto mockRepo = std::make_shared<StrictMock<MockTeamRepository>>();
    EXPECT_CALL(*mockRepo, ReadById("X0"sv)).WillOnce(Return(nullptr));
    EXPECT_CALL(*mockRepo, Update(::testing::_)).Times(0);

    TeamDelegate target{mockRepo};
    domain::Team dummy{"X0", "Nada"};

    // En este flujo no debe intentar actualizar
    target.UpdateTeam("X0", dummy);
    SUCCEED();
}


// Requisito #8 CORREGIDO: Actualización exitosa
TEST(TeamDelegateSpec, Update_ExistingRecord_Successful) {
    auto mockRepo = std::make_shared<StrictMock<MockTeamRepository>>();
    // NO espera ReadById, solo Update
    EXPECT_CALL(*mockRepo, Update(::testing::_))
        .WillOnce(Return("U10"sv));

    TeamDelegate target{mockRepo};
    domain::Team updated{"U10", "NuevoNombre"};

    target.UpdateTeam("U10", updated);
    SUCCEED();
}

// Requisito #9 CORREGIDO: Actualización con ID inexistente
TEST(TeamDelegateSpec, Update_NonexistentId_ThrowsException) {
    auto mockRepo = std::make_shared<StrictMock<MockTeamRepository>>();
    EXPECT_CALL(*mockRepo, Update(::testing::_))
        .WillOnce(testing::Throw(std::runtime_error("Team not found")));

    TeamDelegate target{mockRepo};
    domain::Team dummy{"X0", "Nada"};

    EXPECT_THROW(target.UpdateTeam("X0", dummy), std::runtime_error);
}
// Caso 10: Eliminación con ID no encontrado
TEST(TeamDelegateSpec, DeleteTeam_NotFound_NoAction) {
    auto mockRepo = std::make_shared<StrictMock<MockTeamRepository>>();
    EXPECT_CALL(*mockRepo, ReadById("NF"sv)).WillOnce(Return(nullptr));
    EXPECT_CALL(*mockRepo, Delete(::testing::_)).Times(0);

    TeamDelegate target{mockRepo};
    target.DeleteTeam("NF");
    SUCCEED();
}