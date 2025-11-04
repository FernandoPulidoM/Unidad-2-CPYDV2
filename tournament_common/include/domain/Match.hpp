#ifndef DOMAIN_MATCH_HPP
#define DOMAIN_MATCH_HPP

#include <string>
#include <optional>
#include "Team.hpp"

namespace domain {
    enum class MatchStatus {
        PENDING,      // No ha comenzado
        IN_PROGRESS,  // En juego
        COMPLETED,    // Terminado
        CANCELLED     // Cancelado
    };

    enum class TournamentPhase {
        QUARTER_FINALS,  // Cuartos de final (8 equipos)
        SEMI_FINALS,     // Semifinales (4 equipos)
        FINAL,           // Final (2 equipos)
        THIRD_PLACE      // Tercer lugar
    };

    class Match {
        std::string id;
        std::string tournamentId;
        TournamentPhase phase;

        std::optional<Team> team1;
        std::optional<Team> team2;

        int scoreTeam1;
        int scoreTeam2;

        MatchStatus status;
        std::optional<std::string> winnerId;
        std::optional<std::string> nextMatchId;  // Para eliminación

    public:
        Match() : scoreTeam1(0), scoreTeam2(0), status(MatchStatus::PENDING) {}

        Match(std::string tournamentId, TournamentPhase phase)
            : tournamentId(std::move(tournamentId))
            , phase(phase)
            , scoreTeam1(0)
            , scoreTeam2(0)
            , status(MatchStatus::PENDING) {}

        // Getters
        [[nodiscard]] std::string Id() const { return id; }
        [[nodiscard]] std::string TournamentId() const { return tournamentId; }
        [[nodiscard]] TournamentPhase Phase() const { return phase; }
        [[nodiscard]] std::optional<Team> Team1() const { return team1; }
        [[nodiscard]] std::optional<Team> Team2() const { return team2; }
        [[nodiscard]] int ScoreTeam1() const { return scoreTeam1; }
        [[nodiscard]] int ScoreTeam2() const { return scoreTeam2; }
        [[nodiscard]] MatchStatus Status() const { return status; }
        [[nodiscard]] std::optional<std::string> WinnerId() const { return winnerId; }
        [[nodiscard]] std::optional<std::string> NextMatchId() const { return nextMatchId; }

        // Setters
        std::string& Id() { return id; }
        std::string& TournamentId() { return tournamentId; }
        TournamentPhase& Phase() { return phase; }
        std::optional<Team>& Team1() { return team1; }
        std::optional<Team>& Team2() { return team2; }
        int& ScoreTeam1() { return scoreTeam1; }
        int& ScoreTeam2() { return scoreTeam2; }
        MatchStatus& Status() { return status; }
        std::optional<std::string>& WinnerId() { return winnerId; }
        std::optional<std::string>& NextMatchId() { return nextMatchId; }

        // Métodos de negocio
        void SetScore(int score1, int score2) {
            scoreTeam1 = score1;
            scoreTeam2 = score2;

            if (status == MatchStatus::PENDING) {
                status = MatchStatus::IN_PROGRESS;
            }
        }

        void CompleteMatch() {
            if (!team1 || !team2) {
                throw std::runtime_error("Cannot complete match without both teams");
            }

            if (scoreTeam1 == scoreTeam2) {
                throw std::runtime_error("Match ended in a tie - penalty shootout required");
            }

            status = MatchStatus::COMPLETED;
            winnerId = (scoreTeam1 > scoreTeam2) ? team1->Id : team2->Id;
        }

        std::optional<Team> GetWinner() const {
            if (status != MatchStatus::COMPLETED || !winnerId) {
                return std::nullopt;
            }

            if (team1 && team1->Id == *winnerId) return team1;
            if (team2 && team2->Id == *winnerId) return team2;

            return std::nullopt;
        }

        std::optional<Team> GetLoser() const {
            if (status != MatchStatus::COMPLETED || !winnerId) {
                return std::nullopt;
            }

            if (team1 && team1->Id != *winnerId) return team1;
            if (team2 && team2->Id != *winnerId) return team2;

            return std::nullopt;
        }
    };

    // Helpers para convertir enums a string
    inline std::string phaseToString(TournamentPhase phase) {
        switch(phase) {
            case TournamentPhase::QUARTER_FINALS: return "QUARTER_FINALS";
            case TournamentPhase::SEMI_FINALS: return "SEMI_FINALS";
            case TournamentPhase::FINAL: return "FINAL";
            case TournamentPhase::THIRD_PLACE: return "THIRD_PLACE";
            default: return "UNKNOWN";
        }
    }

    inline std::string statusToString(MatchStatus status) {
        switch(status) {
            case MatchStatus::PENDING: return "PENDING";
            case MatchStatus::IN_PROGRESS: return "IN_PROGRESS";
            case MatchStatus::COMPLETED: return "COMPLETED";
            case MatchStatus::CANCELLED: return "CANCELLED";
            default: return "UNKNOWN";
        }
    }
}

#endif