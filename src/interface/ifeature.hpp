#pragma once

enum class eFeatureState {
    Initialized,
    EnteredVehicle,
    ExitedVehicle,
    Process,
    Shutdown,
    NotInitialized,
};

class IFeature {
protected:
    eFeatureState m_State = eFeatureState::NotInitialized;

public:
    // Called on game init, setup stuff goes here
    virtual void Initialize() {
        m_State = eFeatureState::Initialized;
    }

    // Called once when player enters the vehicle
    virtual void OnEnterVehicle() {
        m_State = eFeatureState::EnteredVehicle;
    };

    // Called once when player exits the vehicle
    virtual void OnExitVehicle() {
        m_State = eFeatureState::ExitedVehicle;
    };

    // This is called every frame for the processing of the feature
    virtual void Process(RwFrame * frame, CVehicle* pVeh) {
        m_State = eFeatureState::Process;
    };

    // Called on game close
    virtual void Shutdown() {
        m_State = eFeatureState::Shutdown;
    };
}; 