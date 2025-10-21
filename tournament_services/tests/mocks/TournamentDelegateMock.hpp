#pragma once
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>

#include "delegate/ITournamentDelegate.hpp"
#include "domain/Tournament.hpp"

// Mock alineado a tu interfaz ACTUAL (no usa std::expected)
class TournamentDelegateMock : public ITournamentDelegate {
public:
    MOCK_METHOD(std::string, CreateTournament, (std::shared_ptr<domain::Tournament>), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
    // GET por id: agrega este metodo si existe en tu interfaz real
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, ReadById, (const std::string&), ());
    MOCK_METHOD(void, UpdateTournament, (const std::string&, std::shared_ptr<domain::Tournament>), (override));
    MOCK_METHOD(void, DeleteTournament, (const std::string&), (override));
};
