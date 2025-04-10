// LCDetectorConstruction.hh - Updated for phase-through electrodes
#ifndef LCDetectorConstruction_h
#define LCDetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4Material.hh"
#include "G4ElectricField.hh"
#include "G4UniformElectricField.hh"
#include "G4EqMagElectricField.hh"
#include "G4MagIntegratorStepper.hh"
#include "G4FieldManager.hh"
#include "G4ClassicalRK4.hh"
#include "G4MagIntegratorDriver.hh"
#include "G4ChordFinder.hh"

class LCDetectorConstruction : public G4VUserDetectorConstruction {
  public:
    LCDetectorConstruction();
    ~LCDetectorConstruction();
    
    virtual G4VPhysicalVolume* Construct();
    
    // Getters for detector parameters
    G4double GetLCThickness() const { return lcSizeZ; }
    G4double GetElectricField() const { return electricFieldStrength; }
    G4double GetLCWidth() const { return lcSizeX; }
    G4double GetLCLength() const { return lcSizeY; }
    
    // Method to set the bias voltage (affects electric field)
    void SetBias(G4double biasVoltage);
    
  private:
    void DefineMaterials();
    void SetupElectricField();
    
    // Materials
    G4Material* worldMaterial;
    G4Material* liquidCrystalMaterial;
    G4Material* electrodeMaterial;        // The material actually used in simulation
    G4Material* ghostElectrodeMaterial;   // Non-interacting electrode material
    G4Material* realElectrodeMaterial;    // Real electrode material properties (for reference)
    G4Material* wireConnectionMaterial;
    G4Material* electrodeConnectorMaterial;
    G4Material* electrometerCaseMaterial;
    
    // Volumes
    G4Box* worldSolid;
    G4LogicalVolume* worldLogical;
    G4VPhysicalVolume* worldPhysical;
    
    G4Box* lcCellSolid;
    G4LogicalVolume* lcCellLogical;
    G4VPhysicalVolume* lcCellPhysical;
    
    G4Box* electrodeTopSolid;
    G4LogicalVolume* electrodeTopLogical;
    G4VPhysicalVolume* electrodeTopPhysical;
    
    G4Box* electrodeBottomSolid;
    G4LogicalVolume* electrodeBottomLogical;
    G4VPhysicalVolume* electrodeBottomPhysical;
    
    // Wire connections to electrometer
    G4Tubs* topWireSolid;
    G4LogicalVolume* topWireLogical;
    G4VPhysicalVolume* topWirePhysical;
    
    G4Tubs* bottomWireSolid;
    G4LogicalVolume* bottomWireLogical;
    G4VPhysicalVolume* bottomWirePhysical;
    
    // Electrometer volume representation
    G4Box* electrometerSolid;
    G4LogicalVolume* electrometerLogical;
    G4VPhysicalVolume* electrometerPhysical;
    
    // Electric Field
    G4ElectricField* fElectricField;
    G4EqMagElectricField* fEquation;
    G4MagIntegratorStepper* fStepper;
    G4FieldManager* fFieldManager;
    G4double fMinStep;
    G4MagInt_Driver* fIntegratorDriver;
    G4ChordFinder* fChordFinder;
    
    // Detector parameters
    G4double lcSizeX;    // LC width (15 mm)
    G4double lcSizeY;    // LC length (25 mm)
    G4double lcSizeZ;    // LC thickness (100 μm)
    G4double electricFieldStrength;  // Electric field strength (3 V/μm)
    G4double biasVoltage; // Current bias voltage
};

#endif
