#include "api.h"
#include "apiurl.h"
#include "log.h"
#include "options.h"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <curl/curl.h>
#include <curl/easy.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <termios.h>
#include <unistd.h>

namespace nvi {
    Api::Api(const options_t &options) : _curl(curl_easy_init()), _options(options) {}

    static bool filter_non_alphanum_char(const char &c) { return not std::isalnum(c); }

    Api *Api::get_key_from_file_or_input() noexcept {
        std::string api_key;

        const std::string api_key_file = std::filesystem::current_path() / ".nvi-key";
        if (std::filesystem::exists(api_key_file)) {
            std::ifstream api_file{api_key_file, std::ios_base::in};

            api_key = std::string{std::istreambuf_iterator{api_file}, {}};

            api_file.close();
        } else {
            std::clog << "[nvi] Please enter your unique API key: ";

            // disable echo input
            termios term;
            tcgetattr(STDIN_FILENO, &term);
            term.c_lflag &= ~ECHO;
            tcsetattr(STDIN_FILENO, TCSANOW, &term);

            // get api key from user input
            std::getline(std::cin, api_key);
            std::clog << std::endl;

            // enable echo input
            term.c_lflag |= ECHO;
            tcsetattr(STDIN_FILENO, TCSANOW, &term);
        }

        api_key.erase(std::remove_if(api_key.begin(), api_key.end(), filter_non_alphanum_char), api_key.end());

        if (api_key.length() == 0 || api_key.length() > 50) {
            log(INVALID_INPUT_KEY);
        }

        _api_url = API_URL "/cli/secrets/?project=" + _options.project + "&environment=" + _options.environment +
                   "&apiKey=" + api_key;

        return this;
    }

    static size_t write_response(char *data, size_t size, size_t nmemb, std::string *output) {
        size_t total_size = size * nmemb;
        output->append(static_cast<char *>(data), total_size);
        return total_size;
    }

    const std::string &Api::fetch_envs() noexcept {
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
        } else if (_options.debug) {
            log(RESPONSE_SUCCESS);
        }

        return _res_data;
    }

    void Api::log(const messages_t &code) const noexcept {
        // clang-format off
        switch (code) {
        case INVALID_INPUT_KEY: {
            NVI_LOG_ERROR_AND_EXIT(
                INVALID_INPUT_KEY,
                "The supplied input is not a valid API key. Please enter a valid API key with aA,zZ,0-9 characters.", 
                NULL);
            break;
        }
        case REQUEST_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                REQUEST_ERROR,
                "The cURL command failed: %s.", 
                curl_easy_strerror(_res));
            break;
        }
        case RESPONSE_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                RESPONSE_ERROR,
                "The nvi API responded with a %d: %s.", 
                _res_status_code, _res_data.c_str());
            break;
        }
        case RESPONSE_SUCCESS: {
            NVI_LOG_DEBUG(
                RESPONSE_SUCCESS,
                "Successfully retrieved the %s ENVs from the nvi API.", 
                _options.environment.c_str());
            break;
        }
        case CURL_FAILED_TO_INIT: {
            NVI_LOG_ERROR_AND_EXIT(
                CURL_FAILED_TO_INIT,
                "Failed to initialize cURL. Are you sure it's installed?", 
                NULL);
            break;
        }
        default:
            break;
        }
    }
}; // namespace nvi
