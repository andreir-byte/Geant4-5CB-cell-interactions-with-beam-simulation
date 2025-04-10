// LCGlobalManager.hh - Singleton to manage global beam settings
#ifndef LCGlobalManager_h
#define LCGlobalManager_h 1

#include "globals.hh"
#include "G4SystemOfUnits.hh"

class LCGlobalManager {
public:
    static LCGlobalManager* Instance();
    
    void SetParticleType(const G4String& name) { fParticleName = name; }
    void SetParticleEnergy(G4double energy) { fParticleEnergy = energy; }
    
    G4String GetParticleType() const { return fParticleName; }
    G4double GetParticleEnergy() const { return fParticleEnergy; }
    
private:
    LCGlobalManager();  // Private constructor (singleton)
    static LCGlobalManager* fInstance;
    
    G4String fParticleName;
    G4double fParticleEnergy;
};

#endif
