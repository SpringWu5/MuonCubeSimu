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
    : G4VUserPrimaryGeneratorAction(), 
    fParticleList(particle_list) {
    spdlog::info("PrimaryGeneratorAction: Initialize primary generator action");
    fParticleGun = new G4ParticleGun(1);
}

PrimaryGeneratorAction::~PrimaryGeneratorAction() {
    delete fParticleGun;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event *event) {
    // Get the next muon index, taking into account looping
    int idx = fNextMuonIndex % fParticleList.size();
    fNextMuonIndex++; // Increment for next time
    
    // To record muon info
    Muons* muons = OutputManager::Instance()->GetMuons();

    auto particleTable = G4ParticleTable::GetParticleTable();

    auto mc_event = fParticleList[idx].get<McEvent>();

    // record event weight
    muons->SetWeightSpectrum(mc_event.weight_spectrum);

    int numParticles = mc_event.particles_at_detector.size();
    G4PrimaryVertex **vecPrimaryVertex = new G4PrimaryVertex *[numParticles];

    for (int i = 0; i < numParticles; i++) {
        vecPrimaryVertex[i] = new G4PrimaryVertex;
        const auto &mcParticle = mc_event.particles_at_detector[i];

        // Use actual particle position from JSON
        vecPrimaryVertex[i]->SetPosition(mcParticle.x * cm, mcParticle.y * cm,
                                        mcParticle.z * cm);
        vecPrimaryVertex[i]->SetT0(mcParticle.t * ns);
        
        auto particleDef = particleTable->FindParticle(mcParticle.pdgid);
        if (particleDef == nullptr) {
            spdlog::warn("Can not find particle with pdgid {:d}", mcParticle.pdgid);
        }
        G4PrimaryParticle *particle =
            new G4PrimaryParticle(particleDef, mcParticle.px * GeV,
                                    mcParticle.py * GeV, mcParticle.pz * GeV);
        vecPrimaryVertex[i]->SetPrimary(particle);

        // Record muons
        muons->AddMuon(mcParticle.px, mcParticle.py, mcParticle.pz);
        event->AddPrimaryVertex(vecPrimaryVertex[i]);
    }
    delete[] vecPrimaryVertex;

    spdlog::info("Generated primary particles for event {}, using muon data index {}", 
                event->GetEventID(), idx);
}
