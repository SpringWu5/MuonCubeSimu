#include "G4ParticleDefinition.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"
#include "spdlog/spdlog.h"
#include "yaml-cpp/yaml.h"
#include <vector>

#include "Record/OutputManager.hh"
#include "Record/Muons.hh"
#include "Action/PrimaryGeneratorAction.hh"

// Initialize static counter
G4int PrimaryGeneratorAction::fNextMuonIndex = 0;

PrimaryGeneratorAction::PrimaryGeneratorAction()
    : G4VUserPrimaryGeneratorAction() {
    spdlog::info("PrimaryGeneratorAction: Initialize primary generator action for high-energy muon beam");
    
    // Configuration should be passed from the main function
    // For now, we'll load it directly like other modules do
    try {
        fConfig = YAML::LoadFile("simulation/config/config.yaml");
    } catch (const YAML::Exception& e) {
        spdlog::error("Could not load config file: {}", e.what());
        fUseParticleGun = false;
        return; // Early return if config file cannot be loaded
    }
    
    // Check if particle gun is enabled in config
    if (fConfig["particle_gun"] && fConfig["particle_gun"]["enabled"]) {
        fUseParticleGun = fConfig["particle_gun"]["enabled"].as<bool>();
    } else {
        fUseParticleGun = false;
    }
    
    // Set up particle gun (will be configured differently based on fUseParticleGun)
    fParticleGun = new G4ParticleGun(1);
    
    if (fUseParticleGun) {
        // Get particle type from config
        fParticleType = fConfig["particle_gun"]["particle"] ? 
            fConfig["particle_gun"]["particle"].as<std::string>() : "mu-";
        
        // Get particle energy from config (in MeV)
        fParticleEnergy = fConfig["particle_gun"]["energy_MeV"] ? 
            fConfig["particle_gun"]["energy_MeV"].as<double>() * MeV : 100.0 * MeV;
        
        // Get particle position from config
        if (fConfig["particle_gun"]["position_mm"]) {
            auto pos = fConfig["particle_gun"]["position_mm"].as<std::vector<double>>();
            fParticlePosition = G4ThreeVector(pos[0] * mm, pos[1] * mm, pos[2] * mm);
        } else {
            fParticlePosition = G4ThreeVector(0, 0, 0);
        }
        
        // Get particle direction from config
        if (fConfig["particle_gun"]["direction"]) {
            auto dir = fConfig["particle_gun"]["direction"].as<std::vector<double>>();
            fParticleDirection = G4ThreeVector(dir[0], dir[1], dir[2]);
        } else {
            fParticleDirection = G4ThreeVector(0, 0, 1);
        }
        
        // Set particle definition
        auto particleTable = G4ParticleTable::GetParticleTable();
        G4ParticleDefinition* particleDef = particleTable->FindParticle(fParticleType);
        if (!particleDef) {
            spdlog::error("Could not find particle: {}", fParticleType);
            particleDef = particleTable->FindParticle("mu-"); // fallback to muon
        }
        fParticleGun->SetParticleDefinition(particleDef);
        
        spdlog::info("Particle gun configured: {} with energy {:.2f} MeV at position ({:.1f}, {:.1f}, {:.1f}) mm, direction ({:.1f}, {:.1f}, {:.1f})", 
                    fParticleType, fParticleEnergy/MeV,
                    fParticlePosition.x()/mm, fParticlePosition.y()/mm, fParticlePosition.z()/mm,
                    fParticleDirection.x(), fParticleDirection.y(), fParticleDirection.z());
    } else {
        // Set particle type to negative muon for default cosmic ray generator logic
        auto particleTable = G4ParticleTable::GetParticleTable();
        G4ParticleDefinition* particleDef = particleTable->FindParticle("mu-");
        fParticleGun->SetParticleDefinition(particleDef);
        
        spdlog::info("Using default cosmic ray generator logic");
    }
}

PrimaryGeneratorAction::~PrimaryGeneratorAction() {
    delete fParticleGun;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event *event) {
    // To record muon info
    Muons* muons = OutputManager::Instance()->GetMuons();

    if (fUseParticleGun) {
        // Use configurable particle gun parameters
        fParticleGun->SetParticlePosition(fParticlePosition);
        fParticleGun->SetParticleMomentumDirection(fParticleDirection);
        fParticleGun->SetParticleEnergy(fParticleEnergy);
        
        // Record muon momentum for validation
        // Calculate momentum components based on energy and direction
        G4double momentum = fParticleEnergy;
        G4double px = momentum * fParticleDirection.x();
        G4double py = momentum * fParticleDirection.y();
        G4double pz = momentum * fParticleDirection.z();
        
        muons->AddMuon(px/GeV, py/GeV, pz/GeV);

        // Generate the particle
        fParticleGun->GeneratePrimaryVertex(event);

        spdlog::info("Generated configurable particle for event {} with energy {:.2f} MeV", 
                    event->GetEventID(), fParticleEnergy/MeV);
    } else {
        // Use original cosmic ray generator logic
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
}
