#include "G4ParticleDefinition.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"
#include "spdlog/spdlog.h"

#include "Record/OutputManager.hh"
#include "Record/Muons.hh"
#include "Action/PrimaryGeneratorAction.hh"

// Initialize static counter
G4int PrimaryGeneratorAction::fNextMuonIndex = 0;

PrimaryGeneratorAction::PrimaryGeneratorAction(json particle_list)
    : G4VUserPrimaryGeneratorAction() {
    spdlog::info("PrimaryGeneratorAction: Initialize primary generator action for high-energy muon beam");
    fParticleGun = new G4ParticleGun(1);
    
    // Set particle type to negative muon
    auto particleTable = G4ParticleTable::GetParticleTable();
    G4ParticleDefinition* particleDef = particleTable->FindParticle("mu-");
    fParticleGun->SetParticleDefinition(particleDef);
}

PrimaryGeneratorAction::~PrimaryGeneratorAction() {
    delete fParticleGun;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event *event) {
    // To record muon info
    Muons* muons = OutputManager::Instance()->GetMuons();

    // Set the particle's starting position to (0, 0, 100 cm)
    fParticleGun->SetParticlePosition(G4ThreeVector(0, 0, 100 * cm));

    // Set the particle's momentum direction to (0, 0, -1) - vertically downwards
    fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, -1));

    // Generate a random kinetic energy between 10 GeV and 40 GeV
    G4double minEnergy = 10 * GeV;
    G4double maxEnergy = 40 * GeV;
    G4double randomEnergy = minEnergy + (maxEnergy - minEnergy) * G4UniformRand();

    fParticleGun->SetParticleEnergy(randomEnergy);

    // Record muon momentum for validation
    muons->AddMuon(0, 0, -randomEnergy/GeV); // Only z-component, negative as it's going downward

    // Generate the particle
    fParticleGun->GeneratePrimaryVertex(event);

    spdlog::info("Generated primary muon for event {} with energy {:.2f} GeV", 
                event->GetEventID(), randomEnergy/GeV);
}
