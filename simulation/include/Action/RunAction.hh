
#ifndef RUNACTION_HH
#define RUNACTION_HH

#include "globals.hh"
#include "G4UserRunAction.hh"

class G4Timer;

class RunAction : public G4UserRunAction
{
public:
    RunAction();
    virtual ~RunAction();

    virtual void BeginOfRunAction(const G4Run *);
    virtual void EndOfRunAction(const G4Run *);

private:
    G4Timer *fTimer;
};

#endif
