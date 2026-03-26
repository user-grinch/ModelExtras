#include <Urlmon.h>

#include "ghtracker.h"

#include <fstream>
#include <nlohmann/json.hpp>

GHTracker::GHTracker(const std::string &userName, const std::string &repoName, const std::string &currentVer)
    : localVer(currentVer)
{
    link = "https://api.github.com/repos/" + userName + "/" + repoName + "/tags";
}

void GHTracker::CheckUpdate(bool showMSG)
{
    curState = eStates::Checking;
    showMessage = showMSG;
}

std::string GHTracker::GetUpdateVersion() const
{
    return latestVer;
}

bool GHTracker::IsUpdateAvailable() const
{
    return curState == eStates::Found;
}

void GHTracker::ResetUpdaterState()
{
    curState = eStates::Idle;
}

static bool IsVersionNewer(const std::string &latest, const std::string &current)
{
    return latest > current;
}

void GHTracker::Process(std::function<void(eStates, bool)> eventCallback, const std::string &path, bool lockDowngrade)
{
    if (curState != eStates::Checking)
        return;

    curState = eStates::Failed;

    if (URLDownloadToFile(nullptr, link.c_str(), path.c_str(), 0, nullptr) != S_OK)
    {
        eventCallback(curState, showMessage);
        return;
    }

    std::ifstream file(path);
    if (!file.is_open())
    {
        eventCallback(curState, showMessage);
        return;
    }

    nlohmann::json data;
    try
    {
        data = nlohmann::json::parse(file);
    }
    catch (...)
    {
        file.close();
        eventCallback(curState, showMessage);
        return;
    }
    file.close();

    if (data.empty() || !data[0].contains("name"))
    {
        curState = eStates::NotFound;
    }
    else
    {
        latestVer = data[0]["name"].get<std::string>();
        bool isNewer = IsVersionNewer(latestVer, localVer);

        curState = (isNewer || !lockDowngrade) ? eStates::Found : eStates::NotFound;
    }

    eventCallback(curState, showMessage);
}
