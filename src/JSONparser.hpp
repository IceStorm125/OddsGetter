#include <iostream>
#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include "JSONdataExtactor.hpp"
#include "EnvManager.hpp"

class JSONparser
{
public:
    JSONparser()
    {
        auto& env = EnvManager::getInstance();
        if(!env.isLoaded()) {
            spdlog::error("Failed to load .env file");
        }

        spdlog::info("Loaded .env file");

        std::optional<std::string> host = env.getValue("DB_HOST");
        std::optional<std::string> user = env.getValue("DB_USER");
        std::optional<std::string> password = env.getValue("DB_PASSWORD");
        std::optional<std::string> dbName = env.getValue("DB_NAME");
        std::optional<std::string> port = env.getValue("DB_PORT");

        if (!host.has_value() || !user.has_value() || !password.has_value() || !dbName.has_value() || !port.has_value()) {
            spdlog::error("Failed to get database credentials from .env file");
        }

        db = std::make_unique<DatabaseManager>(host.value(), user.value(), password.value(), dbName.value(), port.value());
    }

    void getMatchData(const std::string &jsonString) const
    {
        try
        {
            std::unordered_set<std::string> matchIds = db->getMatchesApiIDWithoutResult();

            nlohmann::json matches = nlohmann::json::parse(jsonString);

            for (const auto &match : matches)
            {
                std::string api_id = match["id"];

                if(matchIds.find(api_id) != matchIds.end())
                    continue;

                std::string sportTitle = match["sport_title"];
                std::string homeTeam = match["home_team"];
                std::string awayTeam = match["away_team"];
                std::string commenceTime = match["commence_time"];

                auto firstBookmaker = match["bookmakers"][0];
                auto h2hMarket = firstBookmaker["markets"][0];

                double koef_w1 = 0.0;
                double koef_x = 0.0;
                double koef_w2 = 0.0;

                // Логика: ищем коэффициенты по имени команды
                for (const auto &outcome : h2hMarket["outcomes"])
                {
                    std::string name = outcome["name"];
                    double price = outcome["price"];

                    if (name == homeTeam)
                        koef_w1 = price;
                    else if (name == awayTeam)
                        koef_w2 = price;
                    else if (name == "Draw")
                        koef_x = price;
                }

                db->addMatch(api_id, db->getIdByTitle(sportTitle), homeTeam, awayTeam, koef_w1, koef_x, koef_w2, commenceTime);
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Ошибка при обработке JSON: " << e.what() << std::endl;
        }
    }

    void getResultData(const std::string &jsonString) const
    {
        try
        {
            nlohmann::json matches = nlohmann::json::parse(jsonString);

            std::unordered_set<std::string> matchIds = db->getMatchesApiIDWithResult();


            for (const auto &match : matches)
            {
                if (!match.contains("completed") || !match["completed"].get<bool>())
                    continue;

                std::string api_id = match["id"];
                if(matchIds.find(api_id) != matchIds.end())
                    continue;
                
                std::string homeTeam = match["home_team"];
                std::string awayTeam = match["away_team"];
                int homeScore = 0;
                int awayScore = 0;

                // Извлекаем счёт по именам команд
                for (const auto &scoreItem : match["scores"])
                {
                    std::string name = scoreItem["name"];
                    int score = std::stoi(scoreItem["score"].get<std::string>());

                    if (name == homeTeam)
                        homeScore = score;
                    else if (name == awayTeam)
                        awayScore = score;
                }

                std::string result;
                if (homeScore > awayScore)
                    result = "W1";
                else if (homeScore < awayScore)
                    result = "W2";
                else
                    result = "X";


                db->addResult(api_id, result);
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Ошибка при обработке JSON: " << e.what() << std::endl;
        }
    }

private:
    std::unique_ptr<DatabaseManager> db; // объект базы данных
};