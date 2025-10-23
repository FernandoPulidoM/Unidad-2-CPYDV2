//
// Created by tomas on 8/22/25.
//

#include "delegate/TeamDelegate.hpp"

#include <utility>

TeamDelegate::TeamDelegate(std::shared_ptr<IRepository<domain::Team, std::string_view> > repository)
    : teamRepository(std::move(repository)) {
}

std::vector<std::shared_ptr<domain::Team>> TeamDelegate::GetAllTeams() {
    return teamRepository->ReadAll();
}

std::shared_ptr<domain::Team> TeamDelegate::GetTeam(std::string_view id) {
    return teamRepository->ReadById(id);
}

std::string_view TeamDelegate::SaveTeam(const domain::Team& team){
    return teamRepository->Create(team);
}

// Update: primero verifica existencia; si no existe, NO llama Update.
// Si el repo lanza (p.ej. "Team not found"), dejamos propagar para que el controller mapee 404.
void TeamDelegate::UpdateTeam(std::string_view id, const domain::Team& team) {
    auto current = teamRepository->ReadById(id);
    if (!current) {
        return; // cumple con el test "NoUpdateCall"
    }
    teamRepository->Update(team);
}

void TeamDelegate::DeleteTeam(std::string_view id) {
    auto current = teamRepository->ReadById(id);
    if (!current) {
        return; // no llamamos Delete si no existe (test NotFound_NoAction)
    }
    teamRepository->Delete(id);
}
