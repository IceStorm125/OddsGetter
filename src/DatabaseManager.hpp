#include <iostream>
#include <tuple>
#include <vector>
#include <memory>
#include <mutex>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>
#include <chrono>
#include <optional>

#include <spdlog/spdlog.h>


#include <Poco/Data/RecordSet.h>
#include <Poco/Data/Session.h>
#include <Poco/Data/SessionFactory.h>
#include <Poco/Data/MySQL/Connector.h>
#include <Poco/Data/Statement.h>
#include <Poco/Timestamp.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/Timezone.h>

class DatabaseManager
{
public:
    DatabaseManager(const std::string &host, const std::string &user, const std::string &password, const std::string &dbName, const std::string &port);
    ~DatabaseManager();

    void addMatch(std::string api_id, int tournament_id, std::string team1, std::string team2, double koef_w1, double koef_x, double koef_w2, std::string time);
    void addResult(std::string api_id, std::string result);
    int getIdByTitle(std::string title);
    std::optional<int> hasMatchesWithoutResult();
    std::map<int, std::string> getInProgressTournaments();
    std::unordered_set<std::string> getMatchesApiIDWithoutResult();
    std::unordered_set<std::string> getMatchesApiIDWithResult();

private:
    std::unique_ptr<Poco::Data::Session> session;
    std::mutex sessionMutex; 
};
DatabaseManager::DatabaseManager(const std::string &host, const std::string &user, const std::string &password, const std::string &dbName, const std::string &port)
{
    Poco::Data::MySQL::Connector::registerConnector();
    try
    {
        session = std::make_unique<Poco::Data::Session>("MySQL", "host=" + host + ";user=" + user + ";password=" + password + ";db=" + dbName + ";port=" + port);
        spdlog::info("Succes connection to DB");
    }
    catch (const Poco::Exception &ex)
    {
        spdlog::error("Failed connection to DB: {}", ex.displayText());
    }    
}

DatabaseManager::~DatabaseManager()
{
    Poco::Data::MySQL::Connector::unregisterConnector();
    spdlog::info("Connection to DB closed");
}

void DatabaseManager::addMatch(std::string api_id, int tournament_id, std::string team1, std::string team2, double koef_w1, double koef_x, double koef_w2, std::string time)
{
    try
    {
        std::lock_guard<std::mutex> lock(sessionMutex);

        Poco::Data::Statement insert(*session);
        insert << "INSERT INTO matches (api_id, tournament_id, team1, team2, koef_W1, koef_X, koef_W2, time) VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
            Poco::Data::Keywords::use(api_id),
            Poco::Data::Keywords::use(tournament_id),
            Poco::Data::Keywords::use(team1),
            Poco::Data::Keywords::use(team2),
            Poco::Data::Keywords::use(koef_w1),
            Poco::Data::Keywords::use(koef_x),
            Poco::Data::Keywords::use(koef_w2),
            Poco::Data::Keywords::use(time);

        insert.execute();
        spdlog::info("Match successfully added: Tournament id: {}, {} vs {} ({}/{}/{}) {}", tournament_id, team1, team2, koef_w1, koef_x, koef_w2, time);
        
    }
    catch (const Poco::Exception &ex)
    {
        spdlog::error("Ошибка вставки данных: {}", ex.displayText());
    }
}

void DatabaseManager::addResult(std::string api_id, std::string result)
{
    try
    {
        std::lock_guard<std::mutex> lock(sessionMutex);
        
        Poco::Data::Statement insert(*session);
        insert << "UPDATE matches SET match_result_id=(SELECT id FROM match_results WHERE name = ?) WHERE api_id = ?",
            Poco::Data::Keywords::use(result),
            Poco::Data::Keywords::use(api_id);

        insert.execute();
        spdlog::info("Result successfully added: api_id: {}, result: {}", api_id, result);

    }
    catch (const Poco::Exception &ex)
    {
        spdlog::error("Ошибка вставки данных: {}", ex.displayText());
    }
}

int DatabaseManager::getIdByTitle(std::string title)
{
    try
    {
        std::lock_guard<std::mutex> lock(sessionMutex);

        Poco::Data::Statement select(*session);
        select << "SELECT id FROM tournaments WHERE title = ?", Poco::Data::Keywords::use(title);
        select.execute();
        Poco::Data::RecordSet rs(select);
        return rs[0][0].convert<int>();
    }
    catch (const Poco::Exception &ex)
    {
        spdlog::error("Ошибка выборки данных: {}", ex.displayText());
        return -1;
    }
}


std::map<int, std::string> DatabaseManager::getInProgressTournaments()
{
    try
    {
        std::lock_guard<std::mutex> lock(sessionMutex);

        Poco::Data::Statement select(*session);
        select << "SELECT id, api_key FROM tournaments";
        select.execute();
        Poco::Data::RecordSet rs(select);

        if (rs.rowCount() == 0)
        {
            std::cout << "No tournaments" << std::endl;
            return {};
        }

        std::map<int, std::string> toReturn;
        for (auto it = rs.begin(); it != rs.end(); ++it)
        {
            int id = (*it)[0].convert<int>();
            std::string key = (*it)[1].convert<std::string>();

            std::cout << "Tournament: " << key << std::endl;
            toReturn.insert({id, key});
        }

        return toReturn;
    }
    catch (const Poco::Exception &ex)
    {
        spdlog::error("Ошибка выборки данных: {}", ex.displayText());
    }

    return {};
}

std::unordered_set<std::string> DatabaseManager::getMatchesApiIDWithoutResult()
{
    try
    {
        std::lock_guard<std::mutex> lock(sessionMutex);

        Poco::Data::Statement select(*session);
        select << "SELECT api_id FROM matches WHERE match_result_id IS NULL";
        select.execute();
        Poco::Data::RecordSet rs(select);

        std::unordered_set<std::string> toReturn;
        for (auto it = rs.begin(); it != rs.end(); ++it)
            toReturn.insert((*it)[0].convert<std::string>());

        return toReturn;
    }
    catch (const Poco::Exception &ex)
    {
        spdlog::error("Ошибка выборки данных: {}", ex.displayText());
    }
    return std::unordered_set<std::string>();
}

inline std::unordered_set<std::string> DatabaseManager::getMatchesApiIDWithResult()
{
    try
    {
        std::lock_guard<std::mutex> lock(sessionMutex);

        Poco::Data::Statement select(*session);
        select << "SELECT api_id FROM matches WHERE match_result_id IS NOT NULL";
        select.execute();
        Poco::Data::RecordSet rs(select);

        std::unordered_set<std::string> toReturn;
        for (auto it = rs.begin(); it != rs.end(); ++it)
            toReturn.insert((*it)[0].convert<std::string>());

        return toReturn;
    }
    catch (const Poco::Exception &ex)
    {
        spdlog::error("Ошибка выборки данных: {}", ex.displayText());
    }
    return std::unordered_set<std::string>();
}

std::chrono::time_point<std::chrono::system_clock>
parse_iso8601(const std::string& iso)
{
    std::tm t = {};
    std::istringstream ss(iso);
    ss >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S%Ez");
    
    return std::chrono::system_clock::from_time_t(std::mktime(&t));
}

std::optional<int> DatabaseManager::hasMatchesWithoutResult()
{
    try
    {
        std::lock_guard<std::mutex> lock(sessionMutex);

        Poco::Data::Statement select(*session);
        select << "SELECT time FROM matches WHERE match_result_id IS NULL ORDER BY time ASC LIMIT 1";
        select.execute();
        Poco::Data::RecordSet rs(select);

        if (rs.rowCount() == 0)
            return std::nullopt;

        auto it = rs.begin();
        auto matchTime = parse_iso8601((*it)[0].convert<std::string>());
        
        auto endTime = matchTime + std::chrono::hours(2);
        auto diff = std::chrono::duration_cast<std::chrono::minutes>(endTime - std::chrono::system_clock::now());

        // std::time_t now_t = std::chrono::system_clock::to_time_t(now);
        // std::time_t endTime_t = std::chrono::system_clock::to_time_t(endTime);

        // std::cout << diff.count() << std::endl;
        
        // std::cout << std::put_time(std::localtime(&now_t), "%Y-%m-%d %H:%M:%S") << " " 
        //             << std::put_time(std::localtime(&endTime_t), "%Y-%m-%d %H:%M:%S") << std::endl;
        

        return diff.count();
    }
    catch (const Poco::Exception &ex)
    {
        spdlog::error("Ошибка выборки данных: {}", ex.displayText());
        return std::nullopt;

    }
}
