// LCGlobalManager.cc
#include "LCGlobalManager.hh"

// Initialize static member
LCGlobalManager* LCGlobalManager::fInstance = nullptr;

LCGlobalManager* LCGlobalManager::Instance() {
    if (!fInstance) {
        fInstance = new LCGlobalManager();
    }
    return fInstance;
}

LCGlobalManager::LCGlobalManager()
: fParticleName("proton"),
  fParticleEnergy(0.5*GeV)
{
    // Default values
}
