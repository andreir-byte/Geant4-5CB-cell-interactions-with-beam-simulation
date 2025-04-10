// LCEventAction.hh - Enhanced for electrometer current measurement
#ifndef LCEventAction_h
#define LCEventAction_h 1

#include "G4UserEventAction.hh"
#include "globals.hh"
#include <vector>

class LCEventAction : public G4UserEventAction {
  public:
    LCEventAction();
    virtual ~LCEventAction();
    
    virtual void BeginOfEventAction(const G4Event*);
    virtual void EndOfEventAction(const G4Event*);
    
    // Methods to accumulate energy and charge
    void AddEdep(G4double edep) { fTotalEnergyDeposit += edep; }
    void AddCharge(G4double charge) { fTotalCharge += charge; }
    void AddElectronCount(G4int count) { fTotalElectrons += count; }
    void AddIonCount(G4int count) { fTotalIons += count; }
    
    // New methods for electrometer current modeling
    void AddCurrentPulse(G4double time, G4double current);
    void AddTimeProfile(G4double time, G4double current);
    
    // Methods to get accumulated values
    G4double GetTotalEnergyDeposit() const { return fTotalEnergyDeposit; }
    G4double GetTotalCharge() const { return fTotalCharge; }
    G4int GetTotalElectrons() const { return fTotalElectrons; }
    G4int GetTotalIons() const { return fTotalIons; }
    
    // Method to get the average electrometer current
    G4double GetAverageElectrometerCurrent() const;
    
    // Method to get the peak electrometer current
    G4double GetPeakElectrometerCurrent() const;
    
  private:
    G4double fTotalEnergyDeposit;
    G4double fTotalCharge;
    G4int fTotalElectrons;
    G4int fTotalIons;
    
    // For electrometer modeling
    struct CurrentSample {
        G4double time;
        G4double current;
        
        CurrentSample(G4double t, G4double i) : time(t), current(i) {}
    };
    
    // Store time-current profile
    std::vector<CurrentSample> fCurrentProfile;
    G4double fMaxCurrent;
    G4double fTotalCurrentIntegral;
};

#endif
