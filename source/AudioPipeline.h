#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>

#include "CustomWindow.h"
#include "Localization.h"

struct LoadedPlugin
{
    juce::File file;
    juce::AudioProcessorGraph::Node::Ptr node;
    std::unique_ptr<CustomWindow> editorWindow;
    bool enabled = true;
};

class AudioPipeline : public juce::ChangeListener
{
public:
    AudioPipeline();
    ~AudioPipeline() override;

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    juce::AudioDeviceManager& getDeviceManager();
    bool addVst3Plugin(const juce::File& vst3File, bool shouldSave = true);
    void movePluginUp(size_t index);
    void movePluginDown(size_t index);
    void removePlugin(size_t index);
    void clearPlugins();
    void togglePluginEditor(size_t index);
    void setPluginEnabled(size_t index, bool enabled, bool shouldSave = true);
    bool isPluginEnabled(size_t index) const;
    UiLanguage getUiLanguage() const;
    void setUiLanguage(UiLanguage language);
    size_t getPluginCount() const;
    juce::String getPluginName(size_t index) const;
    void rebuildConnections();
    void saveSettings();
    void loadSettings();

private:
    juce::AudioDeviceManager deviceManager;
    juce::AudioProcessorPlayer audioPlayer;
    juce::AudioPluginFormatManager formatManager;
    std::unique_ptr<juce::AudioProcessorGraph> mainProcessor;

    juce::AudioProcessorGraph::Node::Ptr audioInputNode;
    juce::AudioProcessorGraph::Node::Ptr audioOutputNode;

    std::vector<LoadedPlugin> plugins;
    UiLanguage uiLanguage = UiLanguage::korean;
    bool isLoadingSettings = false;
};
