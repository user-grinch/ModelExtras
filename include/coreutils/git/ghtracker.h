#pragma once
#include <functional>
#include <string>

enum class eStates
{
    Idle,
    Checking,
    Found,
    NotFound,
    Failed,
};

class GHTracker
{
  private:
    eStates curState = eStates::Idle;
    std::string latestVer;
    std::string link;
    std::string localVer;
    bool showMessage = false;

  public:
    GHTracker(const std::string &userName, const std::string &repoName, const std::string &currentVer);

    void CheckUpdate(bool showMSG = false);
    void Process(std::function<void(eStates, bool)> eventCallback, const std::string &path, bool lockDowngrade = true);

    [[nodiscard]] std::string GetUpdateVersion() const;
    [[nodiscard]] bool IsUpdateAvailable() const;
    void ResetUpdaterState();
};
