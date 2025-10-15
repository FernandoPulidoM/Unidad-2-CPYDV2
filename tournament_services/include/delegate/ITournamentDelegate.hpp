//
// Created by tsuny on 8/31/25.
//

#ifndef TOURNAMENTS_ITOURNAMENTDELEGATE_HPP
#define TOURNAMENTS_ITOURNAMENTDELEGATE_HPP

#include <string>
#include <memory>

#include "domain/Tournament.hpp"

class ITournamentDelegate {
public:
    virtual ~ITournamentDelegate() = default;
    virtual std::string CreateTournament(std::shared_ptr<domain::Tournament> tournament) = 0;
    virtual std::vector<std::shared_ptr<domain::Tournament>> ReadAll() = 0;

    // En la interfaz:
    virtual void UpdateTournament(const std::string& id, std::shared_ptr<domain::Tournament> tournament) = 0;

    // Agregar en la interfaz ITournamentDelegate:
    virtual void DeleteTournament(const std::string& id) = 0;
};

#endif //TOURNAMENTS_ITOURNAMENTDELEGATE_HPP