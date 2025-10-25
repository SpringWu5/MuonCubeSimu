#pragma once

#include "TTree.h"
#include "G4ThreeVector.hh"
#include "vector"

class McEventRoot
{
public:
    McEventRoot() {};
    ~McEventRoot() {};

    void BookBranches(TTree* tree) {
        tree->Branch("event_id", &fEventId, "event_id/I");
        tree->Branch("initial_momentum_x", &fInitialMomentumX);
        tree->Branch("initial_momentum_y", &fInitialMomentumY);
        tree->Branch("initial_momentum_z", &fInitialMomentumZ);
    }

    void Reset() {
        fEventId = 0;
        fInitialMomentumX.clear();
        fInitialMomentumY.clear();
        fInitialMomentumZ.clear();
    }

    void SetEventId(Int_t eventId) { fEventId = eventId; }
    
    void AddInitialMomentum(G4ThreeVector momentum) {
        fInitialMomentumX.push_back(momentum.x());
        fInitialMomentumY.push_back(momentum.y());
        fInitialMomentumZ.push_back(momentum.z());
    }

private:
    Int_t fEventId;
    std::vector<Float_t> fInitialMomentumX;
    std::vector<Float_t> fInitialMomentumY;
    std::vector<Float_t> fInitialMomentumZ;
};