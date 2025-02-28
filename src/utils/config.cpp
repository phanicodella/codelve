// E:\CodeLve\src\utils\config.cpp
#include "config.h"
#include "logger.h"
#include <fstream>
#include <sstream>
#include <regex>

namespace codelve {
    namespace utils {

        Config::Config() {
        }

        Config::~Config() {
        }

        Config& Config::getInstance() {
            static Config instance;
            return instance;
        }

        bool Config::loadFromFile(const std::string& configFile) {
            configFile_ = configFile;

            std::ifstream file(configFile);
            if (!file.is_open()) {
                LOG_ERROR("Failed to open config file: " + configFile);
                return false;
            }

            std::stringstream buffer;
            buffer << file.rdbuf();

            return parse(buffer.str());
        }

        bool Config::save() {
            return saveAs(configFile_);
        }

        bool Config::saveAs(const std::string& configFile) {
            std::ofstream file(configFile);
            if (!file.is_open()) {
                LOG_ERROR("Failed to open config file for writing: " + configFile);
                return false;
            }

            file << serialize();
            return true;
        }

        void Config::set(const std::string& key, const ConfigValue& value) {
            data_[key] = value;
        }

        bool Config::hasKey(const std::string& key) const {
            return data_.find(key) != data_.end();
        }

        int Config::getInt(const std::string& key, int defaultValue) const {
            if (!hasKey(key)) {
                return defaultValue;
            }

            try {
                return std::get<int>(data_.at(key));
            }
            catch (const std::bad_variant_access&) {
                LOG_WARNING("Config key " + key + " is not an integer type");
                return defaultValue;
            }
        }

        double Config::getFloat(const std::string& key, double defaultValue) const {
            if (!hasKey(key)) {
                return defaultValue;
            }

            try {
                return std::get<double>(data_.at(key));
            }
            catch (const std::bad_variant_access&) {
                LOG_WARNING("Config key " + key + " is not a float type");
                return defaultValue;
            }
        }

        bool Config::getBool(const std::string& key, bool defaultValue) const {
            if (!hasKey(key)) {
                return defaultValue;
            }

            try {
                return std::get<bool>(data_.at(key));
            }
            catch (const std::bad_variant_access&) {
                LOG_WARNING("Config key " + key + " is not a boolean type");
                return defaultValue;
            }
        }

        std::string Config::getString(const std::string& key, const std::string& defaultValue) const {
            if (!hasKey(key)) {
                return defaultValue;
            }

            try {
                return std::get<std::string>(data_.at(key));
            }
            catch (const std::bad_variant_access&) {
                LOG_WARNING("Config key " + key + " is not a string type");
                return defaultValue;
            }
        }

        std::vector<std::string> Config::getStringList(const std::string& key, const std::vector<std::string>& defaultValue) const {
            if (!hasKey(key)) {
                return defaultValue;
            }

            try {
                return std::get<std::vector<std::string>>(data_.at(key));
            }
            catch (const std::bad_variant_access&) {
                LOG_WARNING("Config key " + key + " is not a string list type");
                return defaultValue;
            }
        }

        bool Config::parse(const std::string& content) {
            std::regex keyValueRegex(R"(^\s*([a-zA-Z0-9_\.]+)\s*=\s*(.+)$)");
            std::regex sectionRegex(R"(^\s*\[([a-zA-Z0-9_\.]+)\]\s*$)");

            std::string currentSection;
            std::istringstream stream(content);
            std::string line;

            while (std::getline(stream, line)) {
                // Skip comments and empty lines
                if (line.empty() || line[0] == '#' || line[0] == ';') {
                    continue;
                }

                std::smatch match;

                // Check if this is a section header
                if (std::regex_match(line, match, sectionRegex)) {
                    currentSection = match[1].str();
                    continue;
                }

                // Check if this is a key-value pair
                if (std::regex_match(line, match, keyValueRegex)) {
                    std::string key = match[1].str();
                    std::string value = match[2].str();

                    // If we're in a section, prefix the key with the section name
                    if (!currentSection.empty()) {
                        key = currentSection + "." + key;
                    }

                    // Try to determine the type of the value
                    if (value == "true" || value == "yes" || value == "on") {
                        data_[key] = true;
                    }
                    else if (value == "false" || value == "no" || value == "off") {
                        data_[key] = false;
                    }
                    else if (std::regex_match(value, std::regex(R"(^-?\d+$)"))) {
                        data_[key] = std::stoi(value);
                    }
                    else if (std::regex_match(value, std::regex(R"(^-?\d+\.\d+$)"))) {
                        data_[key] = std::stod(value);
                    }
                    else {
                        // Remove quotes if present
                        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                            value = value.substr(1, value.size() - 2);
                        }
                        data_[key] = value;
                    }
                }
            }

            return true;
        }

        std::string Config::serialize() const {
            std::stringstream output;
            std::map<std::string, std::stringstream> sections;

            // Sort keys into sections
            for (const auto& pair : data_) {
                size_t dotPos = pair.first.find('.');
                if (dotPos != std::string::npos) {
                    std::string section = pair.first.substr(0, dotPos);
                    std::string key = pair.first.substr(dotPos + 1);

                    std::visit([&sections, &key](const auto& value) {
                        using T = std::decay_t<decltype(value)>;
                        if constexpr (std::is_same_v<T, int>) {
                            sections[key] << key << " = " << value << "\n";
                        }
                        else if constexpr (std::is_same_v<T, double>) {
                            sections[key] << key << " = " << value << "\n";
                        }
                        else if constexpr (std::is_same_v<T, bool>) {
                            sections[key] << key << " = " << (value ? "true" : "false") << "\n";
                        }
                        else if constexpr (std::is_same_v<T, std::string>) {
                            sections[key] << key << " = \"" << value << "\"\n";
                        }
                        else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                            // Not implemented yet - would need to serialize as comma-separated list
                            sections[key] << "# String list not implemented\n";
                        }
                        }, pair.second);
                }
                else {
                    // Keys without a section go into the global section
                    std::visit([&output, &pair](const auto& value) {
                        using T = std::decay_t<decltype(value)>;
                        if constexpr (std::is_same_v<T, int>) {
                            output << pair.first << " = " << value << "\n";
                        }
                        else if constexpr (std::is_same_v<T, double>) {
                            output << pair.first << " = " << value << "\n";
                        }
                        else if constexpr (std::is_same_v<T, bool>) {
                            output << pair.first << " = " << (value ? "true" : "false") << "\n";
                        }
                        else if constexpr (std::is_same_v<T, std::string>) {
                            output << pair.first << " = \"" << value << "\"\n";
                        }
                        else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                            // Not implemented yet
                            output << "# String list not implemented\n";
                        }
                        }, pair.second);
                }
            }

            // Add sections to the output
            for (const auto& section : sections) {
                output << "\n[" << section.first << "]\n" << section.second.str();
            }

            return output.str();
        }

    } // namespace utils
} // namespace codelve