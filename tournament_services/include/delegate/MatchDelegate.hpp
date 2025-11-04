#ifndef SERVICE_MATCH_DELEGATE_HPP
#define SERVICE_MATCH_DELEGATE_HPP

#include <memory>
#include <map>
#include "IMatchDelegate.hpp"
#include "persistence/repository/IRepository.hpp"
#include "cms/QueueMessageProducer.hpp"

class MatchDelegate : public IMatchDelegate {
    std::shared_ptr<IRepository<domain::Match, std::string>> matchRepository;
    std::shared_ptr<QueueMessageProducer> producer;
    
public:
    MatchDelegate(
        std::shared_ptr<IRepository<domain::Match, std::string>> repository,
        std::shared_ptr<QueueMessageProducer> producer
    );
    
    std::expected<std::vector<std::string>, std::string> 
        GenerateSingleEliminationBracket(
            const std::string_view& tournamentId,
            const std::vector<domain::Team>& teams
        ) override;
    
    std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>
        GetMatchesByTournament(const std::string_view& tournamentId) override;
    
    std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>
        GetMatchesByPhase(
            const std::string_view& tournamentId,
            domain::TournamentPhase phase
        ) override;
    
    std::expected<std::shared_ptr<domain::Match>, std::string>
        GetMatch(const std::string_view& matchId) override;
    
    std::expected<void, std::string>
        UpdateScore(
            const std::string_view& matchId,
            int scoreTeam1,
            int scoreTeam2
        ) override;
    
    std::expected<void, std::string>
        CompleteMatch(const std::string_view& matchId) override;
    
    std::expected<std::string, std::string>
        GetTournamentStatus(const std::string_view& tournamentId) override;
};

#endif