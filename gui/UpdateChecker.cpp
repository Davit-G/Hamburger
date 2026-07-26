#include "UpdateChecker.h"
#include "BurgerAlert.h" 

namespace
{
    juce::String stripHtmlTags(const juce::String& input)
    {
        juce::String output;
        bool inTag = false;
        
        for (auto it = input.getCharPointer(); !it.isEmpty(); ++it)
        {
            auto c = *it;
            if (c == '<') 
            {
                inTag = true;
            }
            else if (c == '>') 
            {
                inTag = false;
            }
            else if (!inTag) 
            {
                output += c;
            }
        }
        
        return output.trim().replace("\r\n\r\n\r\n", "\r\n\r\n").replace("\n\n\n", "\n\n");
    }
}

UpdateChecker::UpdateChecker(const juce::String& currentAppVersion)
    : juce::Thread("HamburgerUpdateCheckerThread"),
      currentVersion(currentAppVersion)
{
}

UpdateChecker::~UpdateChecker()
{
    stopThread(4000);
}

void UpdateChecker::checkForUpdates()
{
    if (shouldCheckForUpdates && !shouldCheckForUpdates())
        return;

    if (!isThreadRunning())
        startThread();
}

void UpdateChecker::run()
{
    juce::URL url(apiEndpoint);
    juce::String userAgent = "User-Agent: " + juce::String(JucePlugin_Name) + "/" + juce::String(JucePlugin_VersionString);
    
    auto inputStream = url.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                                 .withConnectionTimeoutMs(3000)
                                                 .withExtraHeaders(userAgent)
                                                 .withNumRedirectsToFollow(5));

    if (inputStream != nullptr)
    {
        juce::String jsonString = inputStream->readEntireStreamAsString();
        juce::var parsedJson = juce::JSON::parse(jsonString);

        if (parsedJson.isArray() && parsedJson.getArray()->size() > 0)
        {
            auto latestRelease = parsedJson.getArray()->getReference(0);
            
            if (latestRelease.hasProperty("tag_name"))
            {
                juce::String latestTag = latestRelease["tag_name"].toString();
                
                if (isNewerVersion(latestTag))
                {
                    juce::String releaseName = latestRelease.getProperty("name", "").toString();
                    juce::String releaseBody = latestRelease.getProperty("body", "").toString();
                    
                    releaseBody = stripHtmlTags(releaseBody);
                    
                    juce::String releaseText = releaseName;
                    if (releaseBody.isNotEmpty())
                        releaseText += "\n\n" + releaseBody;

                    auto disableCb = onDisableUpdates;
                    auto cancelCb = onCancelUpdates;
                    auto kofi = kofiLink;

                    juce::MessageManager::callAsync([this, latestTag, releaseText, disableCb, cancelCb, kofi]() 
                    {
                        showUpdateDialog(latestTag, releaseText);
                    });
                }
            }
        }
    }
}

void UpdateChecker::showUpdateDialog(const juce::String& latestVersion, const juce::String& releaseText)
{
    juce::String prompt = "Hamburger " + latestVersion + " is available!\n\nWould you like to download it now?";
    auto* alert = new BurgerAlert("Update Available", prompt, juce::AlertWindow::InfoIcon);

    alert->createUpdateAlert(releaseText);
    alert->setAlwaysOnTop(true); 

    alert->enterModalState(false, juce::ModalCallbackFunction::create([alert, disableCb = this->onDisableUpdates, cancelCb = this->onCancelUpdates, kofi = this->kofiLink](int result) 
    {
        if (result == 1) // Update
        {
            juce::URL(kofi).launchInDefaultBrowser();
        }
        else if (result == 2) // Don't show again
        {
            if (disableCb)
                disableCb();
        }
        else if (result == 0) // Cancel
        {
            if (cancelCb)
                cancelCb();
        }

        delete alert;
    }));
}

bool UpdateChecker::isNewerVersion(const juce::String& latestVersion)
{
    auto cleanLatest = latestVersion.removeCharacters("vV");
    auto cleanCurrent = currentVersion.removeCharacters("vV");

    juce::StringArray latestParts;
    latestParts.addTokens(cleanLatest, ".", "");
    
    juce::StringArray currentParts;
    currentParts.addTokens(cleanCurrent, ".", "");

    const int numParts = juce::jmax(latestParts.size(), currentParts.size());

    for (int i = 0; i < numParts; ++i)
    {
        int l = latestParts[i].getIntValue();
        int c = currentParts[i].getIntValue();

        if (l > c) return true;
        if (l < c) return false;
    }
    
    return false;
}