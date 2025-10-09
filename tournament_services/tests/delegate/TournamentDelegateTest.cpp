//
// Created by tomas on 8/31/25.
//Version prime
#include "delegate/TournamentDelegate.hpp"

#include <string_view>
#include <memory>
#include <format>

#include "persistence/repository/IRepository.hpp"
#include "../../include/cms/QueueMessageProducer.hpp"

TournamentDelegate::TournamentDelegate(
    std::shared_ptr<IRepository<domain::Tournament, std::string>> repository,
    std::shared_ptr<QueueMessageProducer> producer
) : tournamentRepository(std::move(repository)),
    producer(std::move(producer)) {
}

std::string TournamentDelegate::CreateTournament(std::shared_ptr<domain::Tournament> tournament) {
    if (!tournament) {
        return "";
    }

    // Fill groups according to max groups
    for (int i = 0; i < tournament->Format().NumberOfGroups(); i++) {
        char groupLetter = 'A' + i;
        tournament->Groups().push_back(
            domain::Group{std::format("Tournament {}", groupLetter)}
        );
    }

    std::string id = tournamentRepository->Create(*tournament);

    // Send message to queue about tournament creation
    producer->SendMessage(id, "tournament.created");

    // TODO: If groups are completed, also create matches

    return id;
}

std::vector<std::shared_ptr<domain::Tournament>> TournamentDelegate::ReadAll() {
    return tournamentRepository->ReadAll();
}

// bool TournamentDelegate::UpdateTournament(const domain::Tournament& tournament) {
//     return tournamentRepository->Update(tournament);
// }
//
// bool TournamentDelegate::DeleteTournament(const std::string_view id) {
//     bool deleted = tournamentRepository->Delete(std::string(id));
//
//     if (deleted) {
//         producer->SendMessage(std::string(id), "tournament.deleted");
//     }
//
//     return deleted;
// }