#ifndef NVI_API_H
#define NVI_API_H

#include "log.h"
#include "logger.h"
#include "options.h"
#include <cstddef>
#include <curl/curl.h>
#include <filesystem>
#include <string>

namespace nvi {
    /**
     * @detail Retrieves ENVs from the NVI API via an API key
     * @param `options` initialize parser with the following required option: `files`, followed by optional options:
     * `api`, `debug`, `dir`, `environment`, `project` and `required_envs`.
     * @example Fetching ENVs via project and environment
     *
     * nvi::options_t options;
     * options.api = true;
     * options.debug = false;
     * options.dir = "custom/path/to/envs";
     * optons.required_envs = {"KEY1", "KEY2", ...etc};
     * nvi::Api api(options);
     * std::string api_envs = api.get_key_from_file_or_input()->fetch_envs()->get_envs();
     */
    class Api {
        public:
        Api(options_t &options);
        Api *get_key_from_file_or_input() noexcept;
        Api *fetch_envs() noexcept;
        const std::string &get_envs() noexcept;

        ~Api() {
            if (_curl) {
                curl_easy_cleanup(_curl);
            }
        }

        private:
        void save_envs_to_disk() noexcept;
        void fetch_data(std::string REQ_URL) noexcept;
        std::string get_input_selection_for(std::string type) noexcept;

        options_t &_options;
        CURL *_curl;
        CURLcode _res;
        std::string _res_data;
        std::string _api_key;
        std::filesystem::path _env_file_path;
        unsigned int _res_status_code = 0;
        std::string _api_url;
        Logger logger;
    };
}; // namespace nvi
#endif
