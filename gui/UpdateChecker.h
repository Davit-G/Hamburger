#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

class UpdateChecker : public juce::Thread
{
public:
    UpdateChecker(const juce::String& currentAppVersion);
    ~UpdateChecker() override;

    void checkForUpdates();

    std::function<bool()> shouldCheckForUpdates = [] { return true; };
    std::function<void()> onDisableUpdates = [] {};
    std::function<void()> onCancelUpdates = [] {};

private:
    void run() override;
    void showUpdateDialog(const juce::String& latestVersion, const juce::String& releaseText);
    bool isNewerVersion(const juce::String& latestVersion);

    juce::String currentVersion;
    const juce::String apiEndpoint = "https://hamburger.aviaryaudio.com/releases";
    const juce::String kofiLink = "https://ko-fi.com/s/6e7ded34dc";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UpdateChecker)
};