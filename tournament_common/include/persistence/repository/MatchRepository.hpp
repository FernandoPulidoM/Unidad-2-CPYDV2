#pragma once
#include <memory>
#include <optional>
#include <vector>
#include <string>
#include <expected>
#include <nlohmann/json.hpp>

#include "domain/Match.hpp"
#include "persistence/configuration/IDbConnectionProvider.hpp"  // este header esta en global ns

namespace persistence {

    class MatchRepository {
    public:
        explicit MatchRepository(std::shared_ptr<IDbConnectionProvider> db)
          : db_(std::move(db)) {}

        static domain::Match fromRow(const std::string& id, const nlohmann::json& j);

        std::vector<domain::Match>
        ListByTournament(const std::string& tid,
                         const std::optional<std::string>& filter);

        std::optional<domain::Match>
        GetById(const std::string& tid, const std::string& mid);

        std::expected<void, std::string>
        UpdateScore(const std::string& tid, const std::string& mid, int h, int v);

        bool ExistsPairing(const std::string& tid,
                           const std::string& home,
                           const std::string& away);

        std::expected<std::string, std::string>
        Create(const nlohmann::json& docJson);

        std::expected<void, std::string>
        CreateBulk(const std::vector<nlohmann::json>& docs);

    private:
        std::shared_ptr<IDbConnectionProvider> db_; // global ns
    };

} // namespace persistence
