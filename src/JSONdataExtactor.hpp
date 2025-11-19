#include <curl/curl.h>
#include <iostream>
#include <string>

class JSONdataExtractor {
public:
    // Получить JSON по URL
    std::string getJson(const std::string& url) {
        CURL* curl = curl_easy_init();
        if (!curl) {
            std::cerr << "Ошибка инициализации CURL" << std::endl;
            return "";
        }

        std::string responseData;

        // Устанавливаем URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Функция обратного вызова для записи данных
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

        // Включаем проверку SSL сертификата (по умолчанию)
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

        // Устанавливаем User-Agent
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "JSONdataExtractor/1.0");

        // Выполняем запрос
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
            responseData.clear();
        }

        curl_easy_cleanup(curl);
        return responseData;
    }

private:
    // Callback для записи данных в std::string
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t totalSize = size * nmemb;
        std::string* str = static_cast<std::string*>(userp);
        str->append(static_cast<char*>(contents), totalSize);
        return totalSize;
    }
};
