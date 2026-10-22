#pragma once

#include <JuceHeader.h>
#include <functional>
#include <memory>
#include <vector>

#include "AudioPipeline.h"

class PluginManagerComponent : public juce::Component
{
public:
    PluginManagerComponent(AudioPipeline& pipeline,
                           std::function<void()> openAudioSettings,
                           std::function<void()> languageChanged);
    ~PluginManagerComponent() override;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    class PluginRow;

    void addPlugin();
    void applyLanguage();
    void refreshList();
    void handlePluginAction(size_t index, int action);

    AudioPipeline& audioPipeline;
    std::function<void()> openAudioSettings;
    std::function<void()> languageChanged;
    juce::TextButton addPluginButton;
    juce::TextButton audioSettingsButton;
    juce::Label languageLabel;
    juce::ComboBox languageSelector;
    juce::Viewport pluginViewport;
    juce::Component pluginList;
    std::vector<std::unique_ptr<PluginRow>> pluginRows;
};
