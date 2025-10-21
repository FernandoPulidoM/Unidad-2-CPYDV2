//
// Created by tomas on 8/31/25.
//

#include <string_view>
#include <memory>
#include <utility>

#include "delegate/TournamentDelegate.hpp"
#include "persistence/repository/IRepository.hpp"
#include "cms/QueueMessageProducer.hpp"
#include "domain/Tournament.hpp"

TournamentDelegate::TournamentDelegate(
    std::shared_ptr<IRepository<domain::Tournament, std::string>> repository,
    std::shared_ptr<QueueMessageProducer> producer)
    : tournamentRepository(std::move(repository)), producer(std::move(producer)) {}

std::string TournamentDelegate::CreateTournament(std::shared_ptr<domain::Tournament> tournament) {
    try {
        // Materializa grupos según el formato (lo exige el test: size == NumberOfGroups)
        if (tournament) {
            const auto fmt = tournament->Format();
            const int ng = fmt.NumberOfGroups();
            if (ng > 0 && tournament->Groups().size() != static_cast<size_t>(ng)) {
                tournament->Groups().resize(static_cast<size_t>(ng));
            }
        }

        std::string id = tournamentRepository->Create(*tournament);

        if (!id.empty() && producer) {
            producer->SendMessage(id, "tournament.created");
        }
        return id;

    } catch (const std::exception&) {
        // Contrato que esperan los tests: en error devuelve "", no propaga
        return std::string{};
    }
}

std::vector<std::shared_ptr<domain::Tournament>> TournamentDelegate::ReadAll() {
    return tournamentRepository->ReadAll();
}

void TournamentDelegate::DeleteTournament(const std::string& id) {
    tournamentRepository->Delete(id);
    if (producer) producer->SendMessage(id, "tournament.deleted");
}

void TournamentDelegate::UpdateTournament(const std::string& id, std::shared_ptr<domain::Tournament> tournament) {
    (void)id; // no lo usamos directamente porque el repo retorna el id
    std::string updatedId = tournamentRepository->Update(*tournament);
    if (!updatedId.empty() && producer) {
        producer->SendMessage(updatedId, "tournament.updated");
    }
}
