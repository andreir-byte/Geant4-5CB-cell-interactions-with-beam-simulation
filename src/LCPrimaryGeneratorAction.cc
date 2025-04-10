// LCPrimaryGeneratorAction.cc - Modified for perpendicular incidence
#include "LCPrimaryGeneratorAction.hh"

#include "G4LogicalVolumeStore.hh"
#include "G4LogicalVolume.hh"
#include "G4Box.hh"
#include "G4RunManager.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"

LCPrimaryGeneratorAction::LCPrimaryGeneratorAction()
: G4VUserPrimaryGeneratorAction(),
  fParticleGun(nullptr), 
  fEnvelopeBox(nullptr),
  fParticleEnergy(0.5*GeV),
  fParticleName("proton"),
  fBeamDirection(G4ThreeVector(0.,1.,0.)),
  fGlassFilterEnabled(false)
{
  G4int n_particle = 1;
  fParticleGun = new G4ParticleGun(n_particle);

  // Default particle kinematic
  G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
  G4ParticleDefinition* particle = particleTable->FindParticle(fParticleName);
  fParticleGun->SetParticleDefinition(particle);
  
  // MODIFIED: Changed beam direction to hit the rectangular face (along Y-axis)
  fParticleGun->SetParticleMomentumDirection(fBeamDirection);
  fParticleGun->SetParticleEnergy(fParticleEnergy);
}

LCPrimaryGeneratorAction::~LCPrimaryGeneratorAction()
{
  delete fParticleGun;
}

void LCPrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
{
  // This function is called at the beginning of each event
  
  // Update energy in case it has been changed
  fParticleGun->SetParticleEnergy(fParticleEnergy);
  
  // Update direction in case it has been changed
  fParticleGun->SetParticleMomentumDirection(fBeamDirection);

  // Base position settings
  G4double x0 = 0;
  G4double y0 = -15.0*mm; // Position beam 15mm before the detector in Y direction
  G4double z0 = 0;        // Center beam on Z axis
  
  // Apply beam spot size if requested
  // For Gaussian beam profile, sample from Gaussian distribution
  G4double sigmaX = 3.0*mm; // 3mm sigma in X
  G4double sigmaZ = 3.0*mm; // 3mm sigma in Z (was Y in original)
  
  // Sample from Gaussian distribution
  if(sigmaX > 0 && sigmaZ > 0) {
    x0 = G4RandGauss::shoot(0., sigmaX);
    z0 = G4RandGauss::shoot(0., sigmaZ);
  }
  
  // If glass filter is enabled, modify beam energy according to filter attenuation
  if (fGlassFilterEnabled) {
    // Simplified glass filter energy reduction - more sophisticated model could be used
    if (fParticleName == "gamma") {
      // Approximate exponential attenuation for gammas
      G4double attenuation = G4RandExponential::shoot(2.0);
      G4double attenuatedEnergy = fParticleEnergy * std::exp(-attenuation);
      fParticleGun->SetParticleEnergy(attenuatedEnergy);
    }
    else if (fParticleName == "e-" || fParticleName == "e+") {
      // Electrons and positrons lose significant energy
      G4double attenuatedEnergy = fParticleEnergy * 0.6; // 40% energy loss
      fParticleGun->SetParticleEnergy(attenuatedEnergy);
    }
    else if (fParticleName == "alpha") {
      // Alpha particles are completely stopped by glass
      fParticleGun->SetParticleEnergy(0.001*MeV); // Effectively stopped
    }
    else if (fParticleName == "neutron") {
      // Neutrons have minimal interaction with glass
      // No change to energy
    }
    else {
      // Default basic energy reduction for other particles
      G4double attenuatedEnergy = fParticleEnergy * 0.9; // 10% energy loss
      fParticleGun->SetParticleEnergy(attenuatedEnergy);
    }
    
    // Adjust position to account for glass filter placement
    y0 -= 3.0*mm; // Move source back by glass thickness
  }
  
  fParticleGun->SetParticlePosition(G4ThreeVector(x0, y0, z0));
  fParticleGun->GeneratePrimaryVertex(anEvent);
}

void LCPrimaryGeneratorAction::SetParticleType(const G4String& name)
{
  fParticleName = name;
  G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
  G4ParticleDefinition* particle = particleTable->FindParticle(name);
  if (particle) {
    fParticleGun->SetParticleDefinition(particle);
  } else {
    G4cout << "Warning: Particle " << name << " not found. Using previous particle." << G4endl;
  }
}

void LCPrimaryGeneratorAction::SetBeamDirection(const G4ThreeVector& dir)
{
  // Normalize the direction vector
  G4ThreeVector normalized = dir.unit();
  fBeamDirection = normalized;
  
  // Update particle gun immediately
  if (fParticleGun) {
    fParticleGun->SetParticleMomentumDirection(fBeamDirection);
  }
  
  G4cout << "Beam direction set to (" 
         << fBeamDirection.x() << ", " 
         << fBeamDirection.y() << ", " 
         << fBeamDirection.z() << ")" << G4endl;
}
