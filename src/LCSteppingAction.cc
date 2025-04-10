// LCSteppingAction.cc - Enhanced for electrometer current measurement with selective electrode interactions
#include "LCSteppingAction.hh"
#include "LCDetectorConstruction.hh"
#include "LCEventAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4StepPoint.hh"
#include "G4TrackStatus.hh"
#include "G4VProcess.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4AnalysisManager.hh"
#include "Randomize.hh"
#include <cmath>

// Define units for convenience
namespace {
  const G4double picocoulomb = 1.0e-12 * coulomb;
  const G4double picoampere = 1.0e-12 * ampere;
  const G4double femtoampere = 1.0e-15 * ampere;
}

LCSteppingAction::LCSteppingAction(const LCDetectorConstruction* detConstruction,
                                 LCEventAction* eventAction)
: G4UserSteppingAction(),
  fDetConstruction(detConstruction),
  fEventAction(eventAction),
  fElectricField(detConstruction->GetElectricField()),           // Get field from detector
  fMobilityElectron(1.0e-6*cm2/volt/s),  // Electron mobility in LC
  fMobilityIon(1.0e-8*cm2/volt/s),       // Ion mobility in LC
  fRecombinationCoef(1.0e-6*cm3/s),      // Recombination coefficient
  fCollectionEfficiency(0.8),            // 80% charge collection efficiency
  fEnergyPerIonization(30.0*eV),         // ~30 eV per ionization
  // Electrometer parameters
  fElectrometerResistance(1.0e9*ohm),    // 1 GΩ input resistance
  fElectrometerCapacitance(10.0*picofarad), // 10 pF input capacitance
  fElectrometerTimeConstant(1.0e9*ohm * 10.0*picofarad), // RC time constant
  fElectrometerSamplingRate(1.0e6*hertz), // 1 MHz sampling rate
  fTotalElectrons(0),
  fTotalIons(0)
{
  // Create histograms for charge distribution
  G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();
  
  // Histogram for energy deposition
  analysisManager->CreateH1("Edep", "Energy Deposit in Liquid Crystal", 100, 0., 500*keV);
  
  // Histogram for charge collection
  analysisManager->CreateH1("Charge", "Charge Collected", 100, 0., 100*picocoulomb);
  
  // Histograms for electrometer current
  analysisManager->CreateH1("AvgCurrent", "Average Electrometer Current", 100, 0., 1000*picoampere);
  analysisManager->CreateH1("PeakCurrent", "Peak Electrometer Current", 100, 0., 5000*picoampere);
  
  // 2D histogram for current vs time
  analysisManager->CreateH2("CurrentTime", "Electrometer Current vs Time", 
                           1000, 0., 1000*ns,  // Time bins
                           100, 0., 1000*picoampere); // Current bins
  
  // Histogram for spatial distribution of charge
  analysisManager->CreateH2("ChargeDist", "Charge Distribution in XY", 
                           100, -10*mm, 10*mm, 
                           100, -15*mm, 15*mm);
  
  // Create ntuple
  analysisManager->CreateNtuple("LCData", "Liquid Crystal Detector Data");
  analysisManager->CreateNtupleDColumn("Edep");             // keV
  analysisManager->CreateNtupleDColumn("Charge");           // pC
  analysisManager->CreateNtupleIColumn("ElectronCount");    // number
  analysisManager->CreateNtupleIColumn("IonCount");         // number
  analysisManager->CreateNtupleDColumn("AvgCurrent");       // pA
  analysisManager->CreateNtupleDColumn("PeakCurrent");      // pA
  analysisManager->CreateNtupleDColumn("FinalTime");        // ns
  analysisManager->CreateNtupleDColumn("FinalCurrent");     // pA
  analysisManager->FinishNtuple();
}

LCSteppingAction::~LCSteppingAction() 
{
}

void LCSteppingAction::UserSteppingAction(const G4Step* step) {
  // Get volume and particle information
  G4String volumeName = step->GetPreStepPoint()->GetTouchableHandle()
                      ->GetVolume()->GetLogicalVolume()->GetName();
  
  // Get particle information
  G4Track* track = step->GetTrack();
  G4ParticleDefinition* particle = track->GetDefinition();
  G4String particleName = particle->GetParticleName();
  
  // MODIFIED: Special handling for beam particles in electrodes - skip processing
  if ((volumeName == "ElectrodeFront" || volumeName == "ElectrodeBack") && 
      (particleName == "proton" || particleName == "gamma" || 
       particleName == "e+" || particleName == "neutron")) {
    // For primary beam particles, do nothing in electrodes
    return;
  }
  
  // MODIFIED: Handle secondary electrons/ions in electrodes
  if ((volumeName == "ElectrodeFront" || volumeName == "ElectrodeBack") && 
      (particleName == "e-" || particleName.find("ion") != G4String::npos)) {
    // These are charge carriers that reached the electrodes
    // They contribute to the current
    
    // Calculate charge
    G4double charge = 0.0;
    if (particleName == "e-") {
      charge = 1.602e-19 * coulomb;  // Electron charge (use positive for current direction)
    } else {
      // For ions, use charge state if available
      G4int chargeState = particle->GetPDGCharge();
      charge = std::abs(chargeState * 1.602e-19 * coulomb);
    }
    
    // Get current time
    G4double currentTime = track->GetGlobalTime();
    
    // Calculate simple current contribution (Q/Δt)
    G4double dt = 0.1*ns;  // Small time interval
    G4double instantCurrent = charge / dt;
    
    // Add to the electrometer through event action
    fEventAction->AddCurrentPulse(currentTime, instantCurrent);
    
    // Add some current profile samples
    for (G4int i = 0; i < 5; i++) {
      G4double sampleTime = currentTime + i * dt;
      G4double decayFactor = std::exp(-i * dt / fElectrometerTimeConstant);
      G4double sampleCurrent = instantCurrent * decayFactor;
      fEventAction->AddTimeProfile(sampleTime, sampleCurrent);
    }
    
    // Kill the track - it's been collected by the electrode
    track->SetTrackStatus(fStopAndKill);
    
    return;
  }
  
  // Normal processing for liquid crystal volume
  if (volumeName == "LCCell") {
    // Get energy deposit in this step
    G4double edep = step->GetTotalEnergyDeposit();
    
    if (edep > 0.) {
      // Get position information
      G4StepPoint* preStepPoint = step->GetPreStepPoint();
      G4StepPoint* postStepPoint = step->GetPostStepPoint();
      
      G4ThreeVector prePos = preStepPoint->GetPosition();
      G4ThreeVector postPos = postStepPoint->GetPosition();
      G4ThreeVector midPos = (prePos + postPos) / 2.0;
      
      // Calculate ionization events
      G4int numIonizationEvents = CalculateIonizationEvents(edep);
      
      // Apply collection efficiency
      G4int collectedElectrons = G4int(numIonizationEvents * fCollectionEfficiency);
      G4int collectedIons = collectedElectrons; // Same number collected
      
      // Calculate charge
      G4double charge = CalculateCharge(collectedElectrons);
      
      // MODIFIED: Distance to electrodes (for transit time calculation)
      // Now along Y-axis with the new orientation
      G4double cellThickness = fDetConstruction->GetLCThickness();
      G4double distanceToAnode = (cellThickness/2.0) - midPos.y();
      G4double distanceToCathode = (cellThickness/2.0) + midPos.y();
      
      // Transit times
      G4double electronTransitTime = distanceToAnode / (fMobilityElectron * fElectricField);
      G4double ionTransitTime = distanceToCathode / (fMobilityIon * fElectricField);
      
      // Current contribution 
      G4double electronCurrent = CalculateCurrentPulse(collectedElectrons, electronTransitTime);
      G4double ionCurrent = CalculateCurrentPulse(collectedIons, ionTransitTime);
      G4double totalCurrent = electronCurrent + ionCurrent;
      
      // Add to totals
      fTotalElectrons += collectedElectrons;
      fTotalIons += collectedIons;
      
      // Update event action
      fEventAction->AddEdep(edep);
      fEventAction->AddCharge(charge);
      fEventAction->AddElectronCount(collectedElectrons);
      fEventAction->AddIonCount(collectedIons);
      
      // Simulate electrometer response for electrons and ions
      SimulateElectrometerResponse(CalculateCharge(collectedElectrons), electronTransitTime);
      SimulateElectrometerResponse(CalculateCharge(collectedIons), ionTransitTime);
      
      // Fill histograms
      G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();
      
      // MODIFIED: Spatial distribution - now in X-Z plane for new orientation
      analysisManager->FillH2(1, midPos.x(), midPos.z(), collectedElectrons);
      
      // Debug output for significant energy deposits
      if (edep > 10.0*keV) {
        G4cout << "Significant energy deposit: " << edep/keV << " keV" << G4endl;
        G4cout << "  Position: (" << midPos.x()/mm << ", " << midPos.y()/mm << ", " << midPos.z()/mm << ") mm" << G4endl;
        G4cout << "  Electron-ion pairs: " << numIonizationEvents << G4endl;
        G4cout << "  Charge: " << charge/picocoulomb << " pC" << G4endl;
        G4cout << "  Current pulse: " << totalCurrent/picoampere << " pA" << G4endl;
      }
    }
  }
}

G4int LCSteppingAction::CalculateIonizationEvents(G4double energyDeposit) {
  // Calculate mean number of ionization events
  G4double meanIonizations = energyDeposit / fEnergyPerIonization;
  
  // Apply statistical fluctuations (Poisson distribution)
  G4int actualIonizations = G4int(meanIonizations + 0.5);  // Simple rounding as fallback
  
  // Add statistical fluctuations with normal distribution
  G4double sigma = std::sqrt(meanIonizations);  // Poisson variance = mean
  actualIonizations += G4int(G4RandGauss::shoot(0., sigma));
  if (actualIonizations < 0) actualIonizations = 0;  // Ensure non-negative
  
  return actualIonizations;
}

G4double LCSteppingAction::CalculateCharge(G4int numElectrons) {
  // Convert number of electrons to charge
  return numElectrons * 1.602e-19 * coulomb;
}

G4double LCSteppingAction::CalculateCurrentPulse(G4int numCharges, G4double transitTime) {
  // Simple current model: Q/t
  G4double charge = numCharges * 1.602e-19 * coulomb;
  
  // Apply statistical fluctuations to transit time
  G4double actualTransitTime = G4RandGauss::shoot(transitTime, 0.1*transitTime);
  if (actualTransitTime <= 0) actualTransitTime = transitTime; // Avoid negative time
  
  return charge / actualTransitTime;
}

void LCSteppingAction::SimulateElectrometerResponse(G4double charge, G4double transitTime) {
  // Get current simulation time
  G4double currentTime = G4RunManager::GetRunManager()->GetCurrentEvent()->GetPrimaryVertex()->GetT0();
  
  // Determine when this charge would reach the electrode
  G4double arrivalTime = currentTime + transitTime;
  
  // Store the current pulse information
  fCurrentPulses.push_back(CurrentPulse(arrivalTime, charge, transitTime));
  
  // Calculate the current at this time
  G4double instantCurrent = CalculateElectrometerCurrent(charge, transitTime);
  
  // Add to the electrometer current reading in event action
  fEventAction->AddCurrentPulse(arrivalTime, instantCurrent);
  
  // Now simulate the time profile by adding multiple samples
  // This models the electrometer's response over time
  
  // Number of samples based on electrometer sampling rate and transit time
  // MEMORY OPTIMIZATION: Limit the number of time samples to a reasonable amount
  const G4int MAX_TIME_SAMPLES_PER_PULSE = 100;
  G4int desiredSamples = static_cast<G4int>(transitTime * fElectrometerSamplingRate);
  G4int numSamples = std::min(MAX_TIME_SAMPLES_PER_PULSE, std::max(10, desiredSamples));
  
  // Calculate sample step size based on limited number
  G4double timeStep = transitTime / numSamples;
  
  for(G4int i=0; i<numSamples; i++) {
    // Calculate time for this sample
    G4double sampleTime = arrivalTime + i * timeStep;
    
    // Calculate current at this time based on electrometer model
    // Here we use a simple exponential decay model based on RC time constant
    G4double timeSinceArrival = sampleTime - arrivalTime;
    G4double decayFactor = std::exp(-timeSinceArrival / fElectrometerTimeConstant);
    
    // Current decreases exponentially after initial pulse
    G4double sampleCurrent = instantCurrent * decayFactor;
    
    // Add noise to the reading (typical electrometer noise is in femtoamperes)
    G4double noise = G4RandGauss::shoot(0.0, 10.0*femtoampere);
    sampleCurrent += noise;
    
    // Add this sample to the time profile
    fEventAction->AddTimeProfile(sampleTime, sampleCurrent);
  }
}

G4double LCSteppingAction::CalculateElectrometerCurrent(G4double charge, G4double transitTime) {
  // Base current: I = Q/t
  G4double baseCurrent = charge / transitTime;
  
  // Apply electrometer response characteristics
  // 1. Input impedance effect: Reduces current slightly
  G4double impedanceEffect = 1.0 - std::exp(-transitTime / fElectrometerTimeConstant);
  
  // 2. Apply measurement uncertainty
  G4double uncertainty = 0.01; // 1% uncertainty
  G4double measuredCurrent = baseCurrent * impedanceEffect * 
                            (1.0 + G4RandGauss::shoot(0.0, uncertainty));
  
  return measuredCurrent;
}
