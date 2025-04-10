// LCActionInitialization.cc - Updated for electrometer modeling and configurable beam
#include "LCActionInitialization.hh"
#include "LCPrimaryGeneratorAction.hh"
#include "LCRunAction.hh"
#include "LCEventAction.hh"
#include "LCSteppingAction.hh"
#include "LCDetectorConstruction.hh"
#include "LCMessenger.hh"
#include "G4SystemOfUnits.hh"

LCActionInitialization::LCActionInitialization(const LCDetectorConstruction* detConstruction)
 : G4VUserActionInitialization(),
   fDetConstruction(detConstruction),
   fParticleName("proton"),
   fParticleEnergy(0.5*GeV)
{}

LCActionInitialization::~LCActionInitialization()
{}

void LCActionInitialization::BuildForMaster() const
{
  // Create and configure RunAction for the master thread
  auto runAction = new LCRunAction();
  runAction->SetParticleName(fParticleName);
  runAction->SetParticleEnergy(fParticleEnergy);
  SetUserAction(runAction);
}

void LCActionInitialization::Build() const
{
  // Primary generator - with configured beam settings
  auto primaryGenerator = new LCPrimaryGeneratorAction();
  primaryGenerator->SetParticleType(fParticleName);
  primaryGenerator->SetParticleEnergy(fParticleEnergy);
  SetUserAction(primaryGenerator);
  
  // Run action - pass the same beam settings for consistency in output files
  auto runAction = new LCRunAction();
  runAction->SetParticleName(fParticleName);
  runAction->SetParticleEnergy(fParticleEnergy);
  SetUserAction(runAction);
  
  // Create messenger to allow run-time changes to beam parameters
  // Pass the detector construction as well for bias voltage control
  // This will be deleted automatically by G4
  new LCMessenger(primaryGenerator, runAction, const_cast<LCDetectorConstruction*>(fDetConstruction));
  
  // Event action
  auto eventAction = new LCEventAction();
  SetUserAction(eventAction);
  
  // Stepping action - now passes detector construction to access detector parameters
  auto steppingAction = new LCSteppingAction(fDetConstruction, eventAction);
  SetUserAction(steppingAction);
}
