#pragma once
#include <expected>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "tournament_common/include/domain/Match.hpp"

namespace services {

    struct Error {
        int code;            // 404, 422, 500
        std::string message; // detalle
    };

    class IMatchDelegate {
    public:
        virtual ~IMatchDelegate() = default;

        virtual std::vector<domain::Match>
        List(const std::string& tournamentId,
             const std::optional<std::string>& filterPlayedOrPending) = 0;

        virtual std::optional<domain::Match>
        Get(const std::string& tournamentId, const std::string& matchId) = 0;

        virtual std::expected<void, Error>
        UpdateScore(const std::string& tournamentId, const std::string& matchId,
                    int home, int visitor) = 0;
    };

} // namespace services
