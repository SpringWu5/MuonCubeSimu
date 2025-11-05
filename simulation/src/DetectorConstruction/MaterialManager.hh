#ifndef MATERIAL_MANAGER_HH
#define MATERIAL_MANAGER_HH

#include "yaml-cpp/yaml.h"
#include "G4String.hh"
#include "spdlog/spdlog.h"
#include <vector>
#include <map>
#include <string>

#include "G4Material.hh"
#include "G4Element.hh"
#include "G4MaterialPropertiesTable.hh"

// Structure to hold the SLab geometry parameters
struct SLabGeometry {
    double Scintxlength, Scintylength, Scintzlength;  // Scintillator dimensions
    double ESRthickness;  // ESR (Enhanced Specular Reflector) thickness
    double Tapethickness; // Tape thickness
    double SiPMxlength, SiPMylength, SiPMzlength;     // SiPM dimensions
    double Batteryxlength, Batteryylength, Batteryzlength;  // Battery dimensions
    int numberOfSlabs;  // Number of slabs
    std::vector<double> slabOffsets;  // Offsets for slab positions
};

// Structure to hold sea optical properties
struct SeaOpticalProperty {
    std::vector<double> energy;
    std::vector<double> refracIdxPhase;
    std::vector<double> refracIdxGroup;
    std::vector<double> absLen;
    std::vector<double> scaLenRay;
    std::vector<double> scaLenMie;
    double mieForward;
    int num;
};

class MaterialManager {
public:
    static MaterialManager* Instance();

    // Build all the materials and optical properties
    bool BuildEverything(const G4String &fileYAML);

    // Get material by name
    G4Material* GetMaterial(std::string name);

    // Get SLab geometry
    const SLabGeometry& GetSLabGeometry() const { return fSLabGeometry; }

    // Get sea optical properties
    const SeaOpticalProperty& GetSeaOpticalProperty() const { return fSeaOpticalProperty; }

    // Get material properties
    G4MaterialPropertiesTable* GetMediumOpticalProperties() const { return fMediumOpticalProperties; }

    // Get array of properties
    float* GetArrayProperites();

private:
    MaterialManager() = default;
    ~MaterialManager() = default;
    
    // Delete copy constructor and assignment operator
    MaterialManager(const MaterialManager&) = delete;
    MaterialManager& operator=(const MaterialManager&) = delete;

    // Load parameters from YAML file
    void LoadYAML();

    // Build elements
    void BuildElement();

    // Build optical properties
    void BuildOpticalProperties();

    // Build materials
    void BuildMaterial();

    // Individual material building methods
    void BuildSeaWater();
    void BuildIce();
    void BuildGlass();
    void BuildEpoxy();
    void BuildGel();
    void BuildScint();
    void BuildESR();
    void BuildTape();
    void BuildSiPM();
    void BuildBattery();

    // Set optical properties for polystyrene scintillator
    G4MaterialPropertiesTable* SetOpticalPropertiesOfPS();

    // Root node for the YAML file
    YAML::Node rootNode;

    // Configuration path to resolve relative paths
    G4String fConfigPath;

    // Elements
    G4Element* fElH;
    G4Element* fElC;
    G4Element* fElO;
    G4Element* fElSi;
    G4Element* fElNa;
    G4Element* fElCl;

    // Materials map
    std::map<std::string, G4Material*> fMapMaterial;

    // SLab geometry
    SLabGeometry fSLabGeometry;

    // Sea optical properties
    SeaOpticalProperty fSeaOpticalProperty;

    // Medium optical properties
    G4MaterialPropertiesTable* fMediumOpticalProperties;

    // Logger
    std::shared_ptr<spdlog::logger> logger;
};

#endif // MATERIAL_MANAGER_HH