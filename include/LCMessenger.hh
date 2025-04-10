// LCMessenger.hh - For runtime UI commands
#ifndef LCMessenger_h
#define LCMessenger_h 1

#include "globals.hh"
#include "G4UImessenger.hh"
#include "G4UIdirectory.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWithABool.hh"
#include "G4SystemOfUnits.hh"

class LCPrimaryGeneratorAction;
class LCRunAction;
class LCDetectorConstruction;

class LCMessenger : public G4UImessenger
{
  public:
    LCMessenger(LCPrimaryGeneratorAction* primaryAction, LCRunAction* runAction, 
                LCDetectorConstruction* detConstruction = nullptr);
    virtual ~LCMessenger();
    
    virtual void SetNewValue(G4UIcommand*, G4String);
    
  private:
    LCPrimaryGeneratorAction* fPrimaryAction;
    LCRunAction* fRunAction;
    LCDetectorConstruction* fDetConstruction;
    
    G4UIdirectory*             fLCDir;
    G4UIdirectory*             fBeamDir;
    G4UIdirectory*             fDetectorDir;
    G4UIcmdWithAString*        fParticleCmd;
    G4UIcmdWithADoubleAndUnit* fEnergyCmd;
    G4UIcmdWithABool*          fGlassFilterCmd;
    G4UIcmdWithADoubleAndUnit* fBiasCmd;
};

#endif
