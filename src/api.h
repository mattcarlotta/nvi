#ifndef NVI_API_H
#define NVI_API_H

#include "log.h"
#include "options.h"
#include <cstddef>
#include <curl/curl.h>
#include <string>

namespace nvi {
    /**
     * @detail Retrieves ENVs from the NVI API
     * @param `options` initialize parser with the following required option: `files`, followed by optional options:
     * `dir`, `required_envs`, and `debug`.
     * @example Fetching ENVs via project and environment
     *
     */
    class API {
        public:
            API(const options_t &options);
            const std::string &get_envs() const noexcept;
            void fetch_envs() noexcept;
            API *get_API_key_from_cli() noexcept;

        private:
            // static size_t write_response(void *contents, size_t nemb, std::string *ouput);
            // void log(const messages_t &code) const noexcept;

            unsigned int _attempts = 0;
            options_t _options;
            std::string _apiKey;
            std::string _response_data;
    };
}; // namespace nvi
#endif
