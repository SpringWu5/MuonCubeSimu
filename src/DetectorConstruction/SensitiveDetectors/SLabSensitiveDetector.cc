#include "DetectorConstruction/SensitiveDetectors/SLabSensitiveDetector.hh"
#include "Record/Hits.hh"
#include "Record/OutputManager.hh"

#include "G4Material.hh"
#include "G4ParticleDefinition.hh"
#include "G4SDManager.hh"
#include "G4Step.hh"
#include "G4SystemOfUnits.hh"
#include "G4OpticalPhoton.hh"
#include "G4TouchableHistory.hh"
#include "G4VTouchable.hh"
#include "g4root.hh"
#include "G4VProcess.hh"  // Added this header for G4VProcess
#include "G4MuonPlus.hh"
#include "G4MuonMinus.hh"

#include "spdlog/spdlog.h"
#include "Util/Logger.hh"

SLabSensitiveDetector::SLabSensitiveDetector(G4String name)
    : G4VSensitiveDetector(name) {}

SLabSensitiveDetector::~SLabSensitiveDetector() {}

G4bool SLabSensitiveDetector::ProcessHits(G4Step *step, G4TouchableHistory *)
{
    spdlog::debug("Slab HIT");

    auto *track = step->GetTrack();
    auto preStepPoint = step->GetPreStepPoint();
    auto postStepPoint = step->GetPostStepPoint();
    auto touchable =
        static_cast<const G4TouchableHistory *>(preStepPoint->GetTouchable());

    if (preStepPoint->GetPosition() == postStepPoint->GetPosition()) {
        spdlog::debug("preStepPoint is the same as postStepPoint, radius: {:.2f} cm",
        (preStepPoint->GetPosition() - touchable->GetTranslation()).mag()/cm);
        return false;
    } 

    // Hit information
    G4int idSlab = touchable->GetCopyNumber(0);
    G4ThreeVector VecSlabToMuon =
        preStepPoint->GetPosition() - touchable->GetTranslation();
    G4ThreeVector dirSlabToMuon = VecSlabToMuon / VecSlabToMuon.mag();
    
#ifdef G4ANALYSIS_USE_DIRECTION
    const G4ThreeVector &dirMuon = preStepPoint->GetMomentumDirection();
#endif
    G4ThreeVector posSlab = touchable->GetTranslation();
    
    // Get particle information
    G4String particleName = track->GetDefinition()->GetParticleName();
    G4int trackID = track->GetTrackID();

    // Record muon only
    Hits* hits = OutputManager::Instance()->GetSLabHits();
    if ((step->GetTrack()->GetDefinition() == G4MuonMinus::Definition() ||
        step->GetTrack()->GetDefinition() == G4MuonPlus::Definition())
        )
    {
        auto hitType = HitType::Muon;
        Float_t energy = postStepPoint->GetKineticEnergy();
        G4ThreeVector momentum = postStepPoint->GetMomentum();
        G4ThreeVector pos = postStepPoint->GetPosition();
        Float_t hitTime = postStepPoint->GetGlobalTime() / ns;
        hits->AddHit(
            hitType, trackID, idSlab, 
            energy/GeV, momentum.x()/GeV, momentum.y()/GeV, momentum.z()/GeV,
            pos.x()/cm, pos.y()/cm, pos.z()/cm, hitTime
        );
    }
    
    // Disable them for now in case of high memory consumption
    // // Check if this is an optical photon
    // if (track->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition()) {
    //     // For optical photons
    //     bool isCerenkov = false;
    //     const G4VProcess* creatorProcess = track->GetCreatorProcess();
    //     if (creatorProcess) {
    //         G4String processName = creatorProcess->GetProcessName();
    //         isCerenkov = (processName == "Cerenkov");
            
    //         // Count as a hit (this counts every interaction)
    //         LogUtils::add_detected_optical_photon(idSlab, isCerenkov);
            
    //         // Also count as a unique particle (only counts each trackID once)
    //         LogUtils::add_unique_optical_photon(idSlab, isCerenkov, trackID);
    //     }
    // } else {
    //     // For non-optical particles
        
    //     // Count as a hit (this counts every interaction)
    //     LogUtils::add_detected_particle(idSlab, particleName.data());
        
    //     // Also count as a unique particle (only counts each trackID once)
    //     LogUtils::add_unique_particle(idSlab, particleName.data(), trackID);
        
    //     // Keep the original incident/generated tracking for backward compatibility
    //     if (track->GetParentID() == 0) {
    //         LogUtils::add_incident_particle(idSlab, particleName.data());
    //     } else {
    //         LogUtils::add_generated_particle(idSlab, particleName.data());
    //     }
    // }

    return true;
}

void SLabSensitiveDetector::DumpInfo(G4Step *step,
                                    G4TouchableHistory *touchable)
{
    // code for understanding the return value of Geant4 function
    G4cout << "*******************************" << G4endl;
    G4cout << "             Slab HIT           " << G4endl;
    G4cout << "  touchable->GetVolume(0)->GetCopyNo(): "
            << touchable->GetVolume(0)->GetCopyNo() << G4endl;
    G4cout << "  touchable->GetVolume(0)->GetTranslation(): "
            << touchable->GetVolume(0)->GetTranslation().x() / CLHEP::mm << " "
            << touchable->GetVolume(0)->GetTranslation().y() / CLHEP::mm << " "
            << touchable->GetVolume(0)->GetTranslation().z() / CLHEP::mm << G4endl;
    G4cout << "  step->GetTrack()->GetKineticEnergy() (eV) : "
            << step->GetTrack()->GetKineticEnergy() / CLHEP::eV << G4endl;
    G4ThreeVector VecSlabToMuon = step->GetPreStepPoint()->GetPosition() -
                                    touchable->GetVolume(0)->GetTranslation();
    G4cout << "  VecSlabToMuon (mm) : " << VecSlabToMuon.x() / CLHEP::mm << " "
            << VecSlabToMuon.y() / CLHEP::mm << " "
            << VecSlabToMuon.z() / CLHEP::mm << G4endl;
    G4cout << "  step->GetPreStepPoint()->GetMomentumDirection(): "
            << step->GetPreStepPoint()->GetMomentumDirection().x() << " "
            << step->GetPreStepPoint()->GetMomentumDirection().y() << " "
            << step->GetPreStepPoint()->GetMomentumDirection().z() << G4endl;
    G4cout << "  step->GetPreStepPoint()->GetGlobalTime() (ns): "
            << step->GetPreStepPoint()->GetGlobalTime() / CLHEP::ns << G4endl;
    G4cout << "  step->GetTrack()->GetStepLength() (mm): "
            << step->GetTrack()->GetStepLength() / CLHEP::mm << G4endl;
    G4cout << "*******************************" << G4endl << G4endl;
}
