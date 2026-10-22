#include "PluginManagerComponent.h"

#include <functional>

namespace
{
enum PluginAction
{
    openEditorAction = 1,
    moveUpAction,
    moveDownAction,
    toggleEnabledAction,
    removePluginAction
};
}

class PluginManagerComponent::PluginRow : public juce::Component
{
public:
    PluginRow(AudioPipeline& pipeline, size_t pluginIndex, UiLanguage language,
              std::function<void(int)> actionHandler)
        : audioPipeline(pipeline), index(pluginIndex), onAction(std::move(actionHandler))
    {
        nameLabel.setText(juce::String(static_cast<int>(index + 1)) + ". " + audioPipeline.getPluginName(index),
                          juce::dontSendNotification);
        nameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(nameLabel);

        addButton(editorButton, localizedText(language, UiText::openPluginEditor), openEditorAction);
        addButton(upButton, localizedText(language, UiText::moveUp), moveUpAction);
        addButton(downButton, localizedText(language, UiText::moveDown), moveDownAction);
        addButton(enabledButton,
              localizedText(language, audioPipeline.isPluginEnabled(index)
                              ? UiText::disablePlugin : UiText::enablePlugin),
              toggleEnabledAction);
        addButton(deleteButton, localizedText(language, UiText::removePlugin), removePluginAction);

        upButton.setEnabled(index > 0);
        downButton.setEnabled(index + 1 < audioPipeline.getPluginCount());
    }

    void paint(juce::Graphics& graphics) override
    {
        graphics.fillAll(juce::Colour(0xff292d32));
        graphics.setColour(juce::Colour(0xff41474e));
        graphics.drawHorizontalLine(getHeight() - 1, 0.0f, static_cast<float>(getWidth()));
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(10, 6);
        nameLabel.setBounds(area.removeFromLeft(juce::jmax(100, area.getWidth() - 416)));
        area.removeFromLeft(8);
        editorButton.setBounds(area.removeFromLeft(72));
        area.removeFromLeft(6);
        upButton.setBounds(area.removeFromLeft(74));
        area.removeFromLeft(6);
        downButton.setBounds(area.removeFromLeft(88));
        area.removeFromLeft(6);
        enabledButton.setBounds(area.removeFromLeft(78));
        area.removeFromLeft(6);
        deleteButton.setBounds(area.removeFromLeft(72));
    }

private:
    void addButton(juce::TextButton& button, const juce::String& text, int action)
    {
        button.setButtonText(text);
        button.onClick = [this, action] { onAction(action); };
        addAndMakeVisible(button);
    }

    AudioPipeline& audioPipeline;
    size_t index;
    std::function<void(int)> onAction;
    juce::Label nameLabel;
    juce::TextButton editorButton;
    juce::TextButton upButton;
    juce::TextButton downButton;
    juce::TextButton enabledButton;
    juce::TextButton deleteButton;
};

PluginManagerComponent::PluginManagerComponent(AudioPipeline& pipeline,
                                               std::function<void()> openSettings,
                                               std::function<void()> onLanguageChanged)
    : audioPipeline(pipeline),
      openAudioSettings(std::move(openSettings)),
      languageChanged(std::move(onLanguageChanged))
{
    addPluginButton.onClick = [this] { addPlugin(); };
    addAndMakeVisible(addPluginButton);

    audioSettingsButton.onClick = [this]
    {
        if (openAudioSettings)
            openAudioSettings();
    };
    addAndMakeVisible(audioSettingsButton);

    languageSelector.addItem(juce::String(L"한국어"), 1);
    languageSelector.addItem("English", 2);
    languageSelector.onChange = [this]
    {
        audioPipeline.setUiLanguage(languageSelector.getSelectedId() == 2
                                        ? UiLanguage::english : UiLanguage::korean);
        applyLanguage();
        if (languageChanged)
            languageChanged();
    };
    addAndMakeVisible(languageLabel);
    addAndMakeVisible(languageSelector);

    pluginViewport.setViewedComponent(&pluginList, false);
    pluginViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(pluginViewport);

    applyLanguage();
    refreshList();
    setSize(760, 500);
}

PluginManagerComponent::~PluginManagerComponent() = default;

void PluginManagerComponent::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colour(0xff202428));
}

void PluginManagerComponent::resized()
{
    auto area = getLocalBounds().reduced(16);
    auto toolbar = area.removeFromTop(36);
    addPluginButton.setBounds(toolbar.removeFromLeft(150));
    audioSettingsButton.setBounds(toolbar.removeFromRight(132));
    toolbar.removeFromRight(10);
    languageSelector.setBounds(toolbar.removeFromRight(112));
    toolbar.removeFromRight(8);
    languageLabel.setBounds(toolbar.removeFromRight(64));
    area.removeFromTop(12);
    pluginViewport.setBounds(area);
    auto contentHeight = juce::jmax(1, static_cast<int>(pluginRows.size()) * 56);
    pluginList.setSize(pluginViewport.getWidth(), contentHeight);
    for (size_t index = 0; index < pluginRows.size(); ++index)
        pluginRows[index]->setBounds(0, static_cast<int>(index) * 56, pluginViewport.getWidth(), 56);
}

void PluginManagerComponent::applyLanguage()
{
    auto language = audioPipeline.getUiLanguage();
    addPluginButton.setButtonText(localizedText(language, UiText::addPlugin));
    audioSettingsButton.setButtonText(localizedText(language, UiText::audioSettingsButton));
    languageLabel.setText(localizedText(language, UiText::languageLabel), juce::dontSendNotification);
    languageLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    languageSelector.setSelectedId(language == UiLanguage::english ? 2 : 1,
                                   juce::dontSendNotification);
    refreshList();
}

void PluginManagerComponent::addPlugin()
{
    auto vst3Directory = juce::File::getSpecialLocation(juce::File::globalApplicationsDirectory)
                            .getChildFile("Common Files/VST3");
    auto chooser = std::make_shared<juce::FileChooser>(
        localizedText(audioPipeline.getUiLanguage(), UiText::chooseVst3Plugin), vst3Directory, "*.vst3");
    auto safeThis = juce::Component::SafePointer<PluginManagerComponent>(this);

    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [chooser, safeThis](const juce::FileChooser& fileChooser)
        {
            auto file = fileChooser.getResult();
            if (safeThis != nullptr && file.exists() && safeThis->audioPipeline.addVst3Plugin(file, true))
                safeThis->refreshList();
        });
}

void PluginManagerComponent::refreshList()
{
    pluginList.removeAllChildren();
    pluginRows.clear();

    auto pluginCount = audioPipeline.getPluginCount();
    for (size_t index = 0; index < pluginCount; ++index)
    {
        auto row = std::make_unique<PluginRow>(audioPipeline, index, audioPipeline.getUiLanguage(),
            [this, index](int action) { handlePluginAction(index, action); });
        pluginList.addAndMakeVisible(row.get());
        pluginRows.push_back(std::move(row));
    }

    auto contentHeight = juce::jmax(1, static_cast<int>(pluginCount) * 56);
    pluginList.setSize(pluginViewport.getWidth(), contentHeight);
    for (size_t index = 0; index < pluginRows.size(); ++index)
        pluginRows[index]->setBounds(0, static_cast<int>(index) * 56, pluginViewport.getWidth(), 56);
}

void PluginManagerComponent::handlePluginAction(size_t index, int action)
{
    switch (action)
    {
    case openEditorAction:
        audioPipeline.togglePluginEditor(index);
        return;
    case moveUpAction:
        audioPipeline.movePluginUp(index);
        break;
    case moveDownAction:
        audioPipeline.movePluginDown(index);
        break;
    case toggleEnabledAction:
        audioPipeline.setPluginEnabled(index, !audioPipeline.isPluginEnabled(index));
        break;
    case removePluginAction:
        audioPipeline.removePlugin(index);
        break;
    default:
        return;
    }

    auto safeThis = juce::Component::SafePointer<PluginManagerComponent>(this);
    juce::MessageManager::callAsync([safeThis]
    {
        if (safeThis != nullptr)
            safeThis->refreshList();
    });
}
