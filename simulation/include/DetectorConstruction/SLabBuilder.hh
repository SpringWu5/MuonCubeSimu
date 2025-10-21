/*
 * SLabBuilder.hh
 * 
 * Created on: 2024.09.28
 * Author: Weilun Huang
 */

#ifndef SLABBUILDER_HH
#define SLABBUILDER_HH

#include "G4VUserDetectorConstruction.hh"

#include "Util/Logger.hh"
#include "G4LogicalVolume.hh"
#include "G4VSolid.hh"
#include "G4Transform3D.hh"
#include "vector"

using std::vector;
class SLabBuilder  : public G4VUserDetectorConstruction
{
public:
    SLabBuilder(const char* config_path);
    virtual G4VPhysicalVolume* Construct();
    void BuildSolid();
    void BuildSurface();
    void BuildSD();

private:
    vector<G4Transform3D> GetTransformsForSiPMs();
    std::shared_ptr<spdlog::logger> logger;
    const char* fConfigPath;
    G4LogicalVolume *fLogicScint;
    G4LogicalVolume *fLogicESR;
    G4LogicalVolume *fLogicTape;
    G4LogicalVolume *fLogicBattery;

    G4VSolid *fSolidSlabScint;
    G4VSolid *fSolidSlabESR;
    G4VSolid *fSolidSlabTape;
    G4VSolid *fSolidBattery;

    G4VPhysicalVolume *world;
};

#endif