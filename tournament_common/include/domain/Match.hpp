#pragma once
#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace domain {

    struct Score {
        int home = 0;
        int visitor = 0;
    };

    struct Match {
        std::string id;
        std::string tournamentId;
        std::string homeTeamId;
        std::string awayTeamId;
        std::string round;   // "regular" | "elimination"
        std::string status;  // "pending" | "played"
        std::optional<Score> score;

        static Match fromJson(const nlohmann::json& j) {
            Match m;
            if (j.contains("id"))            m.id = j.at("id").get<std::string>();
            if (j.contains("tournamentId"))  m.tournamentId = j.at("tournamentId").get<std::string>();
            if (j.contains("homeTeamId"))    m.homeTeamId = j.at("homeTeamId").get<std::string>();
            if (j.contains("awayTeamId"))    m.awayTeamId = j.at("awayTeamId").get<std::string>();
            if (j.contains("round"))         m.round = j.at("round").get<std::string>();
            if (j.contains("status"))        m.status = j.at("status").get<std::string>();
            if (j.contains("score") && !j.at("score").is_null()) {
                Score s;
                const auto& sj = j.at("score");
                s.home    = sj.value("home", 0);
                s.visitor = sj.value("visitor", 0);
                m.score = s;
            }
            return m;
        }
    };

} // namespace domain
