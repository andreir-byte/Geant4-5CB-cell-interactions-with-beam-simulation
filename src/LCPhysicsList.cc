// LCPhysicsList.cc
#include "LCPhysicsList.hh"

#include "G4DecayPhysics.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4EmExtraPhysics.hh"
#include "G4HadronElasticPhysics.hh"
#include "G4HadronPhysicsFTFP_BERT.hh"
#include "G4IonPhysics.hh"
#include "G4StoppingPhysics.hh"

#include "G4SystemOfUnits.hh"

LCPhysicsList::LCPhysicsList() : G4VModularPhysicsList()
{
  G4int verb = 0;
  SetVerboseLevel(verb);
  
  // EM Physics - Option4 is most accurate
  RegisterPhysics(new G4EmStandardPhysics_option4(verb));
  
  // Synchroton Radiation & GN Physics
  RegisterPhysics(new G4EmExtraPhysics(verb));
  
  // Decays
  RegisterPhysics(new G4DecayPhysics(verb));
  
  // Hadron Elastic scattering
  RegisterPhysics(new G4HadronElasticPhysics(verb));
  
  // Hadron Physics
  RegisterPhysics(new G4HadronPhysicsFTFP_BERT(verb));
  
  // Stopping Physics
  RegisterPhysics(new G4StoppingPhysics(verb));
  
  // Ion Physics
  RegisterPhysics(new G4IonPhysics(verb));
}

LCPhysicsList::~LCPhysicsList()
{
}

void LCPhysicsList::SetCuts()
{
  // Set lower production cut for better accuracy
  // Note: This will slow down the simulation slightly
  SetCutValue(0.01*mm, "gamma");
  SetCutValue(0.01*mm, "e-");
  SetCutValue(0.01*mm, "e+");
  SetCutValue(0.01*mm, "proton");
}
