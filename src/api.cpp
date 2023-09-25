#include "api.h"
#include "options.h"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <curl/curl.h>
#include <curl/easy.h>
#include <iostream>
#include <string>

namespace nvi {
    API::API(const options_t &options) : _options(options) {}

    API *API::get_API_key_from_cli() noexcept {
        std::clog << "[nvi] Please enter your unique API key: ";
        std::getline(std::cin, _apiKey);

        ++_attempts;

        const bool is_valid = std::all_of(_apiKey.begin(), _apiKey.end(), [](char c) { return std::isalnum(c); });

        if (_apiKey.size() == 0 || not is_valid) {
            if (_attempts >= 5) {
                std::cerr << "[nvi] You've made too many attempts. Please try again later." << std::endl;
                std::exit(EXIT_FAILURE);
            }
            std::cerr << "[nvi] The supplied input is not a valid API key. Please enter a valid [aAzZ0-9] API key."
                      << " (attempts:" << _attempts << "/5)"
                      << "\n";
            get_API_key_from_cli();
        }

        return this;
    }

    size_t write_response(void *contents, size_t size, size_t nmemb, std::string *output) {
        size_t totalSize = size * nmemb;
        output->append(static_cast<char *>(contents), totalSize);
        return totalSize;
    }

    void API::fetch_envs() noexcept {
        CURL *curl;
        CURLcode res;
        std::string response_data;
        std::string api_url{"http://127.0.0.1:5000/cli/secrets/?project=nvi&environment=development&apiKey=" + _apiKey};

        curl = curl_easy_init();

        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, api_url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

            res = curl_easy_perform(curl);

            if (res != CURLE_OK) {
                std::cerr << "[nvi] ERROR: cURL failed. Reason: " << curl_easy_strerror(res) << '\n';
                std::exit(EXIT_FAILURE);
            } else if (_attempts >= 5) {
                std::cerr << "[nvi] ERROR: You've made too many attempts. Please try again later." << std::endl;
                std::exit(EXIT_FAILURE);
            } else {
                unsigned int res_status_code;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_status_code);
                if (res_status_code >= 400) {
                    std::cerr << "[nvi] ERROR: The nvi API responded with a " << res_status_code
                              << ". Reason: " << response_data << ". (attempts:" << _attempts << "/5)" << std::endl;
                    get_API_key_from_cli()->fetch_envs();
                } else {
                    std::cout << "[nvi] GOOD Response: " << response_data << std::endl;
                }
            }

            curl_easy_cleanup(curl);
        } else {
            std::cerr << "Failed to initialize cURL. Are you sure it's installed?" << std::endl;
        }

        std::exit(0);
    }
}; // namespace nvi
