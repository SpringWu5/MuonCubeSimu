#ifndef SteppingAction_h
#define SteppingAction_h 1

#include "G4UserSteppingAction.hh"
#include "globals.hh"
#include <map>
#include <string>

class EventAction;

class SteppingAction : public G4UserSteppingAction
{
  public:
    SteppingAction(EventAction* eventAction = nullptr);
    virtual ~SteppingAction();
    
    virtual void UserSteppingAction(const G4Step*);
    
    // �������ͳ����Ϣ
    void OutputPhotonStatistics(G4int eventID, G4int cerCount, G4int scintCount);
    
    // �������ͳ����Ϣ
    void OutputFinalStatistics();
    
    // Get particle statistics
    const std::map<G4String, G4int>& GetParticleCount() const { return particleCount; }
    
  private:
    EventAction* fEventAction;
    
    // ������
    G4int cerenkovCount;
    G4int scintillationCount;
    G4int currentEventID;
    
    // Particle counting map
    std::map<G4String, G4int> particleCount;
    
    // Secondary particle PDG codes (for the current event)
    std::set<G4int> secondaryParticlePDGCodes;
};

#endif