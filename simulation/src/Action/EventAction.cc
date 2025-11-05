#include "G4Event.hh"
#include "G4SDManager.hh"
#include "G4SystemOfUnits.hh"
#include "g4root.hh"
#include "G4RunManager.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleGun.hh"
#include "G4PrimaryParticle.hh"
#include "G4PrimaryVertex.hh"
#include <vector>

#include "Action/EventAction.hh"
#include "Action/SteppingAction.hh"
#include "Util/Logger.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleTable.hh"
#include <set>

EventAction::EventAction() : fEventID(-1), fTotalEnergyDeposition(0.0) {}

EventAction::~EventAction() {}

void EventAction::BeginOfEventAction(const G4Event *event) {
    // Update event ID
    fEventID = event->GetEventID();
    
    // Initialize total energy deposition accumulator
    fTotalEnergyDeposition = 0.0;
    
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
                
                // Calculate incident angles
                G4double px = primary->GetPx();
                G4double py = primary->GetPy();
                G4double pz = primary->GetPz();
                
                // Calculate theta (polar angle) and phi (azimuthal angle)
                G4double momentum_magnitude = sqrt(px*px + py*py + pz*pz);
                G4double theta = 0.0;
                if (momentum_magnitude > 0) {
                    theta = acos(pz / momentum_magnitude);  // Angle relative to z-axis
                }
                G4double phi = atan2(py, px);  // Azimuthal angle in x-y plane
                
                // Set angles in OutputManager
                OutputManager::Instance()->SetInitialTheta(theta);
                OutputManager::Instance()->SetInitialPhi(phi);
                
                // Log minimal primary particle info
                std::stringstream primaryInfo;
                primaryInfo << "Primary particle: " << particleName 
                           << " with energy " << energy << " MeV"
                           << ", theta: " << theta << " rad, phi: " << phi << " rad";
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
    
    // Sum up the energy deposition from all sensitive detector hits
    fTotalEnergyDeposition = 0.0;
    
    // Get hits collections from OutputManager
    Hits* slabHits = OutputManager::Instance()->GetSLabHits();
    Hits* sipmHits = OutputManager::Instance()->GetHits();
    
    // Accumulate energy from SLab hits
    std::vector<Float_t> slabEnergy = slabHits->GetEnergy();
    for (const auto& energy : slabEnergy) {
        fTotalEnergyDeposition += energy;
    }
    
    // Accumulate energy from SiPM hits
    std::vector<Float_t> sipmEnergy = sipmHits->GetEnergy();
    for (const auto& energy : sipmEnergy) {
        fTotalEnergyDeposition += energy;
    }
    
    // Pass the total energy deposition to OutputManager
    OutputManager::Instance()->SetTotalEnergyDeposition(fTotalEnergyDeposition);
    
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
