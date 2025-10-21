/*
 * Muon.hh
 * 
 * Created on: 2024.09.29
 * Author: Cen Mo
 */


#ifndef MUONS_HH
#define MUONS_HH

#include "TTree.h"
#include "vector"

using std::vector;

class Muons
{
public:
    Muons() {};
    ~Muons() {};

    void BookBranches(TTree* tree) {
        tree->Branch("muon_energy", &fEnergy);
        tree->Branch("muon_px", &fPx);
        tree->Branch("muon_py", &fPy);
        tree->Branch("muon_pz", &fPz);
        tree->Branch("weight_spectrum", &fWeightSpectrum, "weight_spectrum/F");
    }

    void Reset() {
        fEnergy.clear();
        fPx.clear();
        fPy.clear();
        fPz.clear();
        fWeightSpectrum = 1.0;
    }

    void AddMuon(Float_t px, Float_t py, Float_t pz) {
        fPx.push_back(px);
        fPy.push_back(py);
        fPz.push_back(pz);
        Float_t energy = sqrt(px*px + py*py + pz*pz);
        fEnergy.push_back(energy);
    }

    void SetWeightSpectrum(Float_t weight) {
        fWeightSpectrum = weight;
    }

private:
    vector<Float_t> fEnergy;
    vector<Float_t> fPx;
    vector<Float_t> fPy;
    vector<Float_t> fPz;
    Float_t fWeightSpectrum;

};


#endif