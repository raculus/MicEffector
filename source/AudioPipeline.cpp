#include "AudioPipeline.h"

AudioPipeline::AudioPipeline()
{
    formatManager.addFormat(new juce::VST3PluginFormat());

    mainProcessor = std::make_unique<juce::AudioProcessorGraph>();
    audioPlayer.setProcessor(mainProcessor.get());
    deviceManager.addAudioCallback(&audioPlayer);

    audioInputNode = mainProcessor->addNode(
        std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
            juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode));

    audioOutputNode = mainProcessor->addNode(
        std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
            juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));

    loadSettings();
    deviceManager.addChangeListener(this);
}

AudioPipeline::~AudioPipeline()
{
    deviceManager.removeChangeListener(this);
    saveSettings();

    deviceManager.removeAudioCallback(&audioPlayer);
    audioPlayer.setProcessor(nullptr);
    if (mainProcessor != nullptr)
        mainProcessor->clear();
}

void AudioPipeline::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &deviceManager && !isLoadingSettings)
        saveSettings();
}

juce::AudioDeviceManager& AudioPipeline::getDeviceManager()
{
    return deviceManager;
}

bool AudioPipeline::addVst3Plugin(const juce::File& vst3File, bool shouldSave)
{
    juce::OwnedArray<juce::PluginDescription> pluginDescriptions;
    juce::VST3PluginFormat vst3Format;
    vst3Format.findAllTypesForFile(pluginDescriptions, vst3File.getFullPathName());

    if (pluginDescriptions.isEmpty())
        return false;

    juce::String error;
    double sampleRate = deviceManager.getAudioDeviceSetup().sampleRate;
    if (sampleRate <= 0.0)
        sampleRate = 48000.0;

    int bufferSize = deviceManager.getAudioDeviceSetup().bufferSize;
    if (bufferSize <= 0)
        bufferSize = 128;

    std::unique_ptr<juce::AudioPluginInstance> instance =
        formatManager.createPluginInstance(*pluginDescriptions[0], sampleRate, bufferSize, error);

    if (!instance)
        return false;

    auto node = mainProcessor->addNode(std::move(instance));
    if (node == nullptr)
        return false;

    LoadedPlugin item;
    item.file = vst3File;
    item.node = node;
    plugins.push_back(std::move(item));
    rebuildConnections();

    if (shouldSave)
        saveSettings();

    return true;
}

void AudioPipeline::movePluginUp(size_t index)
{
    if (index > 0 && index < plugins.size())
    {
        std::swap(plugins[index], plugins[index - 1]);
        rebuildConnections();
        saveSettings();
    }
}

void AudioPipeline::movePluginDown(size_t index)
{
    if (index + 1 < plugins.size())
    {
        std::swap(plugins[index], plugins[index + 1]);
        rebuildConnections();
        saveSettings();
    }
}

void AudioPipeline::removePlugin(size_t index)
{
    if (index >= plugins.size())
        return;

    plugins[index].editorWindow.reset();
    mainProcessor->removeNode(plugins[index].node->nodeID);
    plugins.erase(plugins.begin() + index);

    rebuildConnections();
    saveSettings();
}

void AudioPipeline::clearPlugins()
{
    for (auto& item : plugins)
    {
        item.editorWindow.reset();
        mainProcessor->removeNode(item.node->nodeID);
    }

    plugins.clear();
    rebuildConnections();
    saveSettings();
}

void AudioPipeline::togglePluginEditor(size_t index)
{
    if (index >= plugins.size())
        return;

    auto& item = plugins[index];
    if (item.editorWindow != nullptr)
    {
        bool show = !item.editorWindow->isVisible();
        item.editorWindow->setVisible(show);
        if (show)
            item.editorWindow->toFront(true);
        return;
    }

    if (auto* processor = item.node->getProcessor())
    {
        if (processor->hasEditor())
        {
            if (auto* editor = processor->createEditorIfNeeded())
            {
                item.editorWindow = std::make_unique<CustomWindow>(
                    processor->getName(),
                    juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId),
                    juce::DocumentWindow::closeButton | juce::DocumentWindow::minimiseButton);

                item.editorWindow->setContentNonOwned(editor, true);
                item.editorWindow->setResizable(true, false);
                item.editorWindow->setUsingNativeTitleBar(true);
                item.editorWindow->centreWithSize(editor->getWidth() + 16, editor->getHeight() + 40);
                item.editorWindow->setVisible(true);
            }
        }
    }
}

void AudioPipeline::setPluginEnabled(size_t index, bool enabled, bool shouldSave)
{
    if (index >= plugins.size() || plugins[index].enabled == enabled)
        return;

    plugins[index].enabled = enabled;
    rebuildConnections();

    if (shouldSave)
        saveSettings();
}

bool AudioPipeline::isPluginEnabled(size_t index) const
{
    return index < plugins.size() && plugins[index].enabled;
}

UiLanguage AudioPipeline::getUiLanguage() const
{
    return uiLanguage;
}

void AudioPipeline::setUiLanguage(UiLanguage language)
{
    if (uiLanguage == language)
        return;

    uiLanguage = language;
    saveSettings();
}

size_t AudioPipeline::getPluginCount() const
{
    return plugins.size();
}

juce::String AudioPipeline::getPluginName(size_t index) const
{
    if (index < plugins.size() && plugins[index].node != nullptr)
    {
        if (auto* processor = plugins[index].node->getProcessor())
            return processor->getName();
    }

    return "Unknown Plugin";
}

void AudioPipeline::rebuildConnections()
{
    if (mainProcessor == nullptr || audioInputNode == nullptr || audioOutputNode == nullptr)
        return;

    for (auto connection : mainProcessor->getConnections())
        mainProcessor->removeConnection(connection);

    std::vector<const LoadedPlugin*> activePlugins;
    for (const auto& plugin : plugins)
        if (plugin.enabled)
            activePlugins.push_back(&plugin);

    auto previousNode = audioInputNode->nodeID;
    for (const auto* plugin : activePlugins)
    {
        for (int channel = 0; channel < 2; ++channel)
            mainProcessor->addConnection({ { previousNode, channel }, { plugin->node->nodeID, channel } });

        previousNode = plugin->node->nodeID;
    }

    for (int channel = 0; channel < 2; ++channel)
        mainProcessor->addConnection({ { previousNode, channel }, { audioOutputNode->nodeID, channel } });
}

void AudioPipeline::saveSettings()
{
    if (isLoadingSettings)
        return;

    juce::File configFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                                .getSiblingFile("config.xml");

    juce::XmlElement xml("MicEffectorConfig");
    xml.setAttribute("language", uiLanguageCode(uiLanguage));
    if (auto state = deviceManager.createStateXml())
    {
        xml.addChildElement(state.release());
    }
    else if (configFile.existsAsFile())
    {
        if (auto oldXml = juce::XmlDocument::parse(configFile))
        {
            if (auto* oldState = oldXml->getChildByName("DEVICESETUP"))
                xml.addChildElement(new juce::XmlElement(*oldState));
        }
    }

    auto* pluginsXml = xml.createNewChildElement("Plugins");
    for (const auto& item : plugins)
    {
        auto* pluginXml = pluginsXml->createNewChildElement("Plugin");
        pluginXml->setAttribute("path", item.file.getFullPathName());
        pluginXml->setAttribute("enabled", item.enabled);
    }

    if (!xml.writeTo(configFile))
        juce::Logger::writeToLog("Failed to write config.xml");
}

void AudioPipeline::loadSettings()
{
    isLoadingSettings = true;

    juce::File configFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                                .getSiblingFile("config.xml");

    std::unique_ptr<juce::XmlElement> xml;
    if (configFile.existsAsFile())
        xml = juce::XmlDocument::parse(configFile);

    if (xml != nullptr && xml->hasTagName("MicEffectorConfig"))
        uiLanguage = uiLanguageFromCode(xml->getStringAttribute("language", "ko"));

    std::unique_ptr<juce::XmlElement> audioState;
    if (xml != nullptr && xml->hasTagName("MicEffectorConfig"))
    {
        if (auto* state = xml->getChildByName("DEVICESETUP"))
            audioState = std::make_unique<juce::XmlElement>(*state);
    }

    auto error = deviceManager.initialise(2, 2, audioState.get(), true);
    if (error.isNotEmpty())
        juce::Logger::writeToLog("Audio init error: " + error);
    else if (auto* device = deviceManager.getCurrentAudioDevice())
        juce::Logger::writeToLog("Audio device opened: " + device->getName()
                                 + " (in=" + deviceManager.getAudioDeviceSetup().inputDeviceName
                                 + ", out=" + deviceManager.getAudioDeviceSetup().outputDeviceName + ")");

    if (xml != nullptr && xml->hasTagName("MicEffectorConfig"))
    {
        if (auto* pluginsXml = xml->getChildByName("Plugins"))
        {
            for (auto* pluginXml : pluginsXml->getChildIterator())
            {
                if (pluginXml->hasTagName("Plugin"))
                {
                    juce::File file(pluginXml->getStringAttribute("path"));
                    if (file.existsAsFile())
                    {
                        auto previousCount = plugins.size();
                        addVst3Plugin(file, false);
                        if (plugins.size() > previousCount)
                            setPluginEnabled(previousCount, pluginXml->getBoolAttribute("enabled", true), false);
                    }
                }
            }
        }
    }

    rebuildConnections();
    isLoadingSettings = false;
}
