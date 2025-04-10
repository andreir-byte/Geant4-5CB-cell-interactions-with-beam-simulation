// LCEventAction.cc - Enhanced for electrometer current measurement with memory limits
#include "LCEventAction.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "G4AnalysisManager.hh"
#include <algorithm>
#include <numeric>

// Define units for convenience
namespace {
  const G4double picocoulomb = 1.0e-12 * coulomb;
  const G4double picoampere = 1.0e-12 * ampere;
}

// Define a maximum number of current samples to store
const G4int MAX_CURRENT_SAMPLES = 100000;

LCEventAction::LCEventAction() 
  : G4UserEventAction(),
    fTotalEnergyDeposit(0.),
    fTotalCharge(0.),
    fTotalElectrons(0),
    fTotalIons(0),
    fMaxCurrent(0.),
    fTotalCurrentIntegral(0.)
{
}

LCEventAction::~LCEventAction() 
{
}

void LCEventAction::BeginOfEventAction(const G4Event*) {
  // Initialize accumulators
  fTotalEnergyDeposit = 0.;
  fTotalCharge = 0.;
  fTotalElectrons = 0;
  fTotalIons = 0;
  
  // Clear electrometer data
  fCurrentProfile.clear();
  fMaxCurrent = 0.;
  fTotalCurrentIntegral = 0.;
}

void LCEventAction::AddCurrentPulse(G4double time, G4double current) {
  // Add a single current pulse to the electrometer reading
  // Only add if we haven't reached the limit
  if (fCurrentProfile.size() < MAX_CURRENT_SAMPLES) {
    fCurrentProfile.push_back(CurrentSample(time, current));
  }
  
  // Update max current if needed
  if(current > fMaxCurrent) {
    fMaxCurrent = current;
  }
  
  // For a more accurate current integral, we'd need to know the duration
  // Here we assume a simplistic model where each pulse contributes equally
  fTotalCurrentIntegral += current;
}

void LCEventAction::AddTimeProfile(G4double time, G4double current) {
  // This provides more detailed time profile for the electrometer
  // For example, tracking the current as charges drift over time
  // Only add if we're below the limit
  if (fCurrentProfile.size() < MAX_CURRENT_SAMPLES) {
    AddCurrentPulse(time, current);
  }
}

G4double LCEventAction::GetAverageElectrometerCurrent() const {
  if(fCurrentProfile.empty()) return 0.;
  
  // Calculate average current over all samples
  G4double totalCurrent = 0.;
  for(const auto& sample : fCurrentProfile) {
    totalCurrent += sample.current;
  }
  
  return totalCurrent / fCurrentProfile.size();
}

G4double LCEventAction::GetPeakElectrometerCurrent() const {
  return fMaxCurrent;
}

void LCEventAction::EndOfEventAction(const G4Event* event) {
  // Get analysis manager
  G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();
  
  // Calculate average and peak electrometer currents
  G4double avgCurrent = GetAverageElectrometerCurrent();
  G4double peakCurrent = GetPeakElectrometerCurrent();
  
  // Fill histograms with accumulated values
  analysisManager->FillH1(0, fTotalEnergyDeposit/keV);
  analysisManager->FillH1(1, fTotalCharge/picocoulomb);
  analysisManager->FillH1(2, avgCurrent/picoampere);
  analysisManager->FillH1(3, peakCurrent/picoampere);
  
  // Fill ntuple
  analysisManager->FillNtupleDColumn(0, fTotalEnergyDeposit/keV);
  analysisManager->FillNtupleDColumn(1, fTotalCharge/picocoulomb);
  analysisManager->FillNtupleIColumn(2, fTotalElectrons);
  analysisManager->FillNtupleIColumn(3, fTotalIons);
  analysisManager->FillNtupleDColumn(4, avgCurrent/picoampere);
  analysisManager->FillNtupleDColumn(5, peakCurrent/picoampere);
  
  // Time profile if available
  if(!fCurrentProfile.empty()) {
    // Sort by time for proper time-series analysis
    std::sort(fCurrentProfile.begin(), fCurrentProfile.end(),
              [](const CurrentSample& a, const CurrentSample& b) {
                return a.time < b.time;
              });
    
    // Fill time profile histogram - limit the number of points for memory
    G4int stepSize = std::max(1, static_cast<G4int>(fCurrentProfile.size() / 1000));
    for(size_t i = 0; i < fCurrentProfile.size(); i += stepSize) {
      const auto& sample = fCurrentProfile[i];
      analysisManager->FillH2(0, sample.time/ns, sample.current/picoampere);
    }
    
    // Fill the last sample point for this event
    auto lastSample = fCurrentProfile.back();
    analysisManager->FillNtupleDColumn(6, lastSample.time/ns);
    analysisManager->FillNtupleDColumn(7, lastSample.current/picoampere);
  }
  
  analysisManager->AddNtupleRow();
  
  // Print periodic update
  G4int eventID = event->GetEventID();
  if (eventID % 100 == 0) {
    G4cout << ">>> Event: " << eventID << G4endl;
    G4cout << "    Total energy deposit: " << fTotalEnergyDeposit/keV << " keV" << G4endl;
    G4cout << "    Charge created: " << fTotalCharge/picocoulomb << " pC" << G4endl;
    G4cout << "    Electron-ion pairs: " << fTotalElectrons << G4endl;
    G4cout << "    Electrometer current: " << avgCurrent/picoampere << " pA (avg), "
           << peakCurrent/picoampere << " pA (peak)" << G4endl;
    
    // Report number of current samples
    G4cout << "    Electrometer recorded " << fCurrentProfile.size();
    if (fCurrentProfile.size() >= MAX_CURRENT_SAMPLES) {
      G4cout << " (limit reached)";
    }
    G4cout << " current samples" << G4endl;
  }
}
