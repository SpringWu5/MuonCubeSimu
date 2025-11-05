#include "spdlog/spdlog.h"
#include "G4Types.hh"
#include "G4RunManager.hh"
#include "G4UIExecutive.hh"
#include "G4VisExecutive.hh"
#include "G4UImanager.hh"
#include "yaml-cpp/yaml.h"
#include "nlohmann/json.hpp"
#include "Util/Logger.hh"
#include "Util/ConfigManager.hh"
#include <fstream>
#include <streambuf>

#include "DetectorConstruction/SLabBuilder.hh"
#include "PhysicsList/PhysicsList.hh"
#include "Action/ActionInitialization.hh"
#include "Record/OutputManager.hh"

using json = nlohmann::json;
using std::string;

int main(int argc, char **argv)
{
    // Initialize logging system
    LogUtils::initialize_logging();
    
    // Hardcode config file path - zero-argument execution
    const char *config = "../config/config.yaml";
    const char *schema = "../config/config_schema.json";
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

    // Load and validate config using ConfigManager
    if (!ConfigManager::Instance()->LoadConfig(config, schema)) {
        spdlog::error("Configuration validation failed. Exiting.");
        return 1;
    }
    
    // For high-energy muon beam simulation, we don't need to load particle json file
    spdlog::info("Setting up high-energy muon beam simulation (10-40 GeV)");
    
    // Set output filename from config using ConfigManager
    auto output_node = ConfigManager::Instance()->GetNode("output.filename");
    if (output_node) {
        output = output_node.as<std::string>().c_str();
    } else {
        spdlog::warn("Output filename not specified in config, using default: output.root");
    }
    
    if (!gui) {
        auto events_node = ConfigManager::Instance()->GetNode("Run.number_of_events");
        if (events_node) {
            n_events = events_node.as<int>();
        } else {
            spdlog::error("Number of events not specified in config. Exiting.");
            return 1;
        }
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
        UImanager->ApplyCommand("/control/execute ../config/vis.mac"); 
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