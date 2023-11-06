#include "arg.h"
#include "format.h"
#include "log.h"
#include "options.h"
#include "version.h"
#include <algorithm>
#include <cstdlib>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace nvi {
    Arg::Arg(int &argc, char *argv[], options_t &options)
        : _argc(argc - 1), _argv(argv), _options(options),
          logger(LOGGER::ARG, _options, _command, _invalid_args, _invalid_flag) {
        while (_index < _argc) {
            const std::string_view flag{_argv[++_index]};

            switch (FLAGS.count(flag) ? FLAGS.at(flag) : FLAG::UNKNOWN) {
            case FLAG::API: {
                _options.api = true;
                break;
            }
            case FLAG::CONFIG: {
                _options.config = parse_single_arg(CONFIG_FLAG_ERROR);
                break;
            }
            case FLAG::DEBUG: {
                _options.debug = true;
                break;
            }
            case FLAG::DIRECTORY: {
                _options.dir = parse_single_arg(DIR_FLAG_ERROR);
                break;
            }
            case FLAG::ENVIRONMENT: {
                _options.environment = parse_single_arg(ENV_FLAG_ERROR);
                break;
            }
            case FLAG::EXECUTE: {
                if (not _options.config.length()) {
                    parse_command_args();
                    goto exit_flag_parsing;
                }
                break;
            }
            case FLAG::FILES: {
                _options.files = parse_multi_arg(FILES_FLAG_ERROR);
                break;
            }
            case FLAG::HELP: {
                logger.log_and_exit(HELP_DOC);
                break;
            }
            case FLAG::PRINT: {
                _options.print = true;
                break;
            }
            case FLAG::PROJECT: {
                _options.project = parse_single_arg(PROJECT_FLAG_ERROR);
                break;
            }
            case FLAG::REQUIRED: {
                _options.required_envs = parse_multi_arg(REQUIRED_FLAG_ERROR);
                break;
            }
            case FLAG::SAVE: {
                _options.save = true;
                break;
            }
            case FLAG::VERSION: {
                logger.log_and_exit(NVI_VERSION);
                break;
            }
            default: {
                remove_invalid_flag();
                break;
            }
            };
        }

    exit_flag_parsing:
        // remove print flag if commands are present
        if (_options.commands.size() && _options.print) {
            if (_options.debug) {
                logger.debug(CONFLICTING_COMMAND_FLAG);
            }

            _options.print = false;
        }

        // remove flags when a config flag is set
        if (_options.config.length() &&
            (_options.api || _options.dir.length() || _options.environment.length() || _options.files.size() > 1 ||
             _options.print || _options.project.length() || _options.required_envs.size() || _options.save)) {
            if (_options.debug) {
                logger.debug(CONFLICTING_CONFIG_FLAG);
            }

            _options.api = false;
            _options.dir.clear();
            _options.environment.clear();
            _options.files.clear();
            _options.print = false;
            _options.project.clear();
            _options.required_envs.clear();
            _options.save = false;
        }

        if (_options.debug) {
            logger.debug(DEBUG_ARG);
        }
    };

    std::string Arg::parse_single_arg(const messages_t &code) noexcept {
        ++_index;
        if (_argv[_index] == nullptr) {
            logger.fatal(code);
        }

        const std::string arg{_argv[_index]};
        if (arg.find("-") != std::string::npos) {
            logger.fatal(code);
        }

        return arg;
    }

    std::vector<std::string> Arg::parse_multi_arg(const messages_t &code) noexcept {
        std::vector<std::string> args;
        while (_index < _argc) {
            ++_index;

            if (_argv[_index] == nullptr) {
                break;
            }

            const std::string next_arg{_argv[_index]};
            if (next_arg.find("-") != std::string::npos) {
                --_index;
                break;
            }

            if (std::find(args.begin(), args.end(), next_arg) == args.end()) {
                args.push_back(next_arg);
            }
        }

        if (not args.size()) {
            logger.fatal(code);
        }

        return args;
    }

    void Arg::parse_command_args() noexcept {
        while (_index < _argc) {
            ++_index;

            if (_argv[_index] == nullptr) {
                break;
            }

            std::string next_arg{_argv[_index]};
            if (not _options.commands.size()) {
                _bin_name = next_arg;
            }

            _command += _command.length() > 0 ? " " + next_arg : next_arg;
            _options.commands.push_back(std::move(_argv[_index]));
        }

        if (not _options.commands.size()) {
            logger.fatal(COMMAND_FLAG_ERROR);
        }

        _options.commands.push_back(nullptr);
    }

    void Arg::remove_invalid_flag() noexcept {
        _invalid_flag = std::string{_argv[_index]};
        _invalid_args.clear();
        while (_index < _argc) {
            ++_index;

            if (_argv[_index] == nullptr) {
                break;
            }

            const std::string arg{_argv[_index]};
            if (arg.find("-") != std::string::npos) {
                --_index;
                break;
            }

            _invalid_args += _invalid_args.length() ? " " + arg : arg;
        }

        logger.debug(INVALID_FLAG_WARNING);
    }
} // namespace nvi
