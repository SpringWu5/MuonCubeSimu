#include "Action/ActionInitialization.hh"
#include "Action/PrimaryGeneratorAction.hh"
#include "Action/RunAction.hh"
#include "Action/EventAction.hh"
#include "Action/SteppingAction.hh"
#include "spdlog/spdlog.h"
#include "Util/Logger.hh"

ActionInitialization::ActionInitialization()
    : fPga(nullptr),
      fEventAction(nullptr),
      fRunAction(nullptr) {}

void ActionInitialization::Build() const
{
    // Create a new timestamped log file for this run
    LogUtils::initialize_logging();
    
    // Log the particle information from the particle list
    LogUtils::log_info("=== High-Energy Muon Beam Configuration ===");
    LogUtils::log_info("Setting up muon beam generation with 10-40 GeV energy range");
    
    // Set up the actions as before
    SetUserAction(new PrimaryGeneratorAction()); // No particle list needed for new beam generation
    
    RunAction* runAction = new RunAction();
    SetUserAction(runAction);
    
    EventAction* eventAction = new EventAction();
    SetUserAction(eventAction);
    
    // // disable stepping action for now since cost too much memory
    // SetUserAction(new SteppingAction(eventAction));
    
    spdlog::info("Action initialization completed");
    LogUtils::log_info("Action initialization completed");
}
