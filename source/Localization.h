#pragma once

#include <JuceHeader.h>

enum class UiLanguage
{
    korean,
    english
};

enum class UiText
{
    managerTitle,
    audioSettingsTitle,
    addPlugin,
    openPluginEditor,
    moveUp,
    moveDown,
    disablePlugin,
    enablePlugin,
    removePlugin,
    chooseVst3Plugin,
    audioSettingsButton,
    languageLabel,
    trayOpen,
    audioSettingsMenu,
    addVst3Menu,
    clearPluginsMenu,
    exitMenu
};

UiLanguage uiLanguageFromCode(const juce::String& code);
juce::String uiLanguageCode(UiLanguage language);
juce::String localizedText(UiLanguage language, UiText text);
