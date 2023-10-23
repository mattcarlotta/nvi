#ifndef NVI_ENV_GENERATOR_H
#define NVI_ENV_GENERATOR_H

#include "log.h"
#include "options.h"
#include "parser.h"

namespace nvi {
    class Generator {
        public:
        Generator(const env_map_t &&env_map, const options_t &&options);
        void set_or_print_envs() const noexcept;

        private:
        void log(const messages_t &code) const noexcept;

        env_map_t _env_map;
        options_t _options;
    };
} // namespace nvi

#endif
