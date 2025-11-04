#include <G4SteppingVerbose.hh>
#include "Record/OutputManager.hh"
#include "yaml-cpp/yaml.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

OutputManager::OutputManager(): fOutputFile(0), fOutputTree(0), fTotalEnergyDeposition(0.0){ 
    fMuons = new Muons();
    fSLabHits = new Hits();
    fHits = new Hits();
    
    // Load the data contract to determine branch structure
    LoadDataContract();
}

void OutputManager::LoadDataContract() {
    try {
        // Attempt to load the data contract file
        std::ifstream file("../SLabSimu/config/data_contract.json");
        if (!file.is_open()) {
            G4cout << "OutputManager: Warning - Could not open data contract file" << G4endl;
            return;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        auto data_contract = nlohmann::json::parse(content);
        
        // Parse the data contract to initialize our internal structures
        for (auto& [key, value] : data_contract.items()) {
            std::string type = value["type"];
            std::string description = value["description"];
            
            // For now, we'll store this information for reference
            // In the full implementation, we would create branches based on this
            branch_info_[key] = {type, description};
        }
        
        G4cout << "OutputManager: Successfully loaded data contract with " 
               << data_contract.size() << " fields" << G4endl;
    }
    catch (const std::exception& e) {
        G4cout << "OutputManager: Error loading data contract: " << e.what() << G4endl;
    }
}

void OutputManager::Book(G4String outfile)
{
    // Creating a tree container to handle histograms and ntuples.
    // This tree is associated to an output file.
    //
    fOutputFile = new TFile(outfile,"RECREATE");
    if(!fOutputFile) {
        G4cout << " OutputManager::book :"
               << " problem creating the ROOT TFile "
               << G4endl;
        return;
    }

    //info about event statistics
    fOutputTree = new TTree("events", "Simulation Events Tree"); // Changed from "Simu" to "events"
    
    // Create branches dynamically based on the data contract
    for (const auto& [branch_name, branch_info] : branch_info_) {
        if (branch_name == "event_id") {
            fOutputTree->Branch("eventID", &fEventID, "eventID/I");
        } else if (branch_name == "total_energy_deposition") {
            fOutputTree->Branch("totalEnergyDeposition", &fTotalEnergyDeposition, "totalEnergyDeposition/F");
        } else if (branch_name == "secondary_particle_pdg") {
            // Special handling for secondary particles - this will be a vector branch
            fOutputTree->Branch("secondaryParticlePDG", &fSecondaryParticlePDG);
        } else {
            // For other branches, we'll continue to use existing hardcoded branches
            // since the OutputManager doesn't have all the necessary member variables
            G4cout << "OutputManager: Found branch '" << branch_name 
                   << "' with type '" << branch_info.type << "' (using existing implementation)" << G4endl;
        }
    }
    
    // Keep the existing branch structure for compatibility with existing code
    fMuons->BookBranches(fOutputTree);
    fSLabHits->BookBranches(fOutputTree, "SLab");
    fHits->BookBranches(fOutputTree, "SiPM");
    
    G4cout << "OutputManager: Booked " << fOutputTree->GetNbranches() << " branches" << G4endl;
}


void OutputManager::Save()
{
    if (fOutputFile) {
        fOutputFile->Write();       // Writing the histograms to the file
        fOutputFile->Close();        // and closing the tree (and the file)
    }
}