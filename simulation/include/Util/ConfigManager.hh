#ifndef CONFIG_MANAGER_HH
#define CONFIG_MANAGER_HH

#include "yaml-cpp/yaml.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json-schema.hpp"
#include <string>
#include <memory>

using json = nlohmann::json;

class ConfigManager {
public:
    // Singleton pattern - Get the instance of ConfigManager
    static ConfigManager* Instance();

    // Load configuration from YAML file and validate against schema
    bool LoadConfig(const std::string& config_path, const std::string& schema_path);

    // Get a specific configuration node by key
    YAML::Node GetNode(const std::string& key) const;

    // Get the root configuration node
    const YAML::Node& GetRootNode() const { return fRootNode; }

    // Helper function to convert YAML::Node to nlohmann::json
    static nlohmann::json YamlToJson(const YAML::Node& yaml_node);

private:
    // Private constructor for singleton pattern
    ConfigManager();
    
    // Private destructor
    ~ConfigManager();
    
    // Delete copy constructor and assignment operator to enforce singleton
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    // Root YAML node to store the loaded configuration
    YAML::Node fRootNode;
    
    // JSON schema for validation
    nlohmann::json_schema::json_validator fValidator;
    
    // Flag to track if configuration is loaded
    bool fIsConfigLoaded;
};

#endif // CONFIG_MANAGER_HH