#include "Localization.h"

UiLanguage uiLanguageFromCode(const juce::String& code)
{
    return code == "en" ? UiLanguage::english : UiLanguage::korean;
}

juce::String uiLanguageCode(UiLanguage language)
{
    return language == UiLanguage::english ? "en" : "ko";
}

juce::String localizedText(UiLanguage language, UiText text)
{
    if (language == UiLanguage::english)
    {
        switch (text)
        {
        case UiText::managerTitle: return juce::String(L"MicEffector - Plugin Manager");
        case UiText::audioSettingsTitle: return juce::String(L"Audio Device Settings");
        case UiText::addPlugin: return juce::String(L"Add Plugin");
        case UiText::openPluginEditor: return juce::String(L"Open UI");
        case UiText::moveUp: return juce::String(L"Move Up");
        case UiText::moveDown: return juce::String(L"Move Down");
        case UiText::disablePlugin: return juce::String(L"Disable");
        case UiText::enablePlugin: return juce::String(L"Enable");
        case UiText::removePlugin: return juce::String(L"Remove");
        case UiText::chooseVst3Plugin: return juce::String(L"Select VST3 Plugin");
        case UiText::audioSettingsButton: return juce::String(L"Audio Settings");
        case UiText::languageLabel: return juce::String(L"Language");
        case UiText::trayOpen: return juce::String(L"Open");
        case UiText::audioSettingsMenu: return juce::String(L"Audio Device Settings (&S)");
        case UiText::addVst3Menu: return juce::String(L"Add VST3 Plugin (&A)...");
        case UiText::clearPluginsMenu: return juce::String(L"Clear All Plugins (&C)");
        case UiText::exitMenu: return juce::String(L"Exit (&X)");
        }
    }

    switch (text)
    {
    case UiText::managerTitle: return juce::String(L"MicEffector - 플러그인 관리");
    case UiText::audioSettingsTitle: return juce::String(L"오디오 장치 및 지연 시간 설정");
    case UiText::addPlugin: return juce::String(L"플러그인 추가");
    case UiText::openPluginEditor: return juce::String(L"UI 열기");
    case UiText::moveUp: return juce::String(L"위로");
    case UiText::moveDown: return juce::String(L"아래로");
    case UiText::disablePlugin: return juce::String(L"끄기");
    case UiText::enablePlugin: return juce::String(L"켜기");
    case UiText::removePlugin: return juce::String(L"삭제");
    case UiText::chooseVst3Plugin: return juce::String(L"VST3 플러그인 선택");
    case UiText::audioSettingsButton: return juce::String(L"오디오 설정");
    case UiText::languageLabel: return juce::String(L"언어");
    case UiText::trayOpen: return juce::String(L"열기");
    case UiText::audioSettingsMenu: return juce::String(L"오디오 장치 및 샘플 설정 (&S)");
    case UiText::addVst3Menu: return juce::String(L"VST3 플러그인 추가 (&A)...");
    case UiText::clearPluginsMenu: return juce::String(L"플러그인 전체 초기화 (&C)");
    case UiText::exitMenu: return juce::String(L"종료 (&X)");
    }

    return {};
}
