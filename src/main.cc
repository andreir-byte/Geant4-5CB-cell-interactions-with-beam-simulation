// main.cc - Multi-threaded with memory optimizations and safe exit
// VISUALIZATION COMPLETELY REMOVED
#include "LCDetectorConstruction.hh"
#include "LCPhysicsList.hh"
#include "LCActionInitialization.hh"
#include "LCGlobalManager.hh"

// Use multi-threaded run manager if available
#ifdef G4MULTITHREADED
#include "G4MTRunManager.hh"
#else
#include "G4RunManager.hh"
#endif

#include "G4UImanager.hh"
#include "Randomize.hh"
#include "G4SystemOfUnits.hh"
#include <chrono>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <stdlib.h> // For exit()

#include "G4AnalysisManager.hh"
#include <chrono>
#include <thread>

int main(int argc, char** argv)
{
  // Record start time for timing measurement
  auto start = std::chrono::high_resolution_clock::now();

  // Default beam parameters
  G4String particleType = "proton";
  G4double particleEnergy = 0.5*GeV;
  G4String macroFile = "";
  
  // Simple command line argument handling
  for (int i = 1; i < argc; i++) {
    G4String arg = argv[i];
    
    if (arg == "--particle" && i+1 < argc) {
      particleType = argv[++i];
    }
    else if (arg == "--energy" && i+1 < argc) {
      G4String energyStr = argv[++i];
      std::istringstream iss(energyStr);
      G4double value;
      G4String unit;
      iss >> value >> unit;
      
      if (unit == "MeV") particleEnergy = value * MeV;
      else if (unit == "GeV") particleEnergy = value * GeV;
      else if (unit == "keV") particleEnergy = value * keV;
      else if (unit == "eV") particleEnergy = value * eV;
      else particleEnergy = std::stod(energyStr) * MeV; // Default to MeV
    }
    else if (arg == "--help") {
      G4cout << "Usage: " << argv[0] << " [options] [macro]" << G4endl;
      G4cout << "Options:" << G4endl;
      G4cout << "  --particle TYPE    Set particle type (proton, e-, gamma, etc.)" << G4endl;
      G4cout << "  --energy VALUE     Set particle energy (with unit: 10 MeV, 1 GeV, etc.)" << G4endl;
      G4cout << "  --help             Show this help message" << G4endl;
      return 0;
    }
    else if (macroFile.empty() && arg[0] != '-') {
      // If not recognized as an option and starts with non-dash, treat as macro file
      macroFile = arg;
    }
  }

  // Check if macro file exists before proceeding
  if (!macroFile.empty()) {
    std::ifstream testFile(macroFile.c_str());
    if (!testFile.good()) {
      G4cerr << "Error: Macro file '" << macroFile << "' not found!" << G4endl;
      return 1;
    }
    testFile.close();
  }

  // Set random seed with better entropy source
  G4Random::setTheEngine(new CLHEP::RanecuEngine);
  unsigned long seed = static_cast<unsigned long>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
  G4Random::setTheSeed(seed);

  // Construct the run manager
  G4RunManager* runManager = nullptr;
  
  try {
    // Initialize global manager with command line parameters before creating run manager
    LCGlobalManager::Instance()->SetParticleType(particleType);
    LCGlobalManager::Instance()->SetParticleEnergy(particleEnergy);

    // Create appropriate run manager
    #ifdef G4MULTITHREADED
      G4cout << "Using multi-threaded run manager (optimized)" << G4endl;
      auto* mtRunManager = new G4MTRunManager();
      runManager = mtRunManager;
      
      // Use 12 threads as requested
      G4int nThreads = 12;
      mtRunManager->SetNumberOfThreads(nThreads);
      G4cout << "Number of threads: " << nThreads << G4endl;
    #else
      G4cout << "Using single-threaded run manager" << G4endl;
      runManager = new G4RunManager();
    #endif

    // Print banner with actual settings that will be used
    G4cout << "===================================================" << G4endl;
    G4cout << "    5CB Liquid Crystal Electrical Detector" << G4endl;
    G4cout << "    15mm × 25mm × 100μm" << G4endl;
    G4cout << "    Electrometer Current Measurement Enabled" << G4endl;
    G4cout << "    Memory Optimized Build" << G4endl;
    G4cout << "    Particle: " << particleType << G4endl;
    G4cout << "    Energy: " << particleEnergy/MeV << " MeV" << G4endl;
    G4cout << "    VISUALIZATION DISABLED" << G4endl;
    G4cout << "===================================================" << G4endl;
    
    // Set mandatory initialization classes
    auto detConstruction = new LCDetectorConstruction();
    runManager->SetUserInitialization(detConstruction);
    runManager->SetUserInitialization(new LCPhysicsList());
    
    // IMPORTANT: Set beam parameters BEFORE initializing
    // This ensures correct energy is used for filename
    auto actionInit = new LCActionInitialization(detConstruction);
    actionInit->SetBeamParticle(particleType);
    actionInit->SetBeamEnergy(particleEnergy);
    runManager->SetUserInitialization(actionInit);
    
    // Initialize G4 kernel
    runManager->Initialize();

    // Get UI manager
    auto UImanager = G4UImanager::GetUIpointer();

    // Execute run commands
    if (!macroFile.empty()) {
      // Execute macro file if provided
      G4String command = "/control/execute ";
      G4int status = UImanager->ApplyCommand(command + macroFile);
      if (status != 0) {
        G4cerr << "Error executing macro file: " << macroFile << G4endl;
        G4cerr << "Status code: " << status << G4endl;
      }
    } else {
      // If no macro provided, run a default but smaller number of events
      G4cout << "No macro file provided. Running default 10 events." << G4endl;
      UImanager->ApplyCommand("/run/beamOn 10");
    }

    // Print timing information
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    G4cout << "Total execution time: " << duration.count() << " seconds" << G4endl;
    
    // Cleanly exit without cleanup that causes segfaults
G4cout << "Simulation completed successfully. Finalizing output..." << G4endl;

// Force analysis manager to write and close files
auto analysisManager = G4AnalysisManager::Instance();
if (analysisManager && analysisManager->IsOpenFile()) {
    G4cout << "Writing and closing output files..." << G4endl;
    analysisManager->Write();
    analysisManager->CloseFile();
}

// Add a small delay to ensure file operations complete
std::this_thread::sleep_for(std::chrono::milliseconds(500));

G4cout << "Exiting..." << G4endl;
    exit(0);  // Skip normal cleanup and just exit
    
  } catch (const std::exception& e) {
    G4cerr << "Exception caught: " << e.what() << G4endl;
    return 1;
  } catch (...) {
    G4cerr << "Unknown exception caught!" << G4endl;
    return 1;
  }
}
