// File: codelve/src/utils/config.cpp
#include "utils/config.h"
#include "utils/logger.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

// For now, we'll use a simple implementation.
// Later, we'll integrate a JSON library like nlohmann/json

namespace codelve {
namespace utils {

Config::Config(const std::string& configFile)
    : configFile_(configFile) {
    LOG_INFO("Config created for file: " + configFile);
}

Config::~Config() {
    LOG_INFO("Config destroyed");
}

bool Config::load() {
    LOG_INFO("Loading configuration from: " + configFile_);
    
    try {
        // For now, we'll use default values since we don't have a JSON parser yet
        // We'll update this implementation later
        
        // Default application settings
        data_["app.name"] = std::string("CodeLve");
        data_["app.version"] = std::string("0.1.0");
        data_["app.log_level"] = std::string("INFO");
        
        // Default model settings
        data_["model.path"] = std::string("models/codellama-34b-instruct.Q4_K_M.gguf");
        data_["model.context_size"] = 8192;
        data_["model.gpu_layers"] = 0;
        data_["model.threads"] = 4;
        
        // Default UI settings
        data_["ui.theme"] = std::string("light");
        data_["ui.font_size"] = 12;
        data_["ui.window_width"] = 1200;
        data_["ui.window_height"] = 800;
        
        // Default scanner settings
        data_["scanner.max_file_size"] = 1048576;  // 1MB
        data_["scanner.ignore_dirs"] = std::vector<std::string>{".git", "node_modules", "build", "bin", "obj"};
        data_["scanner.supported_extensions"] = std::vector<std::string>{".c", ".cpp", ".h", ".hpp", ".cs", ".js", ".ts", ".py", ".java"};
        
        LOG_INFO("Configuration loaded with default values");
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Failed to load configuration: " + std::string(e.what()));
        return false;
    }
}

bool Config::save() {
    LOG_INFO("Saving configuration to: " + configFile_);
    
    try {
        // For now, we'll just log that we're saving
        // We'll implement this later when we add a JSON library
        
        LOG_INFO("Configuration save not yet implemented");
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Failed to save configuration: " + std::string(e.what()));
        return false;
    }
}

std::string Config::getString(const std::string& key, const std::string& defaultValue) const {
    if (hasKey(key)) {
        try {
            return std::any_cast<std::string>(data_.at(key));
        }
        catch (const std::bad_any_cast&) {
            LOG_WARNING("Type mismatch for key: " + key);
        }
    }
    return defaultValue;
}

int Config::getInt(const std::string& key, int defaultValue) const {
    if (hasKey(key)) {
        try {
            return std::any_cast<int>(data_.at(key));
        }
        catch (const std::bad_any_cast&) {
            LOG_WARNING("Type mismatch for key: " + key);
        }
    }
    return defaultValue;
}

bool Config::getBool(const std::string& key, bool defaultValue) const {
    if (hasKey(key)) {
        try {
            return std::any_cast<bool>(data_.at(key));
        }
        catch (const std::bad_any_cast&) {
            LOG_WARNING("Type mismatch for key: " + key);
        }
    }
    return defaultValue;
}

double Config::getDouble(const std::string& key, double defaultValue) const {
    if (hasKey(key)) {
        try {
            return std::any_cast<double>(data_.at(key));
        }
        catch (const std::bad_any_cast&) {
            LOG_WARNING("Type mismatch for key: " + key);
        }
    }
    return defaultValue;
}

void Config::setString(const std::string& key, const std::string& value) {
    data_[key] = value;
}

void Config::setInt(const std::string& key, int value) {
    data_[key] = value;
}

void Config::setBool(const std::string& key, bool value) {
    data_[key] = value;
}

void Config::setDouble(const std::string& key, double value) {
    data_[key] = value;
}

bool Config::hasKey(const std::string& key) const {
    return data_.find(key) != data_.end();
}

}} // namespace codelve::utils