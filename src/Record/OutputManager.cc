#include <G4SteppingVerbose.hh>
#include "Record/OutputManager.hh"


OutputManager::OutputManager(): fOutputFile(0), fOutputTree(0){ 
    fMuons = new Muons();
    fSLabHits = new Hits();
    fHits = new Hits();
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
    fOutputTree = new TTree("Simu", "Simulation Tree");
    fOutputTree->Branch("eventID", &fEventID, "eventID/I");
    fMuons->BookBranches(fOutputTree);
    fSLabHits->BookBranches(fOutputTree, "SLab");
    fHits->BookBranches(fOutputTree, "SiPM");
}


void OutputManager::Save()
{
    if (fOutputFile) {
        fOutputFile->Write();       // Writing the histograms to the file
        fOutputFile->Close();        // and closing the tree (and the file)
    }
}
