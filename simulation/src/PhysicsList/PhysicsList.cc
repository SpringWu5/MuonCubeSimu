#include "PhysicsList/PhysicsList.hh"
#include "PhysicsList/MilliQEMPhysics.hh"
#include "PhysicsList/MilliQMuonPhysics.hh"

#include <iomanip>
#include "globals.hh"
#include "G4ios.hh"
#include "G4ProcessManager.hh"
#include "G4ProcessVector.hh"
#include "G4ParticleTypes.hh"
#include "G4ParticleTable.hh"

#include "G4Material.hh"
#include "G4MaterialTable.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4NistManager.hh"

#include "G4DecayPhysics.hh"
#include "G4RadioactiveDecayPhysics.hh"
#include "G4EmProcessOptions.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4EmExtraPhysics.hh"
#include "G4IonQMDPhysics.hh"
#include "G4IonElasticPhysics.hh"
#include "G4StoppingPhysics.hh"
#include "G4HadronElasticPhysicsHP.hh"
#include "G4HadronElasticPhysicsLEND.hh"
#include "G4EmCalculator.hh"
#include "G4ParticleDefinition.hh"
#include "G4MuonMinus.hh"
#include "G4MuonPlus.hh"
#include "G4Material.hh"

#include "G4HadronPhysicsShielding.hh"

#include "G4OpticalPhysics.hh"

PhysicsList::PhysicsList(G4int verbose)
{
    G4cout << "<<< Geant4 Physics List simulation engine: Shielding 2.1"<<G4endl;
    G4cout <<G4endl;
    this->defaultCutValue = 0.7*CLHEP::mm; //used 0.01mm in smallStep run
    this->SetVerboseLevel(verbose);

    // EM Physics
    RegisterPhysics( new MilliQEMPhysics("standard EM"));

    // Muon Physics
    RegisterPhysics( new MilliQMuonPhysics("muon"));

    // EM Physics
    this->RegisterPhysics( new G4EmStandardPhysics_option4(verbose));

    // Synchroton Radiation & GN Physics
    this->RegisterPhysics( new G4EmExtraPhysics(verbose) );

    // Decays
    this->RegisterPhysics( new G4DecayPhysics(verbose) );
    this->RegisterPhysics( new G4RadioactiveDecayPhysics(verbose) );

    // Hadron Elastic scattering
    this->RegisterPhysics( new G4HadronElasticPhysicsHP(verbose) );

    // Hadron Physics
    G4HadronPhysicsShielding* hps = new G4HadronPhysicsShielding(verbose);

    this->RegisterPhysics( hps );

    //Activate prodcuton of fission fragments in neutronHP
    char env_ff[]="G4NEUTRONHP_PRODUCE_FISSION_FRAGMENTS=1";
    putenv(env_ff);

    // Stopping Physics
    this->RegisterPhysics( new G4StoppingPhysics(verbose) );

    // Ion Physics
    this->RegisterPhysics( new G4IonQMDPhysics(verbose));

    this->RegisterPhysics( new G4IonElasticPhysics(verbose));

    // Neutron tracking cut --> not by default
    // this->RegisterPhysics( new G4NeutronTrackingCut(verbose));

    // G.H.
    //optical physics added manually to shielding list, comment/uncomment to turn on/off photon tracks
    G4OpticalPhysics* opticalPhysics = new G4OpticalPhysics();

    opticalPhysics->SetScintillationYieldFactor(1.0);

    opticalPhysics->SetTrackSecondariesFirst(kCerenkov,false);
    opticalPhysics->SetTrackSecondariesFirst(kScintillation,false);


    this->RegisterPhysics( opticalPhysics );

    //calculator for various physics values
    G4EmCalculator emCalc;
    G4ParticleDefinition* mum = G4MuonMinus::Definition();
    G4ParticleDefinition* mup = G4MuonPlus::Definition();

    G4NistManager* nistMan = G4NistManager::Instance();

    G4Element* elH = nistMan->FindOrBuildElement("H");
    G4Element* elC = nistMan->FindOrBuildElement("C");

    G4Material* concreteMat = nistMan->FindOrBuildMaterial("G4_CONCRETE");
    G4Material* matPlScin = new G4Material("plScintillator", 1.032 * g / cm3, 2);
    matPlScin->AddElement(elC, 10);
    matPlScin->AddElement(elH, 11);
    matPlScin->GetIonisation()->SetBirksConstant(0.126*mm/MeV); // according to L. Reichhart et al., Phys. Rev, use (0.149*mm/MeV) for neutrons, but otherwise use 0.126

    G4double muEnergy = 1*GeV;
    G4double rockDEDX = emCalc.ComputeElectronicDEDX(muEnergy, mum, concreteMat);
    G4double scintDEDX = emCalc.ComputeElectronicDEDX(muEnergy, mum, matPlScin);

    G4cout << "dEdX in rock for mu-: " << rockDEDX << G4endl;
    G4cout << "dEdX in scintillator for mu-: " << scintDEDX << G4endl;

    emCalc.PrintDEDXTable(mum);
    emCalc.PrintRangeTable(mum);
}

PhysicsList::~PhysicsList()
{
}

void PhysicsList::SetCuts()
{
    if (this->verboseLevel >1){
        G4cout << "Shielding::SetCuts:";
    }
    //  " G4VUserPhysicsList::SetCutsWithDefault" method sets
    //   the default cut value for all particle types
    this->SetCutsWithDefault();

    //Set proton cut value to 0 for producing low energy recoil nucleus
    this->SetCutValue(0, "proton");
}




