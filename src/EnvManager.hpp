#include <fstream>
#include <string>
#include <unordered_map>
#include <optional>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <shared_mutex>

class EnvManager {
public:
    // Потокобезопасный доступ к единственному экземпляру
    static EnvManager& getInstance() {
        static EnvManager instance; // Meyers Singleton
        return instance;
    }

    std::optional<std::string> getValue(const std::string& key) const {
        std::shared_lock lock(mutex_);

        if (auto it = envData_.find(key); it != envData_.end())
            return it->second;

        return std::nullopt;
    }

    bool isLoaded() const noexcept {
        return loaded_;
    }

private:
    // Данные
    std::unordered_map<std::string, std::string> envData_;
    bool loaded_ = false;

    mutable std::shared_mutex mutex_;

    // Приватный конструктор (Singleton)
    explicit EnvManager(const std::string& filePath = ".env") {
        load(filePath);
    }

    // Запрещаем копирование
    EnvManager(const EnvManager&) = delete;
    EnvManager& operator=(const EnvManager&) = delete;

    // Парсинг .env
    void load(const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Не удалось открыть файл: " << filePath << std::endl;
            loaded_ = false;
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            trim(line);

            if (line.empty() || line[0] == '#')
                continue;

            auto delimiterPos = line.find('=');
            if (delimiterPos == std::string::npos)
                continue;

            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);

            trim(key);
            trim(value);

            {
                std::unique_lock lock(mutex_);
                envData_[key] = value;
            }
        }

        loaded_ = true;
    }

    // trim
    static void trim(std::string& s) {
        auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
        s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    }
};
