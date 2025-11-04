#include "delegate/MatchDelegate.hpp"
#include <algorithm>
#include <format>

MatchDelegate::MatchDelegate(
    std::shared_ptr<IRepository<domain::Match, std::string>> repository,
    std::shared_ptr<QueueMessageProducer> producer
) : matchRepository(std::move(repository))
  , producer(std::move(producer)) {}

std::expected<std::vector<std::string>, std::string> 
MatchDelegate::GenerateSingleEliminationBracket(
    const std::string_view& tournamentId,
    const std::vector<domain::Team>& teams
) {
    // Validar que sean exactamente 8 equipos
    if (teams.size() != 8) {
        return std::unexpected("Single elimination requires exactly 8 teams");
    }
    
    std::vector<std::string> matchIds;
    std::map<std::string, std::string> nextMatchMapping;
    
    try {
        // === FASE 1: CUARTOS DE FINAL (4 partidos) ===
        std::vector<std::shared_ptr<domain::Match>> quarterMatches;
        
        for (int i = 0; i < 4; i++) {
            auto match = std::make_shared<domain::Match>(
                std::string(tournamentId),
                domain::TournamentPhase::QUARTER_FINALS
            );
            
            match->Team1() = teams[i * 2];
            match->Team2() = teams[i * 2 + 1];
            
            std::string matchId = matchRepository->Create(*match);
            match->Id() = matchId;
            matchIds.push_back(matchId);
            quarterMatches.push_back(match);
        }
        
        // === FASE 2: SEMIFINALES (2 partidos) ===
        std::vector<std::shared_ptr<domain::Match>> semiMatches;
        
        for (int i = 0; i < 2; i++) {
            auto match = std::make_shared<domain::Match>(
                std::string(tournamentId),
                domain::TournamentPhase::SEMI_FINALS
            );
            
            std::string matchId = matchRepository->Create(*match);
            match->Id() = matchId;
            matchIds.push_back(matchId);
            semiMatches.push_back(match);
            
            // Conectar cuartos → semifinales
            quarterMatches[i * 2]->NextMatchId() = matchId;
            quarterMatches[i * 2 + 1]->NextMatchId() = matchId;
            
            matchRepository->Update(*quarterMatches[i * 2]);
            matchRepository->Update(*quarterMatches[i * 2 + 1]);
        }
        
        // === FASE 3: FINAL ===
        auto finalMatch = std::make_shared<domain::Match>(
            std::string(tournamentId),
            domain::TournamentPhase::FINAL
        );
        
        std::string finalId = matchRepository->Create(*finalMatch);
        finalMatch->Id() = finalId;
        matchIds.push_back(finalId);
        
        // Conectar semifinales → final
        semiMatches[0]->NextMatchId() = finalId;
        semiMatches[1]->NextMatchId() = finalId;
        
        matchRepository->Update(*semiMatches[0]);
        matchRepository->Update(*semiMatches[1]);
        
        // === FASE 4: TERCER LUGAR ===
        auto thirdPlaceMatch = std::make_shared<domain::Match>(
            std::string(tournamentId),
            domain::TournamentPhase::THIRD_PLACE
        );
        
        std::string thirdPlaceId = matchRepository->Create(*thirdPlaceMatch);
        matchIds.push_back(thirdPlaceId);
        
        // Enviar evento
        producer->SendMessage(
            std::string(tournamentId),
            std::format("bracket.generated.{}.matches", matchIds.size())
        );
        
        return matchIds;
        
    } catch (const std::exception& e) {
        return std::unexpected(std::string("Error generating bracket: ") + e.what());
    }
}

std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>
MatchDelegate::GetMatchesByTournament(const std::string_view& tournamentId) {
    try {
        // Asume que MatchRepository tiene método FindByTournamentId
        auto matches = matchRepository->ReadAll();
        
        std::vector<std::shared_ptr<domain::Match>> filtered;
        for (auto& match : matches) {
            if (match->TournamentId() == tournamentId) {
                filtered.push_back(match);
            }
        }
        
        return filtered;
    } catch (const std::exception& e) {
        return std::unexpected(std::string(e.what()));
    }
}

std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>
MatchDelegate::GetMatchesByPhase(
    const std::string_view& tournamentId,
    domain::TournamentPhase phase
) {
    auto allMatches = GetMatchesByTournament(tournamentId);
    if (!allMatches) {
        return std::unexpected(allMatches.error());
    }
    
    std::vector<std::shared_ptr<domain::Match>> filtered;
    for (auto& match : *allMatches) {
        if (match->Phase() == phase) {
            filtered.push_back(match);
        }
    }
    
    return filtered;
}

std::expected<std::shared_ptr<domain::Match>, std::string>
MatchDelegate::GetMatch(const std::string_view& matchId) {
    try {
        auto match = matchRepository->ReadById(std::string(matchId));
        if (!match) {
            return std::unexpected("Match not found");
        }
        return match;
    } catch (const std::exception& e) {
        return std::unexpected(std::string(e.what()));
    }
}

std::expected<void, std::string>
MatchDelegate::UpdateScore(
    const std::string_view& matchId,
    int scoreTeam1,
    int scoreTeam2
) {
    try {
        auto matchResult = GetMatch(matchId);
        if (!matchResult) {
            return std::unexpected(matchResult.error());
        }
        
        auto match = *matchResult;
        
        if (match->Status() == domain::MatchStatus::COMPLETED) {
            return std::unexpected("Cannot update score of completed match");
        }
        
        match->SetScore(scoreTeam1, scoreTeam2);
        matchRepository->Update(*match);
        
        producer->SendMessage(
            std::string(matchId),
            std::format("match.score.updated.{}-{}", scoreTeam1, scoreTeam2)
        );
        
        return {};
    } catch (const std::exception& e) {
        return std::unexpected(std::string(e.what()));
    }
}

std::expected<void, std::string>
MatchDelegate::CompleteMatch(const std::string_view& matchId) {
    try {
        auto matchResult = GetMatch(matchId);
        if (!matchResult) {
            return std::unexpected(matchResult.error());
        }
        
        auto match = *matchResult;
        
        // Completar partido (determina ganador)
        match->CompleteMatch();
        matchRepository->Update(*match);
        
        // Si hay siguiente partido, avanzar al ganador
        if (match->NextMatchId()) {
            auto nextMatchResult = GetMatch(*match->NextMatchId());
            if (nextMatchResult) {
                auto nextMatch = *nextMatchResult;
                auto winner = match->GetWinner();
                
                if (winner) {
                    // Asignar ganador al siguiente partido
                    if (!nextMatch->Team1()) {
                        nextMatch->Team1() = *winner;
                    } else if (!nextMatch->Team2()) {
                        nextMatch->Team2() = *winner;
                    }
                    
                    matchRepository->Update(*nextMatch);
                }
            }
        }
        
        // Si es semifinal, avanzar perdedor a tercer lugar
        if (match->Phase() == domain::TournamentPhase::SEMI_FINALS) {
            auto loser = match->GetLoser();
            if (loser) {
                auto thirdPlaceMatches = GetMatchesByPhase(
                    match->TournamentId(),
                    domain::TournamentPhase::THIRD_PLACE
                );
                
                if (thirdPlaceMatches && !thirdPlaceMatches->empty()) {
                    auto thirdPlace = (*thirdPlaceMatches)[0];
                    
                    if (!thirdPlace->Team1()) {
                        thirdPlace->Team1() = *loser;
                    } else if (!thirdPlace->Team2()) {
                        thirdPlace->Team2() = *loser;
                    }
                    
                    matchRepository->Update(*thirdPlace);
                }
            }
        }
        
        producer->SendMessage(
            std::string(matchId),
            std::format("match.completed.winner.{}", match->WinnerId()->data())
        );
        
        return {};
    } catch (const std::exception& e) {
        return std::unexpected(std::string(e.what()));
    }
}

std::expected<std::string, std::string>
MatchDelegate::GetTournamentStatus(const std::string_view& tournamentId) {
    try {
        auto allMatches = GetMatchesByTournament(tournamentId);
        if (!allMatches) {
            return std::unexpected(allMatches.error());
        }
        
        int totalMatches = allMatches->size();
        int completedMatches = 0;
        std::optional<domain::TournamentPhase> currentPhase;
        
        for (auto& match : *allMatches) {
            if (match->Status() == domain::MatchStatus::COMPLETED) {
                completedMatches++;
            } else if (match->Status() == domain::MatchStatus::IN_PROGRESS ||
                       match->Status() == domain::MatchStatus::PENDING) {
                if (!currentPhase || match->Phase() < *currentPhase) {
                    currentPhase = match->Phase();
                }
            }
        }
        
        if (completedMatches == totalMatches) {
            // Buscar ganador de la final
            auto finalMatches = GetMatchesByPhase(tournamentId, domain::TournamentPhase::FINAL);
            if (finalMatches && !finalMatches->empty()) {
                auto final = (*finalMatches)[0];
                if (final->WinnerId()) {
                    return std::format("TOURNAMENT_COMPLETED - Winner: {}", final->WinnerId()->data());
                }
            }
            return "TOURNAMENT_COMPLETED";
        }
        
        if (currentPhase) {
            return std::format("IN_PROGRESS - Current Phase: {} ({}/{})",
                domain::phaseToString(*currentPhase),
                completedMatches,
                totalMatches);
        }
        
        return "PENDING";
        
    } catch (const std::exception& e) {
        return std::unexpected(std::string(e.what()));
    }
}