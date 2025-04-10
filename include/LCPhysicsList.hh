// LCPhysicsList.hh
#ifndef LCPhysicsList_h
#define LCPhysicsList_h 1

#include "G4VModularPhysicsList.hh"
#include "globals.hh"

class LCPhysicsList : public G4VModularPhysicsList
{
  public:
    LCPhysicsList();
    virtual ~LCPhysicsList();
    
    virtual void SetCuts();
};

#endif
