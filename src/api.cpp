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
#include <map>
#include <sstream>
#include <string>
#include <termios.h>
#include <unistd.h>

namespace nvi {
    Api::Api(const options_t &options) : _curl(curl_easy_init()), _options(options) {}

    std::string Api::get_input_selection_for(std::string type) noexcept {
        std::clog << "[nvi] Retrieved the following " << type << "s from the nvi API..." << '\n';
        std::map<int, std::string> list;
        int index = 0;
        std::string line;
        std::istringstream ss{_res_data};
        while (std::getline(ss, line)) {
            if (line.length()) {
                ++index;
                list[index] = line;
                std::clog << "[" << index << "]: " << line << '\n';
            }
        }

        std::clog << '\n' << "Please select one of the " << type << "s by providing its corresponding [number]: ";
        unsigned int item_number;
        std::cin >> item_number;
        std::clog << '\n';

        if (not list.count(item_number)) {
            log(INVALID_INPUT_SELECTION);
        };

        return list.at(item_number);
    }

    static bool filter_non_alphanum_char(const char &c) { return not std::isalnum(c); }

    Api *Api::get_key_from_file_or_input() noexcept {
        const std::filesystem::path api_key_file = std::filesystem::current_path() / ".nvi-key";
        if (std::filesystem::exists(api_key_file)) {
            std::ifstream api_file{api_key_file, std::ios_base::in};

            _api_key = std::string{std::istreambuf_iterator{api_file}, {}};

            api_file.close();
        } else {
            std::clog << "[nvi] Please enter your unique API key: ";

            // disable echo input
            termios term;
            tcgetattr(STDIN_FILENO, &term);
            term.c_lflag &= ~ECHO;
            tcsetattr(STDIN_FILENO, TCSANOW, &term);

            // get api key from user input
            std::getline(std::cin, _api_key);
            std::clog << '\n';

            // enable echo input
            term.c_lflag |= ECHO;
            tcsetattr(STDIN_FILENO, TCSANOW, &term);
        }

        _api_key.erase(std::remove_if(_api_key.begin(), _api_key.end(), filter_non_alphanum_char), _api_key.end());

        if (_api_key.length() == 0 || _api_key.length() > 50) {
            log(INVALID_INPUT_KEY);
        }

        // get projects from API and prompt for user selection
        if (not _options.project.length()) {
            fetch_data("/cli/projects/?apiKey=" + _api_key);
            _options.project = get_input_selection_for("project");
        }

        // get environments from API and prompt for user selection
        if (not _options.environment.length()) {
            fetch_data("/cli/environments/?apiKey=" + _api_key + "&project=" + _options.project);
            _options.environment = get_input_selection_for("environment");
        }

        return this;
    }

    static size_t write_response(char *data, size_t size, size_t nmemb, std::string *output) {
        size_t total_size = size * nmemb;
        output->append(static_cast<char *>(data), total_size);
        return total_size;
    }

    void Api::fetch_data(std::string REQ_URL) noexcept {
        if (_curl == nullptr) {
            log(CURL_FAILED_TO_INIT);
        }

        _res_data.clear();
        _api_url = API_URL + REQ_URL;

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
    }

    void Api::save_envs_to_disk() noexcept {
        const std::string env_name = _options.environment + ".env";
        _env_file_path = std::filesystem::current_path() / env_name;
        if (std::filesystem::exists(_env_file_path)) {
            std::clog << "[nvi] WARNING: A file named \"" << env_name << "\" already exists at the current path ("
                      << std::filesystem::current_path() << "). " << '\n'
                      << "[nvi] Are you sure you want to save and overwrite it? (y|N): ";

            std::string save_file;
            std::getline(std::cin, save_file);

            if (save_file != "y" && save_file != "Y" && save_file == "yes" && save_file == "Yes") {
                return;
            }
        }

        std::ofstream env_file{_env_file_path};
        if (not env_file.is_open()) {
            log(FILE_ERROR);
        }

        env_file << _res_data;

        if (_options.debug) {
            log(SAVED_ENV_FILE);
        }

        env_file.close();
    }

    Api *Api::fetch_envs() noexcept {
        fetch_data("/cli/secrets/?apiKey=" + _api_key + "&project=" + _options.project +
                   "&environment=" + _options.environment);

        if (_options.debug) {
            log(RESPONSE_SUCCESS);
        }

        if (_options.save) {
            save_envs_to_disk();
        }

        return this;
    }

    const std::string &Api::get_envs() noexcept { return _res_data; }

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
        case INVALID_INPUT_SELECTION: {
            NVI_LOG_ERROR_AND_EXIT(
                INVALID_INPUT_SELECTION,
                "The supplied number input was not a valid selection. Please try again.", 
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
        case FILE_ERROR: {
            NVI_LOG_ERROR_AND_EXIT(
                FILE_ERROR,
                R"(Unable to open "%s". The .env file is either invalid, has restricted access, or may be corrupted.)",
                _env_file_path.c_str());
            break;
        }
        case SAVED_ENV_FILE: {
            NVI_LOG_DEBUG(
                SAVED_ENV_FILE,
                R"(Successfully saved the "%s.env" file to disk (%s).)", 
                _options.environment.c_str(), _env_file_path.c_str());
            break;
        }
        default:
            break;
        }
    }
}; // namespace nvi
