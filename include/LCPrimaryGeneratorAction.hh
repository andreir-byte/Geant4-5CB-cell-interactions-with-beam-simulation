// LCPrimaryGeneratorAction.hh
#ifndef LCPrimaryGeneratorAction_h
#define LCPrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "globals.hh"

class G4ParticleGun;
class G4Event;
class G4Box;

class LCPrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
  public:
    LCPrimaryGeneratorAction();    
    virtual ~LCPrimaryGeneratorAction();

    // method from the base class
    virtual void GeneratePrimaries(G4Event*);         
  
    // method to access particle gun
    const G4ParticleGun* GetParticleGun() const { return fParticleGun; }
    
    // Set particle energy
    void SetParticleEnergy(G4double energy) { fParticleEnergy = energy; }
    
    // Get particle energy
    G4double GetParticleEnergy() const { return fParticleEnergy; }
    
    // Set particle type
    void SetParticleType(const G4String& name);
    
    // Get particle type
    G4String GetParticleType() const { return fParticleName; }
    
    // New methods for macro support
    // Set beam direction
    void SetBeamDirection(const G4ThreeVector& dir);
    
    // Enable/disable glass filter
    void SetGlassFilter(G4bool enable) { fGlassFilterEnabled = enable; }
    
    // Get glass filter status
    G4bool IsGlassFilterEnabled() const { return fGlassFilterEnabled; }
  
  private:
    G4ParticleGun*  fParticleGun; // pointer to G4 gun class
    G4Box* fEnvelopeBox;
    
    // Configurable parameters
    G4double fParticleEnergy;
    G4String fParticleName;
    G4ThreeVector fBeamDirection;  // Direction of the beam
    G4bool fGlassFilterEnabled;    // Flag for glass filter
};

#endif
