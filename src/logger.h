#ifndef NVI_LOGGER_H
#define NVI_LOGGER_H

#include "log.h"
#include "options.h"
#include <string>

namespace nvi {
    class Logger {

        public:
        Logger(const options_t &options);
        Logger(const options_t &options, const std::string &command, const std::string &invalid_args,
               const std::string &invalid_flag);

        void Arg(const messages_t &code) const noexcept;
        void Generator(const messages_t &code) const noexcept;

        private:
        const std::string empty_string = "";
        const options_t &_options;
        const std::string &_command;
        const std::string &_invalid_args;
        const std::string &_invalid_flag;
    };
}; // namespace nvi
#endif
