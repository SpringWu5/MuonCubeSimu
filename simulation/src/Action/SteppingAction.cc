#include "Action/SteppingAction.hh"
#include "Action/EventAction.hh"

#include "G4Step.hh"
#include "G4RunManager.hh"
#include "G4Event.hh"
#include "G4Track.hh"
#include "G4OpticalPhoton.hh"
#include "G4SystemOfUnits.hh"
#include "spdlog/spdlog.h"
#include "Util/Logger.hh"
#include <fstream>
#include <iomanip>
#include <ctime>
#include <vector>

// Storage for photon statistics across events
namespace {
    std::vector<int> g_cerenkov_counts;
    std::vector<int> g_scintillation_counts;
}

SteppingAction::SteppingAction(EventAction* eventAction)
: G4UserSteppingAction(),
  fEventAction(eventAction),
  cerenkovCount(0),
  scintillationCount(0),
  currentEventID(-1)
{
    // Initialize photon counting using standardized logging
    LogUtils::log_info("Optical photon counting initialized");
}

SteppingAction::~SteppingAction()
{
    // Ensure statistics for the last event are output
    OutputFinalStatistics();
}

void SteppingAction::OutputFinalStatistics()
{
    // This function will output statistics for the last event
    if (currentEventID >= 0) {
        OutputPhotonStatistics(currentEventID, cerenkovCount, scintillationCount);
    }
    
    // Output the final particle statistics
    LogUtils::log_info("\n=== Final Particle Statistics ===");
    for (const auto& [particle, count] : particleCount) {
        LogUtils::log_particle_statistics(particle, count);
    }
    
    // Create a map for the finalize_logging function
    std::map<std::string, int> particleCountsStd;
    for (const auto& [particle, count] : particleCount) {
        particleCountsStd[particle] = count;
    }
    
    // Get total events - use a simpler approach to avoid compile errors
    G4int totalEvents = currentEventID + 1;
    if (totalEvents <= 0) totalEvents = 1; // Safety check
    
    // Finalize the logging with comprehensive summary
    LogUtils::finalize_logging(totalEvents, particleCountsStd, g_cerenkov_counts, g_scintillation_counts);
}

// Modified parameter names to avoid variable shadowing
void SteppingAction::OutputPhotonStatistics(G4int eventID, G4int cerCount, G4int scintCount)
{
    // Store counts for summary statistics
    g_cerenkov_counts.push_back(cerCount);
    g_scintillation_counts.push_back(scintCount);
    
    // Use the enhanced logging system
    LogUtils::log_photon_statistics(eventID, cerCount, scintCount);
}

void SteppingAction::UserSteppingAction(const G4Step* step)
{
    // Get current event ID safely
    G4int eventID = -1;
    const G4Event* currentEvent = G4RunManager::GetRunManager()->GetCurrentEvent();
    if (currentEvent) {
        eventID = currentEvent->GetEventID();
    } else {
        // No current event, can't proceed
        return;
    }
    
    // If event ID changes, output results of previous event and reset counters
    if (eventID != currentEventID) {
        if (currentEventID >= 0) {
            // Output photon statistics for the previous event
            OutputPhotonStatistics(currentEventID, cerenkovCount, scintillationCount);
            
            // Remove slab statistics logging
            // LogUtils::log_slab_statistics(currentEventID);  // REMOVE THIS LINE
            
            // Only keep detection statistics (will be logged by EventAction)
            
            // Log simplified event summary without detailed generation stats
            std::map<std::string, int> incidentParticles;
            for (const auto& [particle, count] : particleCount) {
                if (particle != "opticalphoton") {
                    incidentParticles[particle] = count;
                }
            }
            
            std::map<std::string, int> emptyMap; // No need for generated particles
            LogUtils::log_event_summary(currentEventID, incidentParticles, 
                                      cerenkovCount, scintillationCount);
        }
        
        // Reset counters
        cerenkovCount = 0;
        scintillationCount = 0;
        currentEventID = eventID;
        particleCount.clear();
        
        // Reset statistics for the new event
        LogUtils::reset_detection_statistics();
        LogUtils::reset_unique_statistics();  // Add this line
    }
    
    // Get the volume information
    G4StepPoint* preStepPoint = step->GetPreStepPoint();
    G4StepPoint* postStepPoint = step->GetPostStepPoint();
    
    if (!preStepPoint || !postStepPoint) return;
    
    G4VPhysicalVolume* preVolume = preStepPoint->GetPhysicalVolume();
    
    if (!preVolume) return;
    
    // Check if this is a step inside a slab
    // Extract the touch history to get the volume hierarchy
    const G4TouchableHistory* touchable = 
        static_cast<const G4TouchableHistory*>(preStepPoint->GetTouchable());
    
    if (!touchable) return;
    
    // Get the copy number which should correspond to the slab ID
    G4int slabID = -1;
    
    // Check if the volume name contains "slab" (case insensitive)
    G4String volumeName = preVolume->GetName();
    std::string volumeNameLower = volumeName;
    std::transform(volumeNameLower.begin(), volumeNameLower.end(), volumeNameLower.begin(), ::tolower);
    
    if (volumeNameLower.find("slab") != std::string::npos) {
        slabID = touchable->GetCopyNumber(0);
    }
    
    // Track primary particle
    G4Track* primaryTrack = step->GetTrack();
    if (primaryTrack) {
        G4ParticleDefinition* particleDef = primaryTrack->GetDefinition();
        if (particleDef) {
            G4String particleName = particleDef->GetParticleName();
            
            // Only count particles we haven't seen in this event
            if (primaryTrack->GetParentID() == 0) { // Primary particle
                if (particleCount.find(particleName) == particleCount.end()) {
                    particleCount[particleName] = 1;
                } else {
                    particleCount[particleName]++;
                }
                
                // If this is a slab, record it as an incident particle
                if (slabID >= 0 && step->IsFirstStepInVolume()) {
                    LogUtils::add_incident_particle(slabID, particleName.data());
                }
            }
        }
    }
    
    // Get secondary particles in this step
    const std::vector<const G4Track*>* secondaries = step->GetSecondaryInCurrentStep();
    
    if (secondaries && !secondaries->empty()) {
        // Check each secondary particle
        for (const auto* secondaryTrack : *secondaries) {
            if (!secondaryTrack) continue; // Skip null pointers
            
            G4ParticleDefinition* particleDef = secondaryTrack->GetDefinition();
            if (!particleDef) continue; // Skip tracks with no definition
            
            G4String particleName = particleDef->GetParticleName();
            
            // Track ALL secondary particle types, not just a few specific ones
            if (particleDef != G4OpticalPhoton::OpticalPhotonDefinition()) {
                // For non-optical particles, count them for statistics
                if (secondaryTrack->GetParentID() > 0) { // Secondary particle
                    std::string trackName = particleName.data();
                    if (particleCount.find(trackName) == particleCount.end()) {
                        particleCount[trackName] = 1;
                    } else {
                        particleCount[trackName]++;
                    }
                    
                    // If this is a slab, record it as a generated particle
                    if (slabID >= 0) {
                        LogUtils::add_generated_particle(slabID, particleName.data());
                    }
                }
            } else {
                // Handle optical photons
                const G4VProcess* process = secondaryTrack->GetCreatorProcess();
                if (process) {
                    G4String processName = process->GetProcessName();
                    
                    // Update counter based on creation process
                    if (processName == "Cerenkov") {
                        cerenkovCount++;
                        
                        // Get creation position and find which slab (if any) it's in
                        G4ThreeVector position = secondaryTrack->GetPosition();
                        
                        // Find the slab this photon was created in (if any)
                        // This approach uses the current step's slabID as a fallback
                        G4int photonSlabID = slabID;
                        
                        // Always record the photon in global statistics
                        if (photonSlabID >= 0) {
                            LogUtils::add_optical_photon(photonSlabID, true);
                        }
                    }
                    else if (processName == "Scintillation") {
                        scintillationCount++;
                        
                        // Use the same slabID logic as above
                        G4int photonSlabID = slabID;
                        
                        // Always record the photon in global statistics
                        if (photonSlabID >= 0) {
                            LogUtils::add_optical_photon(photonSlabID, false);
                        }
                    }
                }
            }
        }
    }
    
    // Track interesting primary particles too
    G4Track* track = step->GetTrack();
    G4ParticleDefinition* particleDef = track->GetDefinition();
    if (particleDef) {
        G4String particleName = particleDef->GetParticleName();
        G4int trackID = track->GetTrackID();
        
        // Special logging for interesting particles like neutrons, protons, and other hadrons
        static const std::set<std::string> interestingParticles = {
            "neutron", "proton", "pi+", "pi-", "pi0", "kaon+", "kaon-", "kaon0", 
            "alpha", "deuteron", "triton", "He3", "sigma+", "sigma-", "xi-", "lambda"
        };
        
        std::string particleNameStr = particleName.data();
        if (interestingParticles.find(particleNameStr) != interestingParticles.end()) {
            // Log interesting hadronic physics at debug level
            G4double energy = track->GetKineticEnergy() / CLHEP::MeV;
            
            // Only log when they're first created
            if (track->GetCurrentStepNumber() == 1) {
                std::stringstream ss;
                ss << "Interesting particle created: " << particleNameStr 
                   << " (TrackID: " << trackID 
                   << ", ParentID: " << track->GetParentID()
                   << ", Energy: " << energy << " MeV)";
                LogUtils::log_debug(ss.str());
            }
        }
    }
}