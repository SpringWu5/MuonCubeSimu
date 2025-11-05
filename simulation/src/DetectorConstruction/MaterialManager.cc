#include "DetectorConstruction/MaterialManager.hh"
#include "G4Molecule.hh"
#include "G4SystemOfUnits.hh"
#include "G4MaterialTable.hh"
#include "G4NistManager.hh"
#include "Util/ConfigManager.hh"
#include <sstream>
#include <string>
#include <filesystem>

using std::string;
using std::vector;

bool MaterialManager::BuildEverything(const G4String &fileYAML)
{
    logger = create_logger("MaterialManager");
    fConfigPath = fileYAML;  // Store the config path to resolve relative paths
    
    // Configuration should already be loaded via ConfigManager in main
    // The configuration is already validated through ConfigManager
    // We'll access the necessary parameters via ConfigManager
    
    try {
        LoadYAML();
    }
    catch (YAML::BadConversion &e)
    {
        logger->error("[Read YAML] ==> {:s}", e.msg);
        return false;
    }
    catch (YAML::InvalidNode &e)
    {
        logger->error("[Read YAML] ==> {:s}", e.msg);
        return false;
    }

    BuildElement();
    BuildOpticalProperties();
    BuildMaterial();
    return true;
}

void MaterialManager::LoadYAML()
{
    // Load SLab geometry from ConfigManager
    fSLabGeometry.Scintxlength = ConfigManager::Instance()->GetNode("detector.geometry.slab_dimensions.width").as<double>() * mm;
    fSLabGeometry.Scintylength = ConfigManager::Instance()->GetNode("detector.geometry.slab_dimensions.height").as<double>() * mm;
    fSLabGeometry.Scintzlength = ConfigManager::Instance()->GetNode("detector.geometry.slab_dimensions.thickness").as<double>() * mm;
    
    // We need to adapt to the new config format which doesn't have ESR/Tape/Battery dimensions separately
    // Using reasonable defaults based on scintillator dimensions
    fSLabGeometry.ESRthickness = 1.0 * mm; // Default ESR thickness
    fSLabGeometry.Tapethickness = 1.0 * mm; // Default tape thickness
    fSLabGeometry.SiPMxlength = 3.0 * mm; // Default SiPM size
    fSLabGeometry.SiPMylength = 3.0 * mm; 
    fSLabGeometry.SiPMzlength = 0.5 * mm; 
    fSLabGeometry.Batteryxlength = 10.0 * mm; // Default battery size
    fSLabGeometry.Batteryylength = 10.0 * mm; 
    fSLabGeometry.Batteryzlength = 5.0 * mm; 

    fSLabGeometry.numberOfSlabs = ConfigManager::Instance()->GetNode("detector.geometry.slab_positions").size();
    
    // Get slab positions from the new config format
    auto slabPositionsNode = ConfigManager::Instance()->GetNode("detector.geometry.slab_positions");
    for (size_t i = 0; i < slabPositionsNode.size(); ++i) {
        // Get z position of each slab from the new structure
        double z_pos = slabPositionsNode[i]["z"].as<double>();
        fSLabGeometry.slabOffsets.push_back(z_pos * mm);
    }

    // Extract config file directory to resolve relative paths
    std::filesystem::path configPath(fConfigPath.c_str());
    std::filesystem::path configDir = configPath.parent_path();

    // Load sea optical properties from the new config structure
    string pathFile = ConfigManager::Instance()->GetNode("property.sea_optical_property.path_file").as<string>();
    logger->debug("Reading optical properties file: {}", pathFile);
    
    // Resolve path relative to config file directory
    std::filesystem::path fullPath = configDir / pathFile;
    YAML::Node node = YAML::LoadFile(fullPath.string());
    fSeaOpticalProperty.energy = node["energy"].as<vector<double>>();
    fSeaOpticalProperty.num = fSeaOpticalProperty.energy.size();
    fSeaOpticalProperty.refracIdxPhase = node["refractive_index_phase"].as<vector<double>>();
    fSeaOpticalProperty.refracIdxGroup = node["refractive_index_group"].as<vector<double>>();
    fSeaOpticalProperty.absLen = node["absorption_length"].as<vector<double>>();
    fSeaOpticalProperty.scaLenRay = node["scatter_length_rayeigh"].as<vector<double>>();
    fSeaOpticalProperty.scaLenMie = node["scatter_length_mie"].as<vector<double>>();
    fSeaOpticalProperty.mieForward = node["mie_forward_angle"].as<double>();
    // set unit
    for (auto &eng : fSeaOpticalProperty.energy) { eng *= eV; }
    for (auto &len : fSeaOpticalProperty.absLen) { len *=  m; }
    for (auto &len : fSeaOpticalProperty.scaLenRay) { len *= m; }
    for (auto &len : fSeaOpticalProperty.scaLenMie) { len *= m; }
    // Note: slabOffsets are already in mm from the config, so no need to multiply by cm again
}

void MaterialManager::BuildElement()
{
    // define element
    G4double a; // Zeff
    a = 1.01 * g / mole;
    fElH = new G4Element("Hydrogen", "H", 1., a);
    a = 12.01 * g / mole;
    fElC = new G4Element("Carbon", "C", 6., a);
    a = 16.00 * g / mole;
    fElO = new G4Element("Oxygen", "O", 8., a);
    a = 28.00 * g / mole;
    fElSi = new G4Element("Silicon", "Si", 14., a);
    a = 22.99 * g / mole;
    fElNa = new G4Element("Sodium", "Na", 11, a);
    a = 35.453 * g / mole;
    fElCl = new G4Element("Chlorine", "Cl", 17., a);
    logger->info("Finish building element");
}

void MaterialManager::BuildMaterial()
{
    // build vacuum
    G4Material *Vacuum = new G4Material("Vacuum", 1., 1.01 * g / mole, 1.e-25 * g / cm3, kStateGas, 2.73 * kelvin, 3.e-18 * pascal);
    fMapMaterial.insert({"Vacuum", Vacuum});

    BuildSeaWater();
    BuildGlass();
    BuildEpoxy();
    BuildGel();
    BuildSiPM();
    BuildScint();
    BuildESR();
    BuildTape();
    BuildBattery();
    logger->info("Finish building material");
}

G4Material *MaterialManager::GetMaterial(std::string name)
{
    auto iter = fMapMaterial.find(name);
    if (iter != fMapMaterial.end())
    {
        return iter->second;
    }
    else
    {
        logger->error("No material named {:s} has been found!", name);
        throw;
    }
}

void MaterialManager::BuildOpticalProperties()
{
    fMediumOpticalProperties = new G4MaterialPropertiesTable();
    fMediumOpticalProperties->AddProperty("RINDEX", fSeaOpticalProperty.energy.data(), fSeaOpticalProperty.refracIdxPhase.data(), fSeaOpticalProperty.num)->SetSpline(false);
    fMediumOpticalProperties->AddProperty("ABSLENGTH", fSeaOpticalProperty.energy.data(), fSeaOpticalProperty.absLen.data(), fSeaOpticalProperty.num)->SetSpline(false);
    fMediumOpticalProperties->AddProperty("RAYLEIGH", fSeaOpticalProperty.energy.data(), fSeaOpticalProperty.scaLenRay.data(), fSeaOpticalProperty.num)->SetSpline(false);
    if (fSeaOpticalProperty.mieForward)
    {
        fMediumOpticalProperties->AddProperty("MIEHG", fSeaOpticalProperty.energy.data(), fSeaOpticalProperty.scaLenMie.data(), fSeaOpticalProperty.num)->SetSpline(false);
        fMediumOpticalProperties->AddConstProperty("MIEHG_FORWARD", fSeaOpticalProperty.mieForward);
        fMediumOpticalProperties->AddConstProperty("MIEHG_BACKWARD", 0.0);
        fMediumOpticalProperties->AddConstProperty("MIEHG_FORWARD_RATIO", 1.0);
    }
}

void MaterialManager::BuildSeaWater()
{
    // build material component
    G4Material *H2O = new G4Material("Water", 1.00 * g / cm3, 2);
    H2O->AddElement(fElH, 2);
    H2O->AddElement(fElO, 1);
    G4Material *NaCl = new G4Material("Sodium Chlorure", 2.16 * g / cm3, 2);
    NaCl->AddElement(fElNa, 1);
    NaCl->AddElement(fElCl, 1);
    G4Material *SeaWater = new G4Material("Sea Water", 1.04 * g / cm3, 2, kStateLiquid,
                                            300. * atmosphere, 275. * kelvin);
    SeaWater->AddMaterial(NaCl, 3.5 * perCent);
    SeaWater->AddMaterial(H2O, 96.5 * perCent);
    SeaWater->SetMaterialPropertiesTable(fMediumOpticalProperties);
    fMapMaterial.insert({"Sea Water", SeaWater});
}

void MaterialManager::BuildIce()
{
    // build material component
    G4Material *Ice = new G4Material("Ice", 0.92 * g / cm3, 2);
    Ice->AddElement(fElH, 2);
    Ice->AddElement(fElO, 1);
    Ice->SetMaterialPropertiesTable(fMediumOpticalProperties);
    fMapMaterial.insert({"Ice", Ice});
}

void MaterialManager::BuildGlass()
{
    // build material component
    G4Material *Glass = new G4Material("Glass", 1.19 * g / cm3, 2);
    Glass->AddElement(fElSi, 1);
    Glass->AddElement(fElO, 2);

    // build optical property
    G4MaterialPropertiesTable *mpt = new G4MaterialPropertiesTable();
    const G4int num = 2;
    G4double photonEnergy[num] = {2.06667 * eV, 4.13333 * eV};
    G4double refractiveIndex[num] = {1.50, 1.50};
    G4double absorptionLength[num] = {10. * m, 10. * m};
    mpt->AddProperty("RINDEX", photonEnergy, refractiveIndex, num);
    mpt->AddProperty("ABSLENGTH", photonEnergy, absorptionLength, num);
    Glass->SetMaterialPropertiesTable(mpt);

    fMapMaterial.insert({"Glass", Glass});
}

void MaterialManager::BuildEpoxy()
{
    // build material component
    G4Material *Epoxy = new G4Material("Epoxy", 1.19 * g / cm3, 3);
    Epoxy->AddElement(fElC, 5);
    Epoxy->AddElement(fElH, 8);
    Epoxy->AddElement(fElO, 2);
    // no optical property, photon will be absorbed by Epoxy
    fMapMaterial.insert({"Epoxy", Epoxy});
}

void MaterialManager::BuildGel()
{
    // build material component
    G4Material *Gel = new G4Material("Gel", 1.20 * g / cm3, 3);
    Gel->AddElement(fElC, 4);
    Gel->AddElement(fElH, 8);
    Gel->AddElement(fElO, 2);

    // build optical property
    G4MaterialPropertiesTable *mpt = new G4MaterialPropertiesTable();
    const G4int num = 2;
    G4double photonEnergy[num] = {2.06667 * eV, 4.13333 * eV};
    G4double refractiveIndex[num] = {1.41, 1.41};  // SilGel 601 A/B by Wacker
    G4double absorptionLength[num] = {10. * m, 10. * m};
    mpt->AddProperty("RINDEX", photonEnergy, refractiveIndex, num);
    mpt->AddProperty("ABSLENGTH", photonEnergy, absorptionLength, num);
    Gel->SetMaterialPropertiesTable(mpt);

    fMapMaterial.insert({"Gel", Gel});
}

void MaterialManager::BuildScint()
{
    // build material component
    G4Material *Scint = new G4Material("Scint", 1.023 * g / cm3, 2);
    Scint->AddElement(fElC, 10);
    Scint->AddElement(fElH, 11);
    Scint->GetIonisation()->SetBirksConstant(0.126*mm/MeV);
    Scint->SetMaterialPropertiesTable(SetOpticalPropertiesOfPS());
    // no optical property, photon will be absorbed by Scint
    fMapMaterial.insert({"Scint", Scint});
}

void MaterialManager::BuildESR()
{
    G4Material* Warp = G4NistManager::Instance()->FindOrBuildMaterial("G4_POLYETHYLENE");
    fMapMaterial.insert({"ESR", Warp});
}

void MaterialManager::BuildTape()
{
    G4Material* Tape = G4NistManager::Instance()->FindOrBuildMaterial("G4_POLYETHYLENE");
    fMapMaterial.insert({"Tape", Tape});
}

void MaterialManager::BuildSiPM()
{
    // build material component
    G4Material *SiPM = new G4Material("SiPM", 1.20 * g / cm3, 2);
    SiPM->AddElement(fElSi, 1);
    SiPM->AddElement(fElO, 2);
    fMapMaterial.insert({"SiPM", SiPM});

    // build optical property
    // To enable sensitive detector to detect photon hit, 
    // we have to set refractive index of material.
    G4MaterialPropertiesTable *mpt = new G4MaterialPropertiesTable();
    const G4int num = 2;
    G4double photonEnergy[num] = {2.06667 * eV, 4.13333 * eV};
    G4double refractiveIndex[num] = {1.50, 1.50};
    G4double absorptionLength[num] = {10. * m, 10. * m};
    mpt->AddProperty("RINDEX", photonEnergy, refractiveIndex, num);
    mpt->AddProperty("ABSLENGTH", photonEnergy, absorptionLength, num);
    SiPM->SetMaterialPropertiesTable(mpt);

    fMapMaterial.insert({"SiPM", SiPM});
}

void MaterialManager::BuildBattery()
{
    G4Material* Battery = G4NistManager::Instance()->FindOrBuildMaterial("G4_Al");
    fMapMaterial.insert({"Battery", Battery});
}

G4MaterialPropertiesTable* MaterialManager::SetOpticalPropertiesOfPS()
{

    G4MaterialPropertiesTable* mptPlScin = new G4MaterialPropertiesTable();

    const G4int nEntries= 43;//301;//100;

	G4double EJ200_SCINT[nEntries];
	G4double EJ200_RIND[nEntries];
	G4double EJ200_ABSL[nEntries];
	G4double photonEnergy[nEntries];

	std::ifstream ReadEJ200;
	G4int ScintEntry=0;
	G4String filler;
	G4double pEnergy;
	G4double pWavelength;
	G4double pSEff;
    string pathFile = ConfigManager::Instance()->GetNode("property.scintillator.spectrum_file").as<string>();
    
    // Extract config file directory to resolve relative paths
    std::filesystem::path configPath(fConfigPath.c_str());
    std::filesystem::path configDir = configPath.parent_path();
    
    // Resolve path relative to config file directory
    std::filesystem::path fullPath = configDir / pathFile;
	ReadEJ200.open(fullPath.string().c_str());
	if(ReadEJ200.is_open()){
    while(!ReadEJ200.eof()){
        ReadEJ200 >> pWavelength >> pSEff;
        pEnergy = (1240/pWavelength)*eV;
        photonEnergy[ScintEntry] = pEnergy;
        EJ200_SCINT[ScintEntry] = pSEff;
        if (spdlog::should_log(spdlog::level::debug)) {
            logger->debug("read-in energy scint: {:f} eff: {:f}", photonEnergy[ScintEntry], EJ200_SCINT[ScintEntry]);
        }
        ScintEntry++;
    }
	}
	else logger->error("Error opening file EJ200ScintSpectrum.txt");
	ReadEJ200.close();


	for (int i = 0; i < nEntries; i++) {
		EJ200_RIND[i] = 1.58;//58; // refractive index at 425 nm
		//EJ200_ABSL[i] *= myPSAttenuationLength;
		EJ200_ABSL[i] = 3.8*m;//2.5 * m; // bulk attenuation at 425 nm
	}

	mptPlScin->AddProperty("FASTCOMPONENT", photonEnergy, EJ200_SCINT,nEntries);//->SetSpline(true);


	mptPlScin->AddProperty("ABSLENGTH", photonEnergy, EJ200_ABSL,nEntries);//->SetSpline(true);

	mptPlScin->AddConstProperty("SCINTILLATIONYIELD", 100.0 / MeV); //--- according to EJ200
	mptPlScin->AddConstProperty("RESOLUTIONSCALE", 1.0);
	mptPlScin->AddConstProperty("FASTTIMECONSTANT", 2.1 * ns); //decay time, according to EJ200
	mptPlScin->AddProperty("RINDEX", photonEnergy, EJ200_RIND, nEntries);//->SetSpline(true);

    return mptPlScin;
}

float *MaterialManager::GetArrayProperites()
{
    int num = fSeaOpticalProperty.num;
    float *propertis = new float [5*num];
    for (int i = 0; i < num; i++)
    {
        int idx = 5*i;
        propertis[idx+0] = fSeaOpticalProperty.refracIdxPhase[i];
        propertis[idx+1] = fSeaOpticalProperty.refracIdxGroup[i];
        propertis[idx+2] = fSeaOpticalProperty.absLen[i];
        propertis[idx+3] = fSeaOpticalProperty.scaLenMie[i];
        propertis[idx+4] = fSeaOpticalProperty.scaLenRay[i];
    }
    return propertis;
}