// LCDetectorConstruction.cc - Modified for perpendicular beam incidence with selective electrode interactions
#include "LCDetectorConstruction.hh"
#include "G4SystemOfUnits.hh"
#include "G4NistManager.hh"
#include "G4VisAttributes.hh"
#include "G4SDManager.hh"
#include "G4TransportationManager.hh"
#include "G4PVPlacement.hh"
#include "G4RotationMatrix.hh"
#include "G4UserLimits.hh"
#include "G4Material.hh"
#include "G4Region.hh"
#include "G4ProductionCuts.hh"

LCDetectorConstruction::LCDetectorConstruction() :
  fElectricField(nullptr),
  fEquation(nullptr),
  fStepper(nullptr),
  fFieldManager(nullptr),
  fMinStep(0.01*mm),
  fIntegratorDriver(nullptr),
  fChordFinder(nullptr),
  // Detector parameters
  lcSizeX(15.0*mm),    // 15 mm width
  lcSizeY(25.0*mm),    // 25 mm length
  lcSizeZ(100.0*um),   // 100 microns thickness
  electricFieldStrength(3.0*volt/um), // 3 V/μm field strength
  biasVoltage(300*volt)  // Initialize with 300V bias
{
  DefineMaterials();
}

LCDetectorConstruction::~LCDetectorConstruction() {
  delete fChordFinder;
  delete fIntegratorDriver;
  delete fStepper;
  delete fEquation;
  delete fElectricField;
}

void LCDetectorConstruction::SetBias(G4double biasVoltage) {
  // Store the bias voltage
  this->biasVoltage = biasVoltage;
  
  // Calculate new electric field based on bias and detector thickness
  // Field strength = Voltage / Distance
  electricFieldStrength = biasVoltage / lcSizeZ;
  
  // Update the electric field if it's already been created
  if (fElectricField && fFieldManager) {
    // For G4UniformElectricField, we can't update it directly
    // We need to create a new field and set it to the field manager
    
    // Delete the old field objects
    delete fChordFinder;
    delete fIntegratorDriver;
    delete fStepper;
    delete fEquation;
    delete fElectricField;
    
    // Create new field with updated strength
    G4ThreeVector fieldVector(0.0, electricFieldStrength, 0.0);
    fElectricField = new G4UniformElectricField(fieldVector);
    
    // Recreate equation of motion with new electric field
    fEquation = new G4EqMagElectricField(fElectricField);
    
    // Recreate stepper
    G4int nvar = 8;
    fStepper = new G4ClassicalRK4(fEquation, nvar);
    
    // Recreate driver
    fIntegratorDriver = new G4MagInt_Driver(fMinStep, fStepper, fStepper->GetNumberOfVariables());
    
    // Recreate chord finder
    fChordFinder = new G4ChordFinder(fIntegratorDriver);
    
    // Update field manager
    fFieldManager->SetDetectorField(fElectricField);
    fFieldManager->SetChordFinder(fChordFinder);
    
    G4cout << "Electric field updated to " << electricFieldStrength/(volt/um) 
           << " V/μm = " << electricFieldStrength * lcSizeZ/volt << " V across detector" << G4endl;
  } else {
    G4cout << "Electric field not yet initialized. Will use new bias of " 
           << biasVoltage/volt << " V when created." << G4endl;
  }
}

void LCDetectorConstruction::DefineMaterials() {
  G4NistManager* nistManager = G4NistManager::Instance();
  
  // Define materials for world volume and electrodes
  worldMaterial = nistManager->FindOrBuildMaterial("G4_AIR");
  wireConnectionMaterial = nistManager->FindOrBuildMaterial("G4_Cu");  // Copper wires
  electrodeConnectorMaterial = nistManager->FindOrBuildMaterial("G4_Ag");  // Silver contacts
  electrometerCaseMaterial = nistManager->FindOrBuildMaterial("G4_Al");    // Aluminum case
  
  // FIXED: Use the standard glass material directly - we'll use physics process controls
  // rather than trying to modify the material composition
  electrodeMaterial = nistManager->FindOrBuildMaterial("G4_GLASS_PLATE");
  
  // Define liquid crystal material (5CB: 4-Cyano-4'-pentylbiphenyl)
  G4double density = 1.02*g/cm3;
  liquidCrystalMaterial = new G4Material("LiquidCrystal_5CB", density, 3);
  
  G4Element* elC = nistManager->FindOrBuildElement("C");
  G4Element* elH = nistManager->FindOrBuildElement("H");
  G4Element* elN = nistManager->FindOrBuildElement("N");
  
  // 5CB molecular formula: C18H19N
  liquidCrystalMaterial->AddElement(elC, 18);
  liquidCrystalMaterial->AddElement(elH, 19);
  liquidCrystalMaterial->AddElement(elN, 1);
}

void LCDetectorConstruction::SetupElectricField() {
  // Create uniform electric field along y-axis (perpendicular to large face)
  G4ThreeVector fieldVector(0.0, electricFieldStrength, 0.0);
  fElectricField = new G4UniformElectricField(fieldVector);
  
  // Create equation of motion with electric field
  fEquation = new G4EqMagElectricField(fElectricField);
  
  // Create stepper for field integration
  G4int nvar = 8;
  fStepper = new G4ClassicalRK4(fEquation, nvar);
  
  // Create driver with stepper
  fIntegratorDriver = new G4MagInt_Driver(fMinStep, fStepper, fStepper->GetNumberOfVariables());
  
  // Create chord finder
  fChordFinder = new G4ChordFinder(fIntegratorDriver);
  
  // Configure field manager
  fFieldManager = G4TransportationManager::GetTransportationManager()->GetFieldManager();
  fFieldManager->SetDetectorField(fElectricField);
  fFieldManager->SetChordFinder(fChordFinder);
}

G4VPhysicalVolume* LCDetectorConstruction::Construct() {
  // World volume
  G4double worldSizeX = 30.0*cm;
  G4double worldSizeY = 30.0*cm;
  G4double worldSizeZ = 30.0*cm;
  
  worldSolid = new G4Box("World", worldSizeX/2, worldSizeY/2, worldSizeZ/2);
  worldLogical = new G4LogicalVolume(worldSolid, worldMaterial, "World");
  worldPhysical = new G4PVPlacement(0, G4ThreeVector(), worldLogical, "World", 0, false, 0);
  
  // Liquid crystal cell - reoriented for perpendicular incidence
  // Now the large face (X-Z plane) is perpendicular to the beam (Y-axis)
  lcCellSolid = new G4Box("LCCell", lcSizeX/2, lcSizeZ/2, lcSizeY/2);
  lcCellLogical = new G4LogicalVolume(lcCellSolid, liquidCrystalMaterial, "LCCell");
  lcCellPhysical = new G4PVPlacement(0, G4ThreeVector(0, 0, 0), lcCellLogical, "LCCell", worldLogical, false, 0);
  
  // ITO Glass electrodes (front and back along Y-axis)
  // Slightly larger than LC cell (by 2mm in X and Z dimensions)
  G4double electrodeSizeX = lcSizeX + 2.0*mm;
  G4double electrodeSizeY = 1.0*mm;  // Electrode thickness
  G4double electrodeSizeZ = lcSizeY + 2.0*mm;
  
  // Front electrode (facing the beam)
  electrodeTopSolid = new G4Box("ElectrodeFront", electrodeSizeX/2, electrodeSizeY/2, electrodeSizeZ/2);
  electrodeTopLogical = new G4LogicalVolume(electrodeTopSolid, electrodeMaterial, "ElectrodeFront");
  
  // FIXED: Create a region for special physics cuts for electrodes
  G4Region* electrodeRegion = new G4Region("ElectrodeRegion");
  
  // Set very high production cuts for this region to minimize secondary particle production
  G4ProductionCuts* electrodeCuts = new G4ProductionCuts();
  electrodeCuts->SetProductionCut(1.0*km); // Effectively disable secondary production
  electrodeRegion->SetProductionCuts(electrodeCuts);
  
  // Add logical volumes to region
  electrodeTopLogical->SetRegion(electrodeRegion);
  electrodeRegion->AddRootLogicalVolume(electrodeTopLogical);
  
  // Add user limits to minimize interactions but preserve electric field
  G4UserLimits* electrodePhysicsLimits = new G4UserLimits();
  electrodePhysicsLimits->SetMaxAllowedStep(10.0*m);  // Large step for protons (jump through)
  electrodeTopLogical->SetUserLimits(electrodePhysicsLimits);
  
  electrodeTopPhysical = new G4PVPlacement(0, G4ThreeVector(0, -lcSizeZ/2 - electrodeSizeY/2, 0), 
                         electrodeTopLogical, "ElectrodeFront", worldLogical, false, 0);
  
  // Back electrode (away from the beam)
  electrodeBottomSolid = new G4Box("ElectrodeBack", electrodeSizeX/2, electrodeSizeY/2, electrodeSizeZ/2);
  electrodeBottomLogical = new G4LogicalVolume(electrodeBottomSolid, electrodeMaterial, "ElectrodeBack");
  
  // Apply the same region and limits to the back electrode
  electrodeBottomLogical->SetRegion(electrodeRegion);
  electrodeRegion->AddRootLogicalVolume(electrodeBottomLogical);
  electrodeBottomLogical->SetUserLimits(electrodePhysicsLimits);
  
  electrodeBottomPhysical = new G4PVPlacement(0, G4ThreeVector(0, lcSizeZ/2 + electrodeSizeY/2, 0), 
                            electrodeBottomLogical, "ElectrodeBack", worldLogical, false, 0);
  
  // Wire connections to the electrometer (copper wires)
  G4double wireRadius = 0.5*mm;
  G4double wireLength = 50.0*mm;
  
  // Front wire (from front electrode to electrometer)
  topWireSolid = new G4Tubs("FrontWire", 0, wireRadius, wireLength/2, 0, 360*deg);
  topWireLogical = new G4LogicalVolume(topWireSolid, wireConnectionMaterial, "FrontWireLogical");
  
  // Position the front wire
  G4RotationMatrix* frontWireRot = new G4RotationMatrix();
  frontWireRot->rotateZ(90*deg);
  frontWireRot->rotateX(30*deg);
  G4ThreeVector frontWirePos(electrodeSizeX/2 - 2*mm, -lcSizeZ/2 - electrodeSizeY, wireLength/4);
  topWirePhysical = new G4PVPlacement(frontWireRot, frontWirePos, topWireLogical, "FrontWire", worldLogical, false, 0);
  
  // Back wire (from back electrode to electrometer)
  bottomWireSolid = new G4Tubs("BackWire", 0, wireRadius, wireLength/2, 0, 360*deg);
  bottomWireLogical = new G4LogicalVolume(bottomWireSolid, wireConnectionMaterial, "BackWireLogical");
  
  // Position the back wire
  G4RotationMatrix* backWireRot = new G4RotationMatrix();
  backWireRot->rotateZ(90*deg);
  backWireRot->rotateX(-30*deg);
  G4ThreeVector backWirePos(-electrodeSizeX/2 + 2*mm, lcSizeZ/2 + electrodeSizeY, wireLength/4);
  bottomWirePhysical = new G4PVPlacement(backWireRot, backWirePos, bottomWireLogical, "BackWire", worldLogical, false, 0);
  
  // Electrometer device (simplified as a box)
  G4double electrometerSizeX = 8.0*cm;
  G4double electrometerSizeY = 6.0*cm;
  G4double electrometerSizeZ = 3.0*cm;
  
  // Create and position the electrometer to the right of the detector
  electrometerSolid = new G4Box("Electrometer", electrometerSizeX/2, electrometerSizeY/2, electrometerSizeZ/2);
  electrometerLogical = new G4LogicalVolume(electrometerSolid, electrometerCaseMaterial, "ElectrometerLogical");
  
  // Position the electrometer to the right of the detector setup
  G4ThreeVector electrometerPos(10.0*cm, 0, 0);
  electrometerPhysical = new G4PVPlacement(0, electrometerPos, electrometerLogical, "Electrometer", worldLogical, false, 0);
  
  // Set up electric field
  SetupElectricField();
  
  // Visual attributes
  G4VisAttributes* lcVisAtt = new G4VisAttributes(G4Colour(0.0, 0.0, 1.0, 0.3));  // Blue, semi-transparent
  lcCellLogical->SetVisAttributes(lcVisAtt);
  
  // Use glass-like appearance for electrodes while maintaining non-interaction with protons
  G4VisAttributes* electrodeVisAtt = new G4VisAttributes(G4Colour(0.7, 0.7, 0.7, 0.5));  // Gray, semi-transparent
  electrodeTopLogical->SetVisAttributes(electrodeVisAtt);
  electrodeBottomLogical->SetVisAttributes(electrodeVisAtt);
  
  G4VisAttributes* wireVisAtt = new G4VisAttributes(G4Colour(0.8, 0.5, 0.2));  // Copper color
  topWireLogical->SetVisAttributes(wireVisAtt);
  bottomWireLogical->SetVisAttributes(wireVisAtt);
  
  G4VisAttributes* electrometerVisAtt = new G4VisAttributes(G4Colour(0.4, 0.4, 0.4));  // Dark gray
  electrometerLogical->SetVisAttributes(electrometerVisAtt);
  
  // Print detector information
  G4cout << "\n--------- 5CB Liquid Crystal Detector Parameters ---------" << G4endl;
  G4cout << "Detector dimensions: " << lcSizeX/mm << " mm × " << lcSizeY/mm << " mm × " << lcSizeZ/um << " μm" << G4endl;
  G4cout << "MODIFIED: Beam hits the large rectangular face (" << lcSizeX/mm << " mm × " << lcSizeY/mm << " mm)" << G4endl;
  G4cout << "MODIFIED: Using production cuts to minimize proton interactions in electrodes" << G4endl;
  G4cout << "Electric field strength: " << electricFieldStrength/(volt/um) << " V/μm = " 
         << electricFieldStrength * lcSizeZ/volt << " V across detector" << G4endl;
  G4cout << "Active volume: " << (lcSizeX*lcSizeY*lcSizeZ)/mm3 << " mm³" << G4endl;
  G4cout << "Electrometer connections: Explicitly modeled with wires" << G4endl;
  G4cout << "Bias voltage: " << biasVoltage/volt << " V" << G4endl;
  G4cout << "----------------------------------------------------------\n" << G4endl;
  
  return worldPhysical;
}
