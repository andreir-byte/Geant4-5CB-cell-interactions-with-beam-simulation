// LCMessenger.cc - Complete implementation with new commands
#include "LCMessenger.hh"
#include "LCPrimaryGeneratorAction.hh"
#include "LCRunAction.hh"
#include "LCDetectorConstruction.hh"
#include "LCGlobalManager.hh"
#include "G4RunManager.hh"

LCMessenger::LCMessenger(LCPrimaryGeneratorAction* primaryAction, LCRunAction* runAction,
                         LCDetectorConstruction* detConstruction)
: G4UImessenger(),
  fPrimaryAction(primaryAction),
  fRunAction(runAction),
  fDetConstruction(detConstruction)
{
  // Create main LC directory
  fLCDir = new G4UIdirectory("/LC/");
  fLCDir->SetGuidance("Liquid Crystal Detector commands");

  // Create directory for beam commands
  fBeamDir = new G4UIdirectory("/LC/beam/");
  fBeamDir->SetGuidance("Beam configuration commands");
  
  // Create directory for detector commands
  fDetectorDir = new G4UIdirectory("/LC/detector/");
  fDetectorDir->SetGuidance("Detector configuration commands");
  
  // Command to set particle type
  fParticleCmd = new G4UIcmdWithAString("/LC/beam/particle", this);
  fParticleCmd->SetGuidance("Set particle type (e.g., proton, e-, gamma)");
  fParticleCmd->SetParameterName("ParticleType", false);
  fParticleCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
  
  // Command to set particle energy
  fEnergyCmd = new G4UIcmdWithADoubleAndUnit("/LC/beam/energy", this);
  fEnergyCmd->SetGuidance("Set particle energy");
  fEnergyCmd->SetParameterName("Energy", false);
  fEnergyCmd->SetUnitCategory("Energy");
  fEnergyCmd->SetUnitCandidates("eV keV MeV GeV");
  fEnergyCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
  
  // Command to set glass filter
  fGlassFilterCmd = new G4UIcmdWithABool("/LC/beam/glassFilter", this);
  fGlassFilterCmd->SetGuidance("Enable/disable glass filter before detector");
  fGlassFilterCmd->SetParameterName("GlassFilter", false);
  fGlassFilterCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
  
  // Command to set detector bias voltage
  fBiasCmd = new G4UIcmdWithADoubleAndUnit("/LC/detector/bias", this);
  fBiasCmd->SetGuidance("Set detector bias voltage");
  fBiasCmd->SetParameterName("Bias", false);
  fBiasCmd->SetUnitCategory("Electric potential");
  fBiasCmd->SetUnitCandidates("volt kV");
  fBiasCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
}

LCMessenger::~LCMessenger()
{
  delete fParticleCmd;
  delete fEnergyCmd;
  delete fGlassFilterCmd;
  delete fBiasCmd;
  delete fBeamDir;
  delete fDetectorDir;
  delete fLCDir;
}

void LCMessenger::SetNewValue(G4UIcommand* command, G4String newValue)
{
  // Set particle type
  if (command == fParticleCmd) {
    fPrimaryAction->SetParticleType(newValue);
    fRunAction->SetParticleName(newValue);
    // Update global manager
    LCGlobalManager::Instance()->SetParticleType(newValue);
    G4cout << "Particle type set to " << newValue << G4endl;
  }
  
  // Set particle energy
  else if (command == fEnergyCmd) {
    G4double energy = fEnergyCmd->GetNewDoubleValue(newValue);
    fPrimaryAction->SetParticleEnergy(energy);
    fRunAction->SetParticleEnergy(energy);
    // Update global manager
    LCGlobalManager::Instance()->SetParticleEnergy(energy);
    G4cout << "Particle energy set to " << energy/MeV << " MeV" << G4endl;
    
    // Check if a run is in progress - warn about filename implications
    if (G4RunManager::GetRunManager()->GetCurrentRun()) {
      G4cout << "NOTE: Changing energy during an active run may cause filename inconsistency." << G4endl;
      G4cout << "      For proper filename with current energy: stop run, change energy, start new run" << G4endl;
    }
  }
  
  // Set glass filter
  else if (command == fGlassFilterCmd) {
    G4bool enableFilter = fGlassFilterCmd->GetNewBoolValue(newValue);
    if (fPrimaryAction) {
      fPrimaryAction->SetGlassFilter(enableFilter);
      G4cout << "Glass filter " << (enableFilter ? "enabled" : "disabled") << G4endl;
    } else {
      G4cerr << "ERROR: Primary generator action not available for glass filter command" << G4endl;
    }
  }
  
  // Set detector bias voltage
  else if (command == fBiasCmd) {
    G4double bias = fBiasCmd->GetNewDoubleValue(newValue);
    if (fDetConstruction) {
      fDetConstruction->SetBias(bias);
      G4cout << "Detector bias set to " << bias/volt << " V" << G4endl;
    } else {
      G4cerr << "ERROR: Detector construction not available for bias command" << G4endl;
    }
  }
}
