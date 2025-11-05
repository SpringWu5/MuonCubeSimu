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
#include "Util/ConfigManager.hh"

// Initialize static counter
G4int PrimaryGeneratorAction::fNextMuonIndex = 0;

PrimaryGeneratorAction::PrimaryGeneratorAction()
    : G4VUserPrimaryGeneratorAction() {
    spdlog::info("PrimaryGeneratorAction: Initialize primary generator action for high-energy muon beam");
    
    // Use ConfigManager to get configuration
    // No need to load config directly as it should be loaded in main
    auto configNode = ConfigManager::Instance()->GetNode("particle_gun");
    if (!configNode) {
        spdlog::error("Could not load particle_gun config from ConfigManager");
        fUseParticleGun = false;
        return; // Early return if config is not available
    }
    
    // Check if particle gun is enabled in config
    if (configNode["enabled"]) {
        fUseParticleGun = configNode["enabled"].as<bool>();
    } else {
        fUseParticleGun = false;
    }
    
    // Set up particle gun (will be configured differently based on fUseParticleGun)
    fParticleGun = new G4ParticleGun(1);
    
    if (fUseParticleGun) {
        // Get particle type from config
        fParticleType = configNode["particle_type"] ? 
            configNode["particle_type"].as<std::string>() : "mu-";
        
        // Get energy distribution configuration from config
        auto energy_dist = configNode["energy_distribution"];
        fParticleEnergy = 100.0 * MeV; // default energy
        
        if (energy_dist && energy_dist["type"]) {
            std::string energy_type = energy_dist["type"].as<std::string>();
            
            if (energy_type == "fixed" && energy_dist["params"]["mean"]) {
                fParticleEnergy = energy_dist["params"]["mean"].as<double>() * MeV;
            } else if (energy_type == "uniform" && energy_dist["params"]["min"] && energy_dist["params"]["max"]) {
                // For uniform distribution we'll calculate the average
                double min_energy = energy_dist["params"]["min"].as<double>();
                double max_energy = energy_dist["params"]["max"].as<double>();
                fParticleEnergy = ((min_energy + max_energy) / 2.0) * MeV;
            }
            // Add other energy distribution types as needed
        }
        
        // Get direction distribution configuration from config
        auto dir_dist = configNode["direction_distribution"];
        fParticleDirection = G4ThreeVector(0, 0, 1); // default direction
        
        if (dir_dist && dir_dist["type"]) {
            std::string dir_type = dir_dist["type"].as<std::string>();
            
            if (dir_type == "fixed" && dir_dist["params"]["dir_x"] && 
                dir_dist["params"]["dir_y"] && dir_dist["params"]["dir_z"]) {
                fParticleDirection = G4ThreeVector(
                    dir_dist["params"]["dir_x"].as<double>(),
                    dir_dist["params"]["dir_y"].as<double>(),
                    dir_dist["params"]["dir_z"].as<double>()
                );
            }
            // Add other direction distribution types as needed
        }
        
        // Get position distribution configuration from config
        auto pos_dist = configNode["position_distribution"];
        fParticlePosition = G4ThreeVector(0, 0, 0); // default position
        
        if (pos_dist && pos_dist["type"]) {
            std::string pos_type = pos_dist["type"].as<std::string>();
            
            if (pos_type == "fixed" && pos_dist["params"]["pos_x"] && 
                pos_dist["params"]["pos_y"] && pos_dist["params"]["pos_z"]) {
                fParticlePosition = G4ThreeVector(
                    pos_dist["params"]["pos_x"].as<double>() * mm,
                    pos_dist["params"]["pos_y"].as<double>() * mm,
                    pos_dist["params"]["pos_z"].as<double>() * mm
                );
            } else if (pos_type == "uniform" && pos_dist["params"]["x_min"] && 
                pos_dist["params"]["x_max"] && pos_dist["params"]["y_min"] && 
                pos_dist["params"]["y_max"] && pos_dist["params"]["pos_z"]) {
                // For uniform distribution, we'll take a random position
                double x_min = pos_dist["params"]["x_min"].as<double>();
                double x_max = pos_dist["params"]["x_max"].as<double>();
                double y_min = pos_dist["params"]["y_min"].as<double>();
                double y_max = pos_dist["params"]["y_max"].as<double>();
                double z_pos = pos_dist["params"]["pos_z"].as<double>();
                
                double x = x_min + G4UniformRand() * (x_max - x_min);
                double y = y_min + G4UniformRand() * (y_max - y_min);
                
                fParticlePosition = G4ThreeVector(x * mm, y * mm, z_pos * mm);
            }
            // Add other position distribution types as needed
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
