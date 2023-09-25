#include "api.h"
#include "log.h"
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
    API::API(const options_t &options) : _curl(curl_easy_init()), _options(options) {}

    API *API::get_api_key_from_cli() noexcept {
        std::string api_key;
        std::clog << "[nvi] Please enter your unique API key: ";
        std::getline(std::cin, api_key);

        const bool api_key_is_valid =
            std::all_of(api_key.begin(), api_key.end(), [](char c) { return std::isalnum(c); });

        if (api_key.size() == 0 || not api_key_is_valid) {
            log(INVALID_INPUT_KEY);
        }

        // TODO(carlotta): this needs to be dynamic for dev and prod compilations
        _api_url = "http://127.0.0.1:5000/cli/secrets/?project=" + _options.project +
                   "&environment=" + _options.environment + "&apiKey=" + api_key;

        return this;
    }

    static size_t write_response(char *data, size_t size, size_t nmemb, std::string *output) {
        size_t totalSize = size * nmemb;
        output->append(static_cast<char *>(data), totalSize);
        return totalSize;
    }

    const std::string &API::fetch_envs() noexcept {
        if (_curl == nullptr) {
            log(CURL_FAILED_TO_INIT);
        }

        curl_easy_setopt(_curl, CURLOPT_URL, _api_url.c_str());
        curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, write_response);
        curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &_res_data);

        _res = curl_easy_perform(_curl);
        curl_easy_getinfo(_curl, CURLINFO_RESPONSE_CODE, &_res_status_code);

        if (_res != CURLE_OK) {
            log(REQUEST_ERROR);
        } else if (_res_status_code >= 400) {
            log(RESPONSE_ERROR);
        }

        return _res_data;
    }

    void API::log(const messages_t &code) const noexcept {
        // clang-format off
        switch (code) {
        case INVALID_INPUT_KEY: {
            NVI_LOG_ERROR_AND_EXIT(
                INVALID_INPUT_KEY,
                R"(The supplied input is not a valid API key. Please enter a valid API key (aAzZ0-9).)", 
                NULL);
            break;
        }
        case REQUEST_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                REQUEST_ERROR,
                R"(The cURL command failed. Reason: %s.)", 
                curl_easy_strerror(_res));
            break;
        }
        case RESPONSE_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                RESPONSE_ERROR,
                R"(The nvi API responded with a %d. Reason: %s.)", 
                _res_status_code, _res_data.c_str());
            break;
        }
        case CURL_FAILED_TO_INIT: {
            NVI_LOG_ERROR_AND_EXIT(
                CURL_FAILED_TO_INIT,
                R"("Failed to initialize cURL. Are you sure it's installed?)", 
                _res_status_code, _res_data.c_str());
            break;
        }
        default:
            break;
        }
    }
}; // namespace nvi