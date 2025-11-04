#include "spdlog/spdlog.h"
#include "G4Types.hh"
#include "G4RunManager.hh"
#include "G4UIExecutive.hh"
#include "G4VisExecutive.hh"
#include "G4UImanager.hh"
#include "yaml-cpp/yaml.h"
#include "nlohmann/json.hpp"
#include "Util/Logger.hh"
#include <fstream>
#include <streambuf>

#include "DetectorConstruction/SLabBuilder.hh"
#include "PhysicsList/PhysicsList.hh"
#include "Action/ActionInitialization.hh"
#include "Record/OutputManager.hh"

using json = nlohmann::json;
using std::string;

// Helper function to convert YAML::Node to nlohmann::json
nlohmann::json yaml_to_json(const YAML::Node& yaml_node) {
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
                json_array.push_back(yaml_to_json(item));
            }
            return json_array;
        }
        case YAML::NodeType::Map: {
            nlohmann::json json_obj = nlohmann::json::object();
            for (const auto& item : yaml_node) {
                json_obj[item.first.as<std::string>()] = yaml_to_json(item.second);
            }
            return json_obj;
        }
        default:
            return nullptr;
    }
}

// Function to validate config against schema (robust implementation using manual validation)
bool validate_config_against_schema(const std::string& config_path, const std::string& schema_path) {
    try {
        // Load the YAML configuration
        YAML::Node yaml_config = YAML::LoadFile(config_path);
        
        // Convert YAML to JSON for validation using helper function
        nlohmann::json config_json = yaml_to_json(yaml_config);
        
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
        
        // Perform custom validation based on the schema structure
        // Check required top-level properties exist
        if (schema_json.contains("required") && schema_json["required"].is_array()) {
            for (const auto& required_key : schema_json["required"]) {
                std::string key = required_key.get<std::string>();
                if (config_json.find(key) == config_json.end()) {
                    spdlog::error("Configuration missing required key: {}", key);
                    return false;
                }
            }
        }
        
        // Validate required properties in particle_gun section
        if (config_json.contains("particle_gun") && schema_json["properties"]["particle_gun"].contains("required")) {
            for (const auto& required_key : schema_json["properties"]["particle_gun"]["required"]) {
                std::string key = required_key.get<std::string>();
                if (config_json["particle_gun"].find(key) == config_json["particle_gun"].end()) {
                    spdlog::error("Configuration missing required key in particle_gun: {}", key);
                    return false;
                }
            }
        }
        
        // Validate required properties in detector section
        if (config_json.contains("detector") && schema_json["properties"]["detector"].contains("required")) {
            for (const auto& required_key : schema_json["properties"]["detector"]["required"]) {
                std::string key = required_key.get<std::string>();
                if (config_json["detector"].find(key) == config_json["detector"].end()) {
                    spdlog::error("Configuration missing required key in detector: {}", key);
                    return false;
                }
            }
        }
        
        // Validate required properties in physics section
        if (config_json.contains("physics") && schema_json["properties"]["physics"].contains("required")) {
            for (const auto& required_key : schema_json["properties"]["physics"]["required"]) {
                std::string key = required_key.get<std::string>();
                if (config_json["physics"].find(key) == config_json["physics"].end()) {
                    spdlog::error("Configuration missing required key in physics: {}", key);
                    return false;
                }
            }
        }
        
        // Validate required properties in output section
        if (config_json.contains("output") && schema_json["properties"]["output"].contains("required")) {
            for (const auto& required_key : schema_json["properties"]["output"]["required"]) {
                std::string key = required_key.get<std::string>();
                if (config_json["output"].find(key) == config_json["output"].end()) {
                    spdlog::error("Configuration missing required key in output: {}", key);
                    return false;
                }
            }
        }
        
        spdlog::info("Configuration validation passed");
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Configuration validation failed: {}", e.what());
        return false;
    }
}

int main(int argc, char **argv)
{
    // Initialize logging system
    LogUtils::initialize_logging();
    
    // Hardcode config file path - zero-argument execution
    const char *config = "../SLabSimu/config/config.yaml";
    const char *output = "output.root";  // Default, may be overridden by config
    int n_events = 1;
    
    // Check for GUI mode argument
    bool gui = false;
    if (argc == 2 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
        spdlog::info("Usage: SLabSimu  (runs with hardcoded config.yaml)");
        return 0;
    } else if (argc == 2 && std::string(argv[1]) == "-g") {
        spdlog::info("Running in GUI mode");
        gui = true;
    } else if (argc > 1) {
        spdlog::error("This executable accepts zero arguments (default batch mode) or -g for GUI mode.");
        spdlog::info("Usage: SLabSimu  (runs with hardcoded config.yaml)");
        spdlog::info("   or: SLabSimu -g  (GUI mode)");
        return 1;
    } else {
        spdlog::info("Running in batch mode with hardcoded config");
    }

    // Validate config against schema
    if (!validate_config_against_schema(config, "../SLabSimu/config/config_schema.json")) {
        spdlog::error("Configuration validation failed. Exiting.");
        return 1;
    }
    
    // Load config file
    auto config_root_node = YAML::LoadFile(config);
    
    // For high-energy muon beam simulation, we don't need to load particle json file
    spdlog::info("Setting up high-energy muon beam simulation (10-40 GeV)");
    
    // Set output filename from config
    if (config_root_node["output"] && config_root_node["output"]["filename"]) {
        output = config_root_node["output"]["filename"].as<std::string>().c_str();
    } else {
        spdlog::warn("Output filename not specified in config, using default: output.root");
    }
    
    if (!gui) {
        n_events = config_root_node["Run"]["number_of_events"].as<int>();
    }

    // Initialize output manager
    OutputManager::Instance()->Book(output);

    // set random number for G4
    G4Random::setTheEngine(new CLHEP::RanecuEngine());
    G4Random::setTheSeed(42);

    // run manager
    G4RunManager *runManager = new G4RunManager;

    // set detector
    SLabBuilder *detector = new SLabBuilder(config);
    runManager->SetUserInitialization(detector);

    // User physics list
    G4VModularPhysicsList *physicsList = new PhysicsList();
    runManager->SetUserInitialization(physicsList);

    // User action class - no particle list needed for new beam generation
    ActionInitialization *action = new ActionInitialization();
    runManager->SetUserInitialization(action);

    runManager->Initialize();

    if (gui) {
        // Initialize visualization
        G4VisManager* visManager = new G4VisExecutive();
        visManager->Initialize();

        // Get the pointer to the User Interface manager
        G4UImanager* UImanager = G4UImanager::GetUIpointer();

        // Optionally initialize a user interface
        G4UIExecutive* ui = new G4UIExecutive(argc, argv);

        // Execute the visualization macro
        UImanager->ApplyCommand("/control/execute ../SLabSimu/config/vis.mac"); 
        ui->SessionStart();
        OutputManager::Instance()->Save();
        delete ui;
        delete visManager;
    } else{
        // run simulation
        runManager->BeamOn(n_events);

        OutputManager::Instance()->Save();
    }    
    delete runManager;

    return 0;
}