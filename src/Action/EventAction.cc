#include "G4Event.hh"
#include "G4SDManager.hh"
#include "G4SystemOfUnits.hh"
#include "g4root.hh"
#include "G4RunManager.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleGun.hh"
#include "G4PrimaryParticle.hh"
#include "G4PrimaryVertex.hh"

#include "Action/EventAction.hh"
#include "Action/SteppingAction.hh"
#include "Util/Logger.hh"

EventAction::EventAction() : fEventID(-1) {}

EventAction::~EventAction() {}

void EventAction::BeginOfEventAction(const G4Event *event) {
    // Update event ID
    fEventID = event->GetEventID();
    
    // Log event start
    LogUtils::log_info("Starting Event ID: " + std::to_string(fEventID));

    // Reset all statistics for the new event
    LogUtils::reset_slab_statistics();
    LogUtils::reset_detection_statistics();
    LogUtils::reset_unique_statistics();  // Add this line for unique particles

    // Set random seeds
    long fEventSeedIndex = CLHEP::HepRandom::getTheSeed();
    auto fEventSeed1 = CLHEP::HepRandom::getTheSeeds()[0];
    auto fEventSeed2 = CLHEP::HepRandom::getTheSeeds()[1];

    long fEventSeedsArray[2] = {fEventSeed1, fEventSeed2};
    const long int* fEventSeeds = fEventSeedsArray;
    G4Random::setTheSeeds(fEventSeeds,fEventSeedIndex);

    // Log the random seeds (debug level)
    std::stringstream seedInfo;
    seedInfo << "Event " << fEventID << " random seeds: " 
             << fEventSeed1 << ", " << fEventSeed2;
    LogUtils::log_debug(seedInfo.str(), false);

    // Log primary particle information
    if (event->GetNumberOfPrimaryVertex() > 0) {
        G4PrimaryVertex* vertex = event->GetPrimaryVertex();
        if (vertex && vertex->GetNumberOfParticle() > 0) {
            G4PrimaryParticle* primary = vertex->GetPrimary();
            if (primary && primary->GetG4code()) {
                // Get particle info
                G4String particleName = primary->GetG4code()->GetParticleName();
                G4double energy = primary->GetKineticEnergy() / CLHEP::MeV;
                
                // Log minimal primary particle info
                std::stringstream primaryInfo;
                primaryInfo << "Primary particle: " << particleName 
                           << " with energy " << energy << " MeV";
                LogUtils::log_info(primaryInfo.str());
            }
        }
    }

    OutputManager::Instance()->SetEventID(fEventID);
}

void EventAction::EndOfEventAction(const G4Event* event)
{
    // Log completion
    LogUtils::log_info("End of event " + std::to_string(event->GetEventID()));
    
    // First log unique particle statistics
    LogUtils::log_unique_statistics(event->GetEventID());
    
    // Then log hit statistics 
    LogUtils::log_detection_statistics(event->GetEventID());
    
    // For the last event, add detailed particle summary
    if (event->GetEventID() == G4RunManager::GetRunManager()->GetNumberOfEventsToBeProcessed() - 1) {
        LogUtils::log_detailed_particle_summary();
    }
    
    OutputManager::Instance()->EndOfEvent();
}
