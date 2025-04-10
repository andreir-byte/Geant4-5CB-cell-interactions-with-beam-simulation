// LCRunAction.cc - Fixed segmentation fault issue

#include "LCRunAction.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4AnalysisManager.hh"
#include "LCGlobalManager.hh"
#include <fstream>
#include <iomanip>
#include <exception>
#include <thread>
#include <chrono>
#include <mutex>

std::mutex filenameMutex;

LCRunAction::LCRunAction()
: G4UserRunAction(),
  fParticleName("proton"),
  fParticleEnergy(15*GeV),
  fFilenameGenerated(false),
  fCurrentFileName("")
{
  // Create analysis manager
  auto analysisManager = G4AnalysisManager::Instance();
  analysisManager->SetVerboseLevel(1);
  analysisManager->SetNtupleMerging(true);
  
  // Use a temporary filename - will be updated with correct energy in BeginOfRunAction
  analysisManager->SetFileName("LC_temp.root");
  
  // Also set up CSV output for easier Linux processing
  analysisManager->SetActivation(true);
}

LCRunAction::~LCRunAction()
{
  // Don't do any cleanup here - it's handled in EndOfRunAction
}

void LCRunAction::BeginOfRunAction(const G4Run* run)
{
  // Print run information
  G4cout << "### Run " << run->GetRunID() << " start." << G4endl;
  
  // Inform the runManager to save random number seed
  G4RunManager::GetRunManager()->SetRandomNumberStore(false);
  
  try {
    // Lock mutex to ensure thread-safe access to global manager and file creation
    std::lock_guard<std::mutex> lock(filenameMutex);
    
    // Get analysis manager
    auto analysisManager = G4AnalysisManager::Instance();
    
    // Get latest settings from global manager
    G4String particleName = LCGlobalManager::Instance()->GetParticleType();
    G4double particleEnergy = LCGlobalManager::Instance()->GetParticleEnergy();
    
    // Set local values to match global
    fParticleName = particleName;
    fParticleEnergy = particleEnergy;
    
    // Generate filename using CURRENT energy values
    G4String baseFileName = "LC_" + fParticleName + "_" 
                    + G4UIcommand::ConvertToString(fParticleEnergy/MeV) + "MeV";
    
    // Full filename with extension
    G4String fullFileName = baseFileName + ".root";
    fCurrentFileName = baseFileName; // Store current filename base
    
    // Print clear information about the actual beam energy being used
    G4cout << "\n==== SIMULATION FILE OUTPUT DETAILS ====" << G4endl;
    G4cout << "Particle type: " << fParticleName << G4endl;
    G4cout << "Particle energy: " << fParticleEnergy/MeV << " MeV" << G4endl;
    G4cout << "Output ROOT file: " << fullFileName << G4endl;
    G4cout << "========================================\n" << G4endl;
    
    // Set and open the file
    analysisManager->SetFileName(fullFileName);
    analysisManager->OpenFile();
    
    // Create ntuple 
    analysisManager->CreateNtuple("LCData", "Liquid Crystal Detector Data");
    analysisManager->CreateNtupleDColumn("Edep");         // keV
    analysisManager->CreateNtupleDColumn("Charge");       // pC
    analysisManager->CreateNtupleIColumn("ElectronCount"); // number
    analysisManager->CreateNtupleIColumn("IonCount");     // number
    analysisManager->CreateNtupleDColumn("AvgCurrent");   // pA
    analysisManager->CreateNtupleDColumn("PeakCurrent");  // pA
    analysisManager->CreateNtupleDColumn("FinalTime");    // ns
    analysisManager->CreateNtupleDColumn("FinalCurrent"); // pA
    analysisManager->FinishNtuple();
    
    // Also create a text file for electrometer readings
    std::string electroFile = baseFileName + "_electrometer.dat";
    std::ofstream outFile(electroFile);
    if (!outFile.is_open()) {
      G4cerr << "Warning: Could not open electrometer data file: " << electroFile << G4endl;
      G4cerr << "Continuing without electrometer data output..." << G4endl;
    } else {
      outFile << "# 5CB Liquid Crystal Detector with Electrometer\n";
      outFile << "# Particle: " << fParticleName << "\n";
      outFile << "# Energy: " << fParticleEnergy/MeV << " MeV\n";
      outFile << "# \n";
      outFile << "# Column 1: Event ID\n";
      outFile << "# Column 2: Energy Deposit (keV)\n";
      outFile << "# Column 3: Charge (pC)\n";
      outFile << "# Column 4: Average Current (pA)\n";
      outFile << "# Column 5: Peak Current (pA)\n";
      outFile << "# \n";
      outFile.close();
    }
    
    // Set flag that filename has been generated
    fFilenameGenerated = true;
    
  } catch (const std::exception& e) {
    G4cerr << "Analysis Error in BeginOfRunAction: " << e.what() << G4endl;
    G4cerr << "Continuing without analysis output..." << G4endl;
  } catch (...) {
    G4cerr << "Unknown error in BeginOfRunAction" << G4endl;
    G4cerr << "Continuing without analysis output..." << G4endl;
  }
}

void LCRunAction::EndOfRunAction(const G4Run* run)
{
  G4int nofEvents = run->GetNumberOfEvent();
  if (nofEvents == 0) return;
  
  // Print run summary
  G4cout << "### Run " << run->GetRunID() << " ended. Number of events: " << nofEvents << G4endl;
  
  try {
    // Try-catch everything to avoid segfaults
    try {
      // Get analysis manager
      auto analysisManager = G4AnalysisManager::Instance();
      
      // Save histograms and ntuple
      if (analysisManager && analysisManager->IsOpenFile()) {
        analysisManager->Write();
        analysisManager->CloseFile();
      }
      
      // Generate additional electrometer report
      G4String reportFile = fCurrentFileName + "_electrometer_report.txt";
      
      std::ofstream report(reportFile);
      if (!report.is_open()) {
        G4cerr << "Warning: Could not open report file: " << reportFile << G4endl;
      } else {
        report << "=================================================\n";
        report << "    5CB LIQUID CRYSTAL DETECTOR REPORT\n";
        report << "=================================================\n";
        report << "Particle type: " << fParticleName << "\n";
        report << "Particle energy: " << fParticleEnergy/MeV << " MeV\n";
        report << "Number of events: " << nofEvents << "\n";
        report << "-------------------------------------------------\n";
        report << "Electrometer measurements:\n";
        report << "  Detailed data available in: " << fCurrentFileName << ".root\n";
        report << "  CSV data available in: " << fCurrentFileName << ".csv\n";
        report << "-------------------------------------------------\n";
        
        report << "Notes: This simulation includes explicit modeling of\n";
        report << "electrometer connected to both sides of the 5CB cell.\n";
        report << "=================================================\n";
        report.close();
        
        // Print analysis summary
        G4cout << "Analysis results saved to file: " << fCurrentFileName << ".root" << G4endl;
        G4cout << "Electrometer report saved to: " << reportFile << G4endl;
      }
      
      // Only clear data if analysis manager exists and is valid
      if (analysisManager) {
        try {
          analysisManager->Clear();
          G4cout << "... clear all data - done" << G4endl;
        } catch (...) {
          G4cerr << "Error during Clear() operation - ignoring" << G4endl;
        }
      }
    
    } catch (const std::exception& e) {
      G4cerr << "Analysis Error in EndOfRunAction: " << e.what() << G4endl;
    } catch (...) {
      G4cerr << "Unknown error in EndOfRunAction" << G4endl;
    }
  
  } catch (...) {
    // Final catch-all to prevent segfaults during cleanup
    G4cerr << "Error during cleanup - continuing safely" << G4endl;
  }
  
  // Add a pause to ensure all threads have finished writing
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

// Add special handling for SetParticleEnergy
void LCRunAction::SetParticleEnergy(G4double energy)
{
  // Update energy value
  fParticleEnergy = energy;
  
  // If filename has already been generated, warn user about mismatch
  if (fFilenameGenerated) {
    G4cout << "WARNING: Energy changed to " << energy/MeV << " MeV after run start." << G4endl;
    G4cout << "         File is already named: " << fCurrentFileName << ".root" << G4endl;
    G4cout << "         For consistent naming, change energy before starting the run." << G4endl;
  }
}
