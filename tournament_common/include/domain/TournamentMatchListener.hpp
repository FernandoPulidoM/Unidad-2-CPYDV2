#ifndef TOURNAMENTS_MATCHLISTENER_HPP
#define TOURNAMENTS_MATCHLISTENER_HPP

#include <cms/ConnectionManager.hpp>
#include <cms/QueueMessageConsumer.hpp>
#include <memory>
#include <string>

class TournamentMatchListener {
    std::shared_ptr<QueueMessageConsumer> consumer;
    
public:
    explicit TournamentMatchListener(std::shared_ptr<QueueMessageConsumer> consumer);
    
    void Start();
    void OnMatchCompleted(const std::string& message);
    void OnBracketGenerated(const std::string& message);
};

#endif