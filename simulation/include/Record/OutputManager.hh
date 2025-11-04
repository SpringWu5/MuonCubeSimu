/*
 * OutputManager.hh
 * 
 * Created on: 2024.09.29
 * Author: Cen Mo
 */

#ifndef OUTPUTMANAGER_HH
#define OUTPUTMANAGER_HH

#include "TFile.h"
#include "TTree.h"
#include "G4String.hh"
#include <map>
#include <string>

#include "Record/Muons.hh"
#include "Record/Hits.hh"
#include "Util/Singleton.hh"

struct BranchInfo {
    std::string type;
    std::string description;
};

class OutputManager : public Singleton<OutputManager>
{
public:
    void Book(G4String);
    void Fill() { fOutputTree->Fill(); }
    void Save();

    void EndOfEvent() {
        Fill();
        fMuons->Reset();
        fSLabHits->Reset();
        fHits->Reset();
        ClearSecondaryParticlePDG();  // Clear secondary particles for next event
    }

    void Clear() {
        delete fOutputFile;
        delete fOutputTree;
        delete fMuons;
        delete fSLabHits;
        delete fHits;
    }

    void SetEventID(Int_t eID) { fEventID = eID; }
    void SetTotalEnergyDeposition(Float_t energy) { fTotalEnergyDeposition = energy; }
    void SetSecondaryParticlePDG(std::vector<Int_t> pdg_codes) { fSecondaryParticlePDG = pdg_codes; }
    std::vector<Int_t> GetSecondaryParticlePDG() { return fSecondaryParticlePDG; }
    
    // Add method to add a single secondary particle PDG code
    void AddSecondaryParticlePDG(Int_t pdg_code) { 
        fSecondaryParticlePDG.push_back(pdg_code); 
    }
    
    // Add method to clear secondary particle PDG codes for a new event
    void ClearSecondaryParticlePDG() {
        fSecondaryParticlePDG.clear();
    }

    Int_t GetEventID() { return fEventID; }
    Float_t GetTotalEnergyDeposition() { return fTotalEnergyDeposition; }
    Muons* GetMuons() { return fMuons; }
    Hits* GetSLabHits() { return fSLabHits; }
    Hits* GetHits() { return fHits; }

private:
    friend class Singleton<OutputManager>;
    OutputManager();
    void LoadDataContract();

    TFile*   fOutputFile;
    TTree*   fOutputTree;

    Int_t    fEventID;
    Float_t  fTotalEnergyDeposition;
    Muons*   fMuons;
    Hits*    fSLabHits;
    Hits*    fHits;
    std::vector<Int_t> fSecondaryParticlePDG;  // Added for secondary particle tracking
    
    // Map to store branch information loaded from data contract
    std::map<std::string, BranchInfo> branch_info_;
};


#endif