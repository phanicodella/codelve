// E:\CodeLve\src\utils\config.h
#pragma once

#include <string>
#include <map>
#include <memory>
#include <variant>
#include <vector>

namespace codelve {
    namespace utils {

        class Config {
        public:
            using ConfigValue = std::variant<int, double, bool, std::string, std::vector<std::string>>;
            using ConfigMap = std::map<std::string, ConfigValue>;

            static Config& getInstance();

            // Make constructor and destructor public
            Config();
            ~Config();

            bool loadFromFile(const std::string& configFile);
            bool save();
            bool saveAs(const std::string& configFile);

            void set(const std::string& key, const ConfigValue& value);
            bool hasKey(const std::string& key) const;
            int getInt(const std::string& key, int defaultValue = 0) const;
            double getFloat(const std::string& key, double defaultValue = 0.0) const;
            bool getBool(const std::string& key, bool defaultValue = false) const;
            std::string getString(const std::string& key, const std::string& defaultValue = "") const;
            std::vector<std::string> getStringList(const std::string& key, const std::vector<std::string>& defaultValue = {}) const;

        private:
            // Remove constructor and destructor from here

            ConfigMap data_;
            std::string configFile_;

            bool parse(const std::string& content);
            std::string serialize() const;
        };

    } // namespace utils
} // namespace codelve