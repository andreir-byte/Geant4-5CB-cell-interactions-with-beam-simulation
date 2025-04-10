// LCRunAction.hh - Updated to track filename state
#ifndef LCRunAction_h
#define LCRunAction_h 1

#include "G4UserRunAction.hh"
#include "globals.hh"
#include "G4SystemOfUnits.hh"

class G4Run;

class LCRunAction : public G4UserRunAction
{
  public:
    LCRunAction();
    virtual ~LCRunAction();

    // Methods from base class
    virtual void BeginOfRunAction(const G4Run*);
    virtual void EndOfRunAction(const G4Run*);
    
    // Set/get particle name
    void SetParticleName(const G4String& name) { fParticleName = name; }
    G4String GetParticleName() const { return fParticleName; }
    
    // Set/get particle energy
    void SetParticleEnergy(G4double energy);
    G4double GetParticleEnergy() const { return fParticleEnergy; }
    
    // Get current output filename
    G4String GetCurrentFileName() const { return fCurrentFileName; }

  private:
    G4String fParticleName;
    G4double fParticleEnergy;
    G4bool fFilenameGenerated;  // Flag to track if filename has been set
    G4String fCurrentFileName;  // Store current filename base
};

#endif
