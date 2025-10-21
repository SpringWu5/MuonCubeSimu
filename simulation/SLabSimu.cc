#include "spdlog/spdlog.h"
#include "G4Types.hh"
#include "G4RunManager.hh"
#include "G4UIExecutive.hh"
#include "G4VisExecutive.hh"
#include "G4UImanager.hh"
#include "yaml-cpp/yaml.h"
#include "nlohmann/json.hpp"
#include "Util/Logger.hh"

#include "DetectorConstruction/SLabBuilder.hh"
#include "PhysicsList/PhysicsList.hh"
#include "Action/ActionInitialization.hh"
#include "Record/OutputManager.hh"

using json = nlohmann::json;
using std::string;

int main( [[maybe_unused]] int argc, [[maybe_unused]] char **argv)
{
    // 初始化日志系统
    LogUtils::initialize_logging();
    
    bool gui = false;
    const char *config = "../SLabSimu/config/config.yaml";
    const char *output = "output.root";
    json particle_list;
    int n_events = 1;
    if (argc==2 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
        spdlog::info("Usage: SLabSimu config.yaml output.root ");
        return 0;
    } else if (argc==1){
        spdlog::info("No argument is given, running in GUI mode");
        gui = true;
    } else {
        spdlog::info("Running in batch mode");
        config = argv[1];
        output = argc>2 ? argv[2] : output;
    }
    
    // Load config file
    auto config_root_node = YAML::LoadFile(config);
    
    // Load particle data for both GUI and batch modes
    string particle_json_path = config_root_node["Particles"]["particle_file_path"].as<string>();
    std::ifstream particles_file;
    particles_file.open(particle_json_path.c_str());
    if (!particles_file.is_open()) {
        spdlog::error("Failed to open particle json file: {:s}", particle_json_path);
        throw;
    } else {
        spdlog::info("Particle json file opened: {:s}", particle_json_path);
        particles_file >> particle_list;
    }
    
    if (!gui) {
        n_events = config_root_node["Run"]["number_of_events"].as<int>();
        if (n_events < 0 || n_events > int(particle_list.size())){
            n_events = int(particle_list.size());
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

    // User action class
    ActionInitialization *action = new ActionInitialization(particle_list);
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
