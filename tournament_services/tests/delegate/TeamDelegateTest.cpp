//
// Created by fmendivil on 10/20/25.
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

// #include "mocks/TeamRepositoryMock.h"
// #include "domain/Team.hpp"
// // #include "delegate/TeamDelegate.hpp"
#include "/home/fmendivil/tournaments/tournament_services/include/delegate/TeamDelegate.hpp"
#include "/home/fmendivil/tournaments/tournament_services/tests/mocks/TeamRepositoryMock.h"
#include "/home/fmendivil/tournaments/tournament_common/include/domain/Team.hpp"


using ::testing::NiceMock;
using ::testing::StrictMock;
using ::testing::Return;
using ::testing::InSequence;
using ::testing::An;
using namespace std::literals;

// Helper para crear objetos Team simulados
static std::shared_ptr<domain::Team> mkT(std::string id, std::string name){
  return std::make_shared<domain::Team>(domain::Team{std::move(id), std::move(name)});
}

/*
   Al método que procesa la creación de equipo, validar que el valor que
   se le transfiera a TeamRepository es el esperado.
   Simular una inserción válida y que la respuesta de esta función
   sea el ID generado.
    */
TEST(TeamDelegateTest, SaveTeam_ReturnsIdView){
  auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
  EXPECT_CALL(*repo, Create(::testing::_)).WillOnce(Return("id-1"sv));
  TeamDelegate sut{repo};
  domain::Team in{"","Nuevo"};
  EXPECT_EQ(sut.SaveTeam(in), "id-1");
}

/*
   Al método que procesa la creación de equipo, validar que el valor que
   se le transfiera a TeamRepository es el esperado.
   Simular una inserción fallida y que la respuesta de esta función
   sea un error o mensaje usando std::expected.
    */
TEST(TeamDelegateTest, SaveTeam_RepoReturnsEmpty_ReturnsEmptyView) {
    auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
    EXPECT_CALL(*repo, Create(::testing::_))
        .WillOnce(Return(""sv));
    TeamDelegate sut{repo};
    domain::Team in{"", "New"};
    auto result = sut.SaveTeam(in);
    EXPECT_TRUE(result.empty());
}


/*
   Al método que procesa la búsqueda de un equipo por ID, validar que
   el valor que se le transfiera a TeamRepository es el esperado.
   Simular el resultado con un objeto y validar el objeto.
    */
TEST(TeamDelegateTest, GetTeam_UsesReadByIdView){
  auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
  EXPECT_CALL(*repo, ReadById("X"sv)).WillOnce(Return(mkT("X","XName")));
  TeamDelegate sut{repo};
  auto t = sut.GetTeam("X");
  ASSERT_NE(t,nullptr);
  EXPECT_EQ(t->Name,"XName");
}

/*
   Al método que procesa la búsqueda de un equipo por ID, validar que
   el valor que se le transfiera a TeamRepository es el esperado.
   Simular el resultado nulo y validar nullptr.
    */
TEST(TeamDelegateTest, GetTeam_NotFound_ReturnsNull) {
  auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
  EXPECT_CALL(*repo, ReadById("X"sv)).WillOnce(Return(nullptr));
  TeamDelegate sut{repo};
  auto t = sut.GetTeam("X");
  EXPECT_EQ(t, nullptr);
}

/*
   Al método que procesa la búsqueda de equipos.
   Simular el resultado con una lista vacía de TeamRepository.
    */
TEST(TeamDelegateTest, GetAll_Empty){
  auto repo = std::make_shared<NiceMock<MockTeamRepository>>();
  EXPECT_CALL(*repo, ReadAll()).WillOnce(Return(std::vector<std::shared_ptr<domain::Team>>{}));
  TeamDelegate sut{repo};
  EXPECT_TRUE(sut.GetAllTeams().empty());
}

/*
   Al método que procesa la búsqueda de equipos.
   Simular el resultado con una lista de objetos de TeamRepository.
    */
TEST(TeamDelegateTest, GetAll_WithItems_ReturnsList) {
  auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
  std::vector<std::shared_ptr<domain::Team>> data{
      mkT("A", "Alpha"), mkT("B", "Beta")
  };
  EXPECT_CALL(*repo, ReadAll()).WillOnce(Return(data));
  TeamDelegate sut{repo};
  auto res = sut.GetAllTeams();
  ASSERT_EQ(res.size(), 2u);
  EXPECT_EQ(res[0]->Name, "Alpha");
}

// /*
//    Al método que procesa la actualización de un equipo, validar la
//    búsqueda de TeamRepository por ID, validar el valor transferido a Update.
//    Simular resultado exitoso.
//     */
// TEST(TeamDelegateTest, Update_Found_CallsUpdateAndTrue){
//   auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
//   EXPECT_CALL(*repo, ReadById("U2"sv)).WillOnce(Return(mkT("U2","Old")));
//   EXPECT_CALL(*repo, Update(::testing::_)).WillOnce(Return("U2"sv));
//   TeamDelegate sut{repo};
//   domain::Team in{"U2","New"};
//   EXPECT_TRUE(sut.UpdateTeam("U2", in));
// }

// /*
//    Al método que procesa la actualización de un equipo, validar la búsqueda
//    de TeamRepository por ID. Simular resultado de búsqueda no exitoso y
//    regresar error o mensaje usando std::expected.
//     */
// TEST(TeamDelegateTest, Update_NotFound_ReturnsFalse){
//   auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
//   EXPECT_CALL(*repo, ReadById(An<std::string_view>())).WillOnce(Return(nullptr));
//   EXPECT_CALL(*repo, Update(::testing::_)).Times(0);
//   TeamDelegate sut{repo};
//   domain::Team in{"U1","Name"};
//   EXPECT_FALSE(sut.UpdateTeam("U1", in));
// }

// /*
//    Caso adicional, al método que procesa la eliminación de un equipo,
//    validar que si no se encuentra el registro, el repositorio no se invoque
//    para eliminar y se regrese false.
//     */
// TEST(TeamDelegateTest, Delete_NotFound_ReturnsFalse){
//   auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
//   EXPECT_CALL(*repo, ReadById("D1"sv)).WillOnce(Return(nullptr));
//   EXPECT_CALL(*repo, Delete(An<std::string_view>())).Times(0);
//   TeamDelegate sut{repo};
//   EXPECT_FALSE(sut.DeleteTeam("D1"));
// }

// /*
//    Caso adicional, al método que procesa la eliminación de un equipo,
//    validar la búsqueda por ID y la eliminación posterior.
//    Simular resultado exitoso.
//     */
// TEST(TeamDelegateTest, Delete_Success_ReturnsTrue){
//   auto repo = std::make_shared<StrictMock<MockTeamRepository>>();
//   TeamDelegate sut{repo};
//   InSequence seq;
//   EXPECT_CALL(*repo, ReadById("D2"sv)).WillOnce(Return(mkT("D2","ToDelete")));
//   EXPECT_CALL(*repo, Delete("D2"sv)).Times(1);
//   EXPECT_CALL(*repo, ReadById("D2"sv)).WillOnce(Return(nullptr));
//   EXPECT_TRUE(sut.DeleteTeam("D2"));
// }

// GetAllTeams: pass-through
TEST(TeamDelegateTest, GetAllTeams_ReturnsRepoVector) {
  auto repo = std::make_shared<StrictMock<TeamRepositoryMock>>();
  EXPECT_CALL(*repo, ReadAll())
      .WillOnce(Return(std::vector<std::shared_ptr<domain::Team>>{
        std::make_shared<domain::Team>(domain::Team{"A","Alpha"}),
        std::make_shared<domain::Team>(domain::Team{"B","Beta"})
      }));

  TeamDelegate sut{repo};
  auto v = sut.GetAllTeams();
  ASSERT_EQ(v.size(), 2u);
  EXPECT_EQ(v[0]->Id, "A");
}

// GetTeam: found and null
TEST(TeamDelegateTest, GetTeam_FoundAndNull) {
  auto repo = std::make_shared<StrictMock<TeamRepositoryMock>>();
  EXPECT_CALL(*repo, ReadById("X"sv)).WillOnce(Return(std::make_shared<domain::Team>(domain::Team{"X","One"})));
  EXPECT_CALL(*repo, ReadById("Z"sv)).WillOnce(Return(nullptr));

  TeamDelegate sut{repo};
  EXPECT_NE(sut.GetTeam("X"), nullptr);
  EXPECT_EQ(sut.GetTeam("Z"), nullptr);
}

// SaveTeam: returns created id
TEST(TeamDelegateTest, SaveTeam_ReturnsId) {
  auto repo = std::make_shared<StrictMock<TeamRepositoryMock>>();
  EXPECT_CALL(*repo, Create(::testing::_))
      .WillOnce(Return("GEN-1"sv));

  TeamDelegate sut{repo};
  domain::Team t{"N1","Name"};
  auto id = sut.SaveTeam(t);
  EXPECT_EQ(id, "GEN-1");
}
//
// // UpdateTeam: not found -> false
// TEST(TeamDelegateTest, UpdateTeam_NotFound_ReturnsFalse) {
//   auto repo = std::make_shared<StrictMock<TeamRepositoryMock>>();
//   EXPECT_CALL(*repo, ReadById(::testing::_)).WillOnce(Return(nullptr));
//
//   TeamDelegate sut{repo};
//   domain::Team incoming{"IGN","NewName"};
//   EXPECT_FALSE(sut.UpdateTeam("U1", incoming));
// }
//
// // UpdateTeam: found -> Update called and true
// TEST(TeamDelegateTest, UpdateTeam_Found_UpdatesAndReturnsTrue) {
//   auto repo = std::make_shared<StrictMock<TeamRepositoryMock>>();
//   EXPECT_CALL(*repo, ReadById(::testing::_)).WillOnce(Return(std::make_shared<domain::Team>(domain::Team{"U2","Old"})));
//   EXPECT_CALL(*repo, Update(::testing::_)).Times(1);
//
//   TeamDelegate sut{repo};
//   domain::Team incoming{"IGN","New"};
//   EXPECT_TRUE(sut.UpdateTeam("U2", incoming));
// }
//
// // DeleteTeam: not found -> false
// TEST(TeamDelegateTest, DeleteTeam_NotFound_ReturnsFalse) {
//   auto repo = std::make_shared<StrictMock<TeamRepositoryMock>>();
//   EXPECT_CALL(*repo, ReadById("D0"sv)).WillOnce(Return(nullptr));
//
//   TeamDelegate sut{repo};
//   EXPECT_FALSE(sut.DeleteTeam("D0"));
// }

// // DeleteTeam: success -> true (post-check null)
// TEST(TeamDelegateTest, DeleteTeam_Success_ReturnsTrue) {
//   auto repo = std::make_shared<StrictMock<TeamRepositoryMock>>();
//   {
//     ::testing::InSequence seq;
//     EXPECT_CALL(*repo, ReadById("D1"sv)).WillOnce(Return(std::make_shared<domain::Team>(domain::Team{"D1","X"})));
//     EXPECT_CALL(*repo, Delete("D1"sv)).Times(1);
//     EXPECT_CALL(*repo, ReadById("D1"sv)).WillOnce(Return(nullptr));
//   }
//
//   TeamDelegate sut{repo};
//   EXPECT_TRUE(sut.DeleteTeam("D1"));
// }
//
// // DeleteTeam: deletion did not remove entity -> false
// TEST(TeamDelegateTest, DeleteTeam_PostCheckStillExists_ReturnsFalse) {
//   auto repo = std::make_shared<StrictMock<TeamRepositoryMock>>();
//   auto stillThere = std::make_shared<domain::Team>(domain::Team{"D2","Y"});
//   {
//     ::testing::InSequence seq;
//     EXPECT_CALL(*repo, ReadById("D2"sv)).WillOnce(Return(stillThere));
//     EXPECT_CALL(*repo, Delete("D2"sv)).Times(1);
//     EXPECT_CALL(*repo, ReadById("D2"sv)).WillOnce(Return(stillThere));
//   }
//
//   TeamDelegate sut{repo};
//   EXPECT_FALSE(sut.DeleteTeam("D2"));
// }