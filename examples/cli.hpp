#ifndef CLI_HPP
#define CLI_HPP

#include <map>
#include <print>
#include <ranges>
#include <regex>

#include <CLI/CLI.hpp>
#include <utility>
#include <logger.h>

namespace TFTP {
    
    static const std::map<std::string, enum logger_log_level> log_level_map{
        {"off",   LOGGER_LOG_LEVEL_OFF},
        {"fatal", LOGGER_LOG_LEVEL_FATAL},
        {"error", LOGGER_LOG_LEVEL_ERROR},
        {"warn",  LOGGER_LOG_LEVEL_WARN},
        {"info",  LOGGER_LOG_LEVEL_INFO},
        {"debug", LOGGER_LOG_LEVEL_DEBUG},
        {"trace", LOGGER_LOG_LEVEL_TRACE},
        {"all",   LOGGER_LOG_LEVEL_ALL},
    };
    
    class CLIParser : public CLI::App {
    public:
        explicit CLIParser(std::string app_description = "", std::string app_name = "") : CLI::App(
            std::move(app_description), std::move(app_name)) {
        }
        
        void format() {
            std::vector<CLI::App*> parsers = {this};
            for (const auto& subcommand: subcommands_) {
                parsers.push_back(subcommand.get());
            }
            
            const auto column_width = 4 + std::ranges::max(
                parsers
                | std::views::transform([](CLI::App *parser) { return parser->get_options(); })
                | std::views::join
                | std::views::transform([](const auto &option) {
                    return std::format("  {} {}", option->get_name(true, true), option->get_option_text()).length();
                }));
            get_formatter()->column_width(column_width);
            for (auto parser: parsers) {
                for (auto option: parser->get_options()) {
                    if (option->get_expected_min() && !option->get_default_str().empty()) {
                        option->description(
                            std::format("{} (default: {})", option->get_description(), option->get_default_str()));
                    }
                }
                bool typed_arguments_exist = std::ranges::any_of(parser->get_options(), [](const auto &option) {
                    return !option->get_option_text().empty();
                });
                if (typed_arguments_exist) {
                    std::string footer = "Argument Types:\n";
                    for (const auto option: parser->get_options()) {
                        if (!option->get_expected_min()) {
                            continue;
                        }
                        const auto padding_length = column_width - option->get_option_text().length() - 2;
                        auto padding = std::string(padding_length, ' ');
                        if (option->get_single_name() == "host") {
                            footer += std::format("  {}{}IPv4 or IPv6 address in standard format\n", option->get_option_text(), padding);
                            continue;
                        }
                        if (option->get_single_name() == "log-level") {
                            std::vector<std::pair<std::string, logger_log_level>> log_levels(log_level_map.begin(), log_level_map.end());
                            std::ranges::sort(log_levels, [](const auto &a, const auto &b) { return a.second < b.second; });
                            std::string log_level_map_keys = std::accumulate(std::next(log_levels.begin()), log_levels.end(), log_levels[0].first,
                                                                             [](const std::string &a, const auto &b) {
                                                                                 return a + ", " + b.first;
                                                                             });
                            footer += std::format("  {}{}Enum value in: {{{}}}\n", option->get_option_text(), padding, log_level_map_keys);
                            continue;
                        }
                        if (option->get_option_text() == "OUTPUT_FILE") {
                            footer += std::format("  {}{}Path to the file\n", option->get_option_text(), padding);
                            continue;
                        }
                        std::string type_name = option->get_type_name();
                        type_name = std::regex_replace(type_name, std::regex(R"(TEXT:FILE)"), "Path to an existing file");
                        type_name = std::regex_replace(type_name, std::regex(R"(TEXT:DIR)"), "Path to an existing directory");
                        type_name = std::regex_replace(type_name, std::regex(R"(TEXT)"), "String of text");
                        type_name = std::regex_replace(type_name, std::regex(R"(\S*:?INT)"), "Integer number");
                        type_name = std::regex_replace(type_name, std::regex(R"(\S*:?FLOAT)"), "Decimal number");
                        type_name = std::regex_replace(type_name, std::regex(R"(\[(-?[0-9.]+) - (-?[0-9.]+)\])"), "range [$1, $2]");
                        type_name = std::regex_replace(type_name, std::regex( R"(ENUM:value in \{([a-zA-Z0-9_]+)->[0-9]+, ?([a-zA-Z0-9_]+)->[0-9]+\}.*)"), "Enum value in: {$1, $2}");
                        footer += std::format("  {}{}{}\n", option->get_option_text(), padding, type_name);
                    }
                    parser->footer(footer);
                }
            }
        }
        
    };
    
}

#endif //CLI_HPP
