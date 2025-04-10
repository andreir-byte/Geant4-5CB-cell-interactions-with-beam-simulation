// LCActionInitialization.hh - Updated for electrometer modeling
#ifndef LCActionInitialization_h
#define LCActionInitialization_h 1

#include "G4VUserActionInitialization.hh"
#include "globals.hh"

class LCDetectorConstruction;

class LCActionInitialization : public G4VUserActionInitialization
{
  public:
    LCActionInitialization(const LCDetectorConstruction* detConstruction);
    virtual ~LCActionInitialization();

    virtual void BuildForMaster() const;
    virtual void Build() const;
    
    // Methods to configure the particle beam
    void SetBeamParticle(const G4String& particleName) { fParticleName = particleName; }
    void SetBeamEnergy(G4double energy) { fParticleEnergy = energy; }
    
    G4String GetBeamParticle() const { return fParticleName; }
    G4double GetBeamEnergy() const { return fParticleEnergy; }
    
  private:
    const LCDetectorConstruction* fDetConstruction;
    G4String fParticleName;
    G4double fParticleEnergy;
};

#endif
