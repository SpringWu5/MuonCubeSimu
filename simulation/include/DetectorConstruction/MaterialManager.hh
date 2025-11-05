/*
 * MaterialManager.hh
 * 
 * Created on: 2024.09.28
 * Author: Weilun Huang
 */

#ifndef MATERIALMANAGER_HH
#define MATERIALMANAGER_HH

#include "G4Element.hh"
#include "G4Material.hh"
#include "G4SystemOfUnits.hh"
#include "Util/Logger.hh"
#include "Util/Singleton.hh"
#include "yaml-cpp/yaml.h"
#include "map"
#include "string"

struct SLabGeometry
{
    double Scintxlength;
    double Scintylength;
    double Scintzlength;
    double ESRthickness;
    double Tapethickness;
    double SiPMxlength;
    double SiPMylength;
    double SiPMzlength;
    double Batteryxlength;
    double Batteryylength;
    double Batteryzlength;
    std::vector<double> slabOffsets;
    int numberOfSlabs;
};

struct OpticalProperty
{
    int num;
    std::vector<double> energy;
    std::vector<double> refracIdxPhase;
    std::vector<double> refracIdxGroup;
    std::vector<double> absLen;
    std::vector<double> scaLenRay;
    std::vector<double> scaLenMie;
    double mieForward;
};


struct SipmProperty
{
    G4int Num = 5;
    G4double Ephoton[5] = {
        2.06667 * eV,
        2.5 * eV,
        3.0 * eV,
        3.5 * eV,
        4.13333 * eV};
    G4double Reflection[5] = {
        0.1,
        0.1,
        0.1,
        0.1,
        0.1};
    G4double RelativeEfficiency[5] = {
        1.0,
        1.0,
        1.0,
        1.0,
        1.0};
    G4double MaxEfficiency = 0.6;
};


class MaterialManager : public Singleton<MaterialManager> {
public:
    YAML::Node getRootNode() { return rootNode; }
    
    G4String getConfigPath() { return fConfigPath; }

    bool BuildEverything(const G4String &fileYAML);

    G4Material *GetMaterial(std::string name);
    float *GetArrayProperites();
    SLabGeometry GetSLabGeometry() { return fSLabGeometry; }
    SipmProperty GetSipmProperty() { return fSipmProperty; }

private:
    friend class Singleton<MaterialManager>;
    MaterialManager() {};
    void LoadYAML();
    void BuildElement();
    void BuildMaterial();

    void BuildOpticalProperties();

    // medium of sea
    void BuildSeaWater();

    void BuildIce();

    // material for DOM protection glass and PMT glass,
    // not precise, it also contains B2O3、Na2O
    void BuildGlass();

    // material for support in DOM, not precise
    void BuildEpoxy();

    // not precise
    void BuildGel();

    // not precise
    void BuildSiPM();

    void BuildScint();

    void BuildESR();

    void BuildTape();

    void BuildBattery();

    G4MaterialPropertiesTable* SetOpticalPropertiesOfPS();

    std::shared_ptr<spdlog::logger> logger;
    G4Element *fElH, *fElC, *fElO, *fElNa, *fElSi, *fElCl;
    G4MaterialPropertiesTable *fMediumOpticalProperties;
    std::map<std::string, G4Material *> fMapMaterial;

    YAML::Node rootNode;
    G4String fConfigPath;  // Store the config file path to resolve relative paths
    SLabGeometry fSLabGeometry;
    OpticalProperty fSeaOpticalProperty;
    SipmProperty fSipmProperty;
};

#endif