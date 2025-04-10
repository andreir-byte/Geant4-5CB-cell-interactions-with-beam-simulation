// LCSteppingAction.hh - Enhanced for electrometer current measurement
#ifndef LCSteppingAction_h
#define LCSteppingAction_h 1

#include "G4UserSteppingAction.hh"
#include "globals.hh"
#include <vector>

class LCDetectorConstruction;
class LCEventAction;

class LCSteppingAction : public G4UserSteppingAction {
  public:
    LCSteppingAction(const LCDetectorConstruction* detConstruction, 
                    LCEventAction* eventAction);
    virtual ~LCSteppingAction();
    
    virtual void UserSteppingAction(const G4Step*);
    
  private:
    const LCDetectorConstruction* fDetConstruction;
    LCEventAction* fEventAction;
    
    // Parameters for charge collection simulation
    G4double fElectricField;        // Electric field strength
    G4double fMobilityElectron;     // Electron mobility
    G4double fMobilityIon;          // Ion mobility
    G4double fRecombinationCoef;    // Recombination coefficient
    G4double fCollectionEfficiency; // Charge collection efficiency
    G4double fEnergyPerIonization;  // Energy required per ionization
    
    // For electrometer model
    G4double fElectrometerResistance;  // Internal resistance of electrometer
    G4double fElectrometerCapacitance; // Input capacitance 
    G4double fElectrometerTimeConstant; // RC time constant
    G4double fElectrometerSamplingRate; // Sampling rate in Hz
    
    // Counters
    G4int fTotalElectrons;
    G4int fTotalIons;
    
    // Methods for charge collection calculation
    G4int CalculateIonizationEvents(G4double energyDeposit);
    G4double CalculateCharge(G4int numElectrons);
    G4double CalculateCurrentPulse(G4int numCharges, G4double transitTime);
    
    // Enhanced methods for electrometer simulation
    void SimulateElectrometerResponse(G4double charge, G4double transitTime);
    G4double CalculateElectrometerCurrent(G4double charge, G4double time);
    
    // Structure to hold current pulses for detailed electrometer simulation
    struct CurrentPulse {
        G4double startTime;
        G4double charge;
        G4double duration;
        
        CurrentPulse(G4double t, G4double q, G4double d) 
          : startTime(t), charge(q), duration(d) {}
    };
    
    std::vector<CurrentPulse> fCurrentPulses;
};

#endif
