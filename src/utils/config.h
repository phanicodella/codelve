// File: codelve/src/utils/config.h
#pragma once

#include <string>
#include <map>
#include <vector>
#include <any>
#include <memory>

namespace codelve {
namespace utils {

/**
 * Configuration class for handling application settings.
 * Loads settings from JSON files and provides access to them.
 */
class Config {
public:
    /**
     * Constructor.
     * @param configFile Path to the configuration file
     */
    explicit Config(const std::string& configFile);
    
    /**
     * Destructor.
     */
    ~Config();
    
    /**
     * Load configuration from file.
     * @return true if loading was successful, false otherwise
     */
    bool load();
    
    /**
     * Save configuration to file.
     * @return true if saving was successful, false otherwise
     */
    bool save();
    
    /**
     * Get string value from configuration.
     * @param key Configuration key
     * @param defaultValue Default value if key doesn't exist
     * @return String value
     */
    std::string getString(const std::string& key, const std::string& defaultValue = "") const;
    
    /**
     * Get integer value from configuration.
     * @param key Configuration key
     * @param defaultValue Default value if key doesn't exist
     * @return Integer value
     */
    int getInt(const std::string& key, int defaultValue = 0) const;
    
    /**
     * Get boolean value from configuration.
     * @param key Configuration key
     * @param defaultValue Default value if key doesn't exist
     * @return Boolean value
     */
    bool getBool(const std::string& key, bool defaultValue = false) const;
    
    /**
     * Get double value from configuration.
     * @param key Configuration key
     * @param defaultValue Default value if key doesn't exist
     * @return Double value
     */
    double getDouble(const std::string& key, double defaultValue = 0.0) const;
    
    /**
     * Set string value in configuration.
     * @param key Configuration key
     * @param value String value
     */
    void setString(const std::string& key, const std::string& value);
    
    /**
     * Set integer value in configuration.
     * @param key Configuration key
     * @param value Integer value
     */
    void setInt(const std::string& key, int value);
    
    /**
     * Set boolean value in configuration.
     * @param key Configuration key
     * @param value Boolean value
     */
    void setBool(const std::string& key, bool value);
    
    /**
     * Set double value in configuration.
     * @param key Configuration key
     * @param value Double value
     */
    void setDouble(const std::string& key, double value);
    
    /**
     * Check if configuration contains key.
     * @param key Configuration key
     * @return true if key exists, false otherwise
     */
    bool hasKey(const std::string& key) const;

private:
    // Path to configuration file
    std::string configFile_;
    
    // Configuration data
    std::map<std::string, std::any> data_;
};

}} // namespace codelve::utils