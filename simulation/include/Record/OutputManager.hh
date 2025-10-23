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

#include "Record/Muons.hh"
#include "Record/Hits.hh"
#include "Util/Singleton.hh"

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

    Int_t GetEventID() { return fEventID; }
    Float_t GetTotalEnergyDeposition() { return fTotalEnergyDeposition; }
    Muons* GetMuons() { return fMuons; }
    Hits* GetSLabHits() { return fSLabHits; }
    Hits* GetHits() { return fHits; }

private:
    friend class Singleton<OutputManager>;
    OutputManager();

    TFile*   fOutputFile;
    TTree*   fOutputTree;

    Int_t    fEventID;
    Float_t  fTotalEnergyDeposition;
    Muons*   fMuons;
    Hits*    fSLabHits;
    Hits*    fHits;
};


#endif