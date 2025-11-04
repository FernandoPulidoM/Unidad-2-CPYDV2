#ifndef DOMAIN_UTILITIES_HPP
#define DOMAIN_UTILITIES_HPP

#include <nlohmann/json.hpp>
#include "domain/Team.hpp"
#include "domain/Tournament.hpp"
#include "domain/Group.hpp"
#include "domain/Match.hpp"

namespace domain {

    inline void to_json(nlohmann::json& json, const Team& team) {
        json = {{"id", team.Id}, {"name", team.Name}};
    }

    inline void from_json(const nlohmann::json& json, Team& team) {
        if(json.contains("id")) {
            json.at("id").get_to(team.Id);
        }
        json.at("name").get_to(team.Name);
    }

    inline void from_json(const nlohmann::json& json, std::vector<Team>& teams) {
        for (auto j = json.begin(); j != json.end(); ++j) {
            Team team;
            if(j.value().contains("id")) {
                j.value().at("id").get_to(team.Id);
            }
            if(j.value().contains("name")) {
                j.value().at("name").get_to(team.Name);
            }
            teams.push_back(team);
        }
    }

    inline void to_json(nlohmann::json& json, const std::shared_ptr<Team>& team) {
        json = nlohmann::basic_json();
        json["name"] = team->Name;

        if (!team->Id.empty()) {
            json["id"] = team->Id;
        }
    }

    inline TournamentType fromString(std::string_view type) {
        if (type == "ROUND_ROBIN")
            return TournamentType::ROUND_ROBIN;
        if (type == "NFL")
            return TournamentType::NFL;

        return TournamentType::ROUND_ROBIN;
    }

    inline void from_json(const nlohmann::json& json, TournamentFormat& format) {
        if(json.contains("maxTeamsPerGroup"))
            json.at("maxTeamsPerGroup").get_to(format.MaxTeamsPerGroup());
        if(json.contains("numberOfGroups"))
            json.at("numberOfGroups").get_to(format.NumberOfGroups());
        if(json.contains("type"))
            format.Type() = fromString(json["type"].get<std::string>());
    }

    inline void to_json(nlohmann::json& json, const TournamentFormat& format) {
        json = {{"maxTeamsPerGroup", format.MaxTeamsPerGroup()}, {"numberOfGroups", format.NumberOfGroups()}};
        switch (format.Type()) {
            case TournamentType::ROUND_ROBIN:
                json["type"] = "ROUND_ROBIN";
                break;
            case TournamentType::NFL:
                json["type"] = "NFL";
                break;
            default:
                json["type"] = "ROUND_ROBIN";
        }
    }

    inline void to_json(nlohmann::json& json, const std::shared_ptr<Tournament>& tournament) {
        json = {{"name", tournament->Name()}};
        if (!tournament->Id().empty()) {
            json["id"] = tournament->Id();
        }
        json["format"] = tournament->Format();
    }

    inline void from_json(const nlohmann::json& json, std::shared_ptr<Tournament>& tournament) {
        if(json.contains("id")) {
            tournament->Id() = json["id"].get<std::string>();
        }
        json["name"].get_to(tournament->Name());
        if (json.contains("format"))
            json.at("format").get_to(tournament->Format());
    }

    inline void to_json(nlohmann::json& json, const Tournament& tournament) {
        json = {{"name", tournament.Name()}};
        if (!tournament.Id().empty()) {
            json["id"] = tournament.Id();
        }
        json["format"] = tournament.Format();
    }

    inline void from_json(const nlohmann::json& json, Tournament& tournament) {
        if(json.contains("id")) {
            tournament.Id() = json["id"].get<std::string>();
        }
        json["name"].get_to(tournament.Name());
        if (json.contains("format"))
            json.at("format").get_to(tournament.Format());
    }

    inline void from_json(const nlohmann::json& json, Group& group) {
        if(json.contains("id")) {
            group.Id() = json["id"].get<std::string>();
        }
        if(json.contains("tournamentId")) {
            group.TournamentId() = json["tournamentId"].get<std::string>();
        }
        json["name"].get_to(group.Name());

//si no contiene un arreglo de teams, saltartelo
        if (auto it = json.find("teams"); it != json.end() && it->is_array()) {
            it->get_to(group.Teams());
        }

    }

    inline void to_json(nlohmann::json& json, const std::shared_ptr<Group>& group) {
        json["name"] = group->Name();
        json["tournamentId"] = group->TournamentId();
        if (!group->Id().empty()) {
            json["id"] = group->Id();
        }
        json["teams"] = group->Teams();
    }

    inline void to_json(nlohmann::json& json, const std::vector<std::shared_ptr<Group>>& groups) {
        json = nlohmann::json::array();
        for (const auto& group : groups) {
            auto jsonGroup = nlohmann::json();
            jsonGroup["name"] = group->Name();
            jsonGroup["tournamentId"] = group->TournamentId();
            if (!group->Id().empty()) {
                jsonGroup["id"] = group->Id();
            }
            jsonGroup["teams"] = group->Teams();
            json.push_back(jsonGroup);
        }
    }

    inline void to_json(nlohmann::json& json, const Group& group) {
        json["name"] = group.Name();
        json["tournamentId"] = group.TournamentId();
        if (!group.Id().empty()) {
            json["id"] = group.Id();
        }
        json["teams"] = group.Teams();
    }

    // En domain/Utilities.hpp - agregar serialización para Match

    // TournamentPhase → JSON
    inline void to_json(nlohmann::json& j, const domain::TournamentPhase& phase) {
        j = domain::phaseToString(phase);
    }

    inline void from_json(const nlohmann::json& j, domain::TournamentPhase& phase) {
        std::string str = j.get<std::string>();
        if (str == "QUARTER_FINALS") phase = domain::TournamentPhase::QUARTER_FINALS;
        else if (str == "SEMI_FINALS") phase = domain::TournamentPhase::SEMI_FINALS;
        else if (str == "FINAL") phase = domain::TournamentPhase::FINAL;
        else if (str == "THIRD_PLACE") phase = domain::TournamentPhase::THIRD_PLACE;
    }

    // MatchStatus → JSON
    inline void to_json(nlohmann::json& j, const domain::MatchStatus& status) {
        j = domain::statusToString(status);
    }

    inline void from_json(const nlohmann::json& j, domain::MatchStatus& status) {
        std::string str = j.get<std::string>();
        if (str == "PENDING") status = domain::MatchStatus::PENDING;
        else if (str == "IN_PROGRESS") status = domain::MatchStatus::IN_PROGRESS;
        else if (str == "COMPLETED") status = domain::MatchStatus::COMPLETED;
        else if (str == "CANCELLED") status = domain::MatchStatus::CANCELLED;
    }

    // Match → JSON
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
        domain::Match,
        id, tournamentId, phase, team1, team2,
        scoreTeam1, scoreTeam2, status, winnerId, nextMatchId
    )
}

#endif /* FC7CD637_41CC_48DE_8D8A_BC2CFC528D72 */
