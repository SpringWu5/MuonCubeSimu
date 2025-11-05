#include "Util/ConfigManager.hh"
#include "spdlog/spdlog.h"
#include <fstream>
#include <streambuf>
#include <stdexcept>

// Static instance of ConfigManager for singleton pattern
static std::unique_ptr<ConfigManager> instance = nullptr;

ConfigManager* ConfigManager::Instance() {
    if (!instance) {
        instance = std::unique_ptr<ConfigManager>(new ConfigManager());
    }
    return instance.get();
}

ConfigManager::ConfigManager() : fIsConfigLoaded(false) {
    // Initialize the instance if not already done
    if (!instance) {
        instance.reset(this);
    }
}

ConfigManager::~ConfigManager() = default;

bool ConfigManager::LoadConfig(const std::string& config_path, const std::string& schema_path) {
    try {
        // Load the YAML configuration
        fRootNode = YAML::LoadFile(config_path);
        
        // Convert YAML to JSON for validation using helper function
        nlohmann::json config_json = YamlToJson(fRootNode);
        
        // Load the schema
        std::ifstream schema_file(schema_path);
        if (!schema_file.is_open()) {
            spdlog::error("Could not open schema file: {}", schema_path);
            return false;
        }
        
        std::string schema_str((std::istreambuf_iterator<char>(schema_file)),
                              std::istreambuf_iterator<char>());
        schema_file.close();
        
        nlohmann::json schema_json = nlohmann::json::parse(schema_str);
        
        // Create a schema validator
        nlohmann::json_schema::json_validator validator;
        try {
            validator.set_root_schema(schema_json);
        } catch (const std::exception& e) {
            spdlog::error("Schema validation failed: {}", e.what());
            return false;
        }
        
        // Perform validation
        try {
            validator.validate(config_json);
            spdlog::info("Configuration validation passed");
        } catch (const std::exception& e) {
            spdlog::error("Configuration validation failed: {}", e.what());
            return false;
        }
        
        fIsConfigLoaded = true;
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Configuration loading failed: {}", e.what());
        return false;
    }
}

YAML::Node ConfigManager::GetNode(const std::string& key) const {
    if (!fIsConfigLoaded) {
        spdlog::error("Configuration not loaded. Cannot retrieve node: {}", key);
        return YAML::Node();
    }
    
    // Parse the key path (e.g., "particle_gun.energy_distribution.type")
    YAML::Node current = fRootNode;
    size_t start = 0;
    size_t end = key.find('.');
    
    while (end != std::string::npos) {
        std::string segment = key.substr(start, end - start);
        current = current[segment];
        start = end + 1;
        end = key.find('.', start);
    }
    
    // Get the final segment
    std::string final_segment = key.substr(start);
    return current[final_segment];
}

nlohmann::json ConfigManager::YamlToJson(const YAML::Node& yaml_node) {
    switch (yaml_node.Type()) {
        case YAML::NodeType::Null:
            return nullptr;
        case YAML::NodeType::Scalar:
            // Check if it's a number, boolean, or string
            try {
                // Try to convert to int
                int int_val = yaml_node.as<int>();
                std::string str_val = yaml_node.as<std::string>();
                if (std::to_string(int_val) == str_val) {
                    return int_val;
                }
            } catch (...) {
                try {
                    // Try to convert to double
                    double double_val = yaml_node.as<double>();
                    std::string str_val = yaml_node.as<std::string>();
                    // Simple check to see if it's actually a double
                    if (str_val.find('.') != std::string::npos || 
                        str_val.find('e') != std::string::npos || 
                        str_val.find('E') != std::string::npos) {
                        return double_val;
                    }
                } catch (...) {
                    // Try to convert to bool
                    try {
                        std::string val = yaml_node.as<std::string>();
                        if (val == "true" || val == "True" || val == "TRUE") {
                            return true;
                        } else if (val == "false" || val == "False" || val == "FALSE") {
                            return false;
                        }
                    } catch (...) {
                        // It's a string
                    }
                    return yaml_node.as<std::string>();
                }
            }
            // If it's a number that didn't match above, return as string
            return yaml_node.as<std::string>();
        case YAML::NodeType::Sequence: {
            nlohmann::json json_array = nlohmann::json::array();
            for (const auto& item : yaml_node) {
                json_array.push_back(YamlToJson(item));
            }
            return json_array;
        }
        case YAML::NodeType::Map: {
            nlohmann::json json_obj = nlohmann::json::object();
            for (const auto& item : yaml_node) {
                json_obj[item.first.as<std::string>()] = YamlToJson(item.second);
            }
            return json_obj;
        }
        default:
            return nullptr;
    }
}