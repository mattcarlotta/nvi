#ifndef NVI_API_H
#define NVI_API_H

#include "log.h"
#include "options.h"
#include <cstddef>
#include <curl/curl.h>
#include <filesystem>
#include <string>

namespace nvi {
    /**
     * @detail Retrieves ENVs from the NVI API via an API key
     * @param `options` initialize parser with the following required option: `files`, followed by optional options:
     * `dir`, `debug`, `env`, `project` and `required_envs`.
     * @example Fetching ENVs via project and environment
     *
     * nvi::options_t options;
     * options.debug = false;
     * options.dir = "custom/path/to/envs";
     * options.files = {".env", "base.env", ...etc};
     * optons.required_envs = {"KEY1", "KEY2", ...etc};
     * nvi::Api api(options);
     * std::string api_envs = api.get_key_from_file_or_input()->fetch_envs();
     */
    class Api {
        public:
        Api(const options_t &options);
        Api *get_key_from_file_or_input() noexcept;
        const std::string &fetch_envs() noexcept;

        ~Api() {
            if (_curl) {
                curl_easy_cleanup(_curl);
            }
        }

        private:
        void log(const messages_t &code) const noexcept;
        void save_envs_to_disk() noexcept;

        CURL *_curl;
        options_t _options;
        CURLcode _res;
        std::string _res_data;
        std::filesystem::path _env_file_path;
        unsigned int _res_status_code = 0;
        std::string _api_url;
    };
}; // namespace nvi
#endif
