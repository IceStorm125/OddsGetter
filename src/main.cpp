#include <iostream>
#include <fstream>
#include <stdexcept>
#include <array>
#include <cstdio>
#include <memory>
#include <thread>
#include <set>
#include <filesystem>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "DatabaseManager.hpp"
#include "JSONparser.hpp"


int main() {

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("/app/OddsGetterLog.txt", false);

    std::vector<spdlog::sink_ptr> sinks {console_sink, file_sink};
    auto logger = std::make_shared<spdlog::logger>("main_logger", sinks.begin(), sinks.end());

    spdlog::register_logger(logger); 
    spdlog::set_default_logger(logger); 
    spdlog::flush_on(spdlog::level::info);

    spdlog::info("Starting application...");
    spdlog::info("Loadding .env file...");
    
    auto& env = EnvManager::getInstance();
    if(!env.isLoaded()) {
        spdlog::error("Failed to load .env file");
        return 1;
    }

    spdlog::info("Loaded .env file");

    std::optional<std::string> host = env.getValue("DB_HOST");
    std::optional<std::string> user = env.getValue("DB_USER");
    std::optional<std::string> password = env.getValue("DB_PASSWORD");
    std::optional<std::string> dbName = env.getValue("DB_NAME");
    std::optional<std::string> port = env.getValue("DB_PORT");
    std::optional<std::string> apiKey = env.getValue("API_KEY");

    if (!host.has_value() || !user.has_value() || !password.has_value() || !dbName.has_value() || !port.has_value() || !apiKey.has_value()) {
        spdlog::error("Failed to get database credentials from .env file");
        return EXIT_FAILURE;
    }

    std::unique_ptr<DatabaseManager> db = std::make_unique<DatabaseManager>(host.value(), user.value(), password.value(), dbName.value(), port.value());
    auto trnmts = db->getInProgressTournaments();

    JSONdataExtractor extractor;
    JSONparser parser;

    auto t1 = std::thread([&extractor, &parser, &apiKey, &trnmts, &db]() {
        while (true) {       
            std::this_thread::sleep_for(std::chrono::minutes(5));

            if(auto diff = db->hasMatchesWithoutResult().value(); db->hasMatchesWithoutResult().has_value() && diff > 10)
                continue;           

            spdlog::info("Getting match data");
            for (auto const & it : trnmts) {
                std::string json = extractor.getJson("https://api.the-odds-api.com/v4/sports/" + it.second + "/scores/?daysFrom=3&apiKey=" + apiKey.value());
                parser.getResultData(json);
            }
        }
    });
    t1.detach();

    while (true)
    {
        spdlog::info("Getting match data");
        for (auto const & it : trnmts) {
            std::string json = extractor.getJson("https://api.the-odds-api.com/v4/sports/" + it.second + "/odds?regions=eu&markets=h2h&oddsFormat=decimal&apiKey=" + apiKey.value());
            parser.getMatchData(json);
        }
        std::this_thread::sleep_for(std::chrono::minutes(60*24));
    }
    

    spdlog::info("Application finished");
   
    
    return 0;
}
