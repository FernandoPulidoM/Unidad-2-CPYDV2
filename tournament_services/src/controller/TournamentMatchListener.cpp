//
// Created by fmendivil on 10/22/25.
//
#include "../../../tournament_common/include/domain/TournamentMatchListener.hpp"
#include <iostream>
#include <regex>
#include <iomanip>  // ← AGREGAR ESTO para std::setw

TournamentMatchListener::TournamentMatchListener(
    std::shared_ptr<QueueMessageConsumer> consumer
) : consumer(std::move(consumer)) {}

void TournamentMatchListener::Start() {
    std::cout << "[LISTENER] Starting Tournament Match Listener..." << std::endl;

    consumer->SetMessageHandler([this](const std::string& message) {
        std::cout << "[LISTENER] Received message: " << message << std::endl;

        // Parsear tipo de mensaje
        if (message.find("match.completed.winner") != std::string::npos) {
            OnMatchCompleted(message);
        } else if (message.find("bracket.generated") != std::string::npos) {
            OnBracketGenerated(message);
        }
    });



    consumer->Start("tournament.match.events");
}

void TournamentMatchListener::OnMatchCompleted(const std::string& message) {
    // Formato: "match.completed.winner.{winnerId}"
    std::regex pattern(R"(match\.completed\.winner\.([a-zA-Z0-9\-]+))");
    std::smatch matches;

    if (std::regex_search(message, matches, pattern)) {
        std::string winnerId = matches[1];

        std::cout << "╔═══════════════════════════════════════╗" << std::endl;
        std::cout << "║      MATCH COMPLETED                  ║" << std::endl;
        std::cout << "╠═══════════════════════════════════════╣" << std::endl;
        std::cout << "║ Winner: " << std::left << std::setw(28) << winnerId << "║" << std::endl;
        std::cout << "╚═══════════════════════════════════════╝" << std::endl;

        // Aquí podrías:
        // - Enviar notificaciones
        // - Actualizar estadísticas
        // - Registrar en logs
    }
}

void TournamentMatchListener::OnBracketGenerated(const std::string& message) {
    // Formato: "bracket.generated.{numMatches}.matches"
    std::regex pattern(R"(bracket\.generated\.(\d+)\.matches)");
    std::smatch matches;

    if (std::regex_search(message, matches, pattern)) {
        int numMatches = std::stoi(matches[1]);

        std::cout << "╔═══════════════════════════════════════╗" << std::endl;
        std::cout << "║   TOURNAMENT BRACKET GENERATED        ║" << std::endl;
        std::cout << "╠═══════════════════════════════════════╣" << std::endl;
        std::cout << "║ Total Matches: " << std::left << std::setw(20) << numMatches << "║" << std::endl;
        std::cout << "║                                       ║" << std::endl;
        std::cout << "║ Structure:                            ║" << std::endl;
        std::cout << "║   • Quarter Finals: 4 matches         ║" << std::endl;
        std::cout << "║   • Semi Finals: 2 matches            ║" << std::endl;
        std::cout << "║   • Final: 1 match                    ║" << std::endl;
        std::cout << "║   • Third Place: 1 match              ║" << std::endl;
        std::cout << "╚═══════════════════════════════════════╝" << std::endl;
    }
}
