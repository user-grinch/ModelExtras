#pragma once

class IFeature {
public:
  // Called on game init, setup stuff goes here
  virtual void Initialize() {}

  // Called once when player enters the vehicle
  virtual void OnEnterVehicle() {};

  // Called once when player exits the vehicle
  virtual void OnExitVehicle() {};

  // This is called every frame for the processing of the feature
  virtual void Process(RwFrame *frame, CVehicle *pVeh) {};

  // Called on game close
  virtual void Shutdown() {};
};