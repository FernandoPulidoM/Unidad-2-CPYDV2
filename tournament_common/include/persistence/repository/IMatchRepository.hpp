#pragma once
#include <memory>
#include <optional>
#include <vector>
#include <string>
#include <expected>
#include <nlohmann/json.hpp>

#include "domain/Match.hpp"

namespace persistence {

    class IMatchRepository {
    public:
        virtual ~IMatchRepository() = default;

        virtual std::vector<domain::Match>
        ListByTournament(const std::string& tid,
                         const std::optional<std::string>& filter) = 0;

        virtual std::optional<domain::Match>
        GetById(const std::string& tid, const std::string& mid) = 0;

        virtual std::expected<void, std::string>
        UpdateScore(const std::string& tid, const std::string& mid, int h, int v) = 0;

        virtual bool ExistsPairing(const std::string& tid,
                                   const std::string& home,
                                   const std::string& away) = 0;

        virtual std::expected<std::string, std::string>
        Create(const nlohmann::json& docJson) = 0;

        virtual std::expected<void, std::string>
        CreateBulk(const std::vector<nlohmann::json>& docs) = 0;
    };

} // namespace persistence