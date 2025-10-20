#include "delegate/TeamDelegate.hpp"

TeamDelegate::TeamDelegate(std::shared_ptr<IRepository<domain::Team, std::string>> repository,
                           std::shared_ptr<QueueMessageProducer> prod)
    : teamRepository(std::move(repository)), producer(std::move(prod)) {}

std::string TeamDelegate::CreateTeam(std::shared_ptr<domain::Team> team) {
    std::string id = teamRepository->Create(*team);
    producer->SendMessage(id, "team.created");
    return id;
}

std::vector<std::shared_ptr<domain::Team>> TeamDelegate::ReadAll() {
    return teamRepository->ReadAll();
}

void TeamDelegate::UpdateTeam(const std::string& id, std::shared_ptr<domain::Team> team) {
    std::string updatedId = teamRepository->Update(*team);
    producer->SendMessage(updatedId, "team.updated");
}

void TeamDelegate::DeleteTeam(const std::string& id) {
    teamRepository->Delete(id);
    producer->SendMessage(id, "team.deleted");
}
