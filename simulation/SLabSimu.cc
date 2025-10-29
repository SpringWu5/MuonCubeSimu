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
    // ��ʼ����־ϵͳ
    LogUtils::initialize_logging();
    
    bool gui = false;
    const char *config = "../SLabSimu/config/config.yaml";
    const char *output = "output.root";
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
    
    // For high-energy muon beam simulation, we don't need to load particle json file
    spdlog::info("Setting up high-energy muon beam simulation (10-40 GeV)");
    
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
