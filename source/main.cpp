#include <JuceHeader.h>
#include <windows.h>
#include <shellapi.h>
#include <memory>
#include <string>

#include "AudioPipeline.h"
#include "CustomWindow.h"
#include "PluginManagerComponent.h"

#define WM_TRAYICON (WM_USER + 1)
#define IDM_OPEN_MANAGER 1000
#define IDM_DEVICE_SETTINGS 1001
#define IDM_ADD_VST3       1002
#define IDM_CLEAR_VST3     1003
#define IDM_EXIT           1004
#define IDM_PLUGIN_BASE    2000

namespace
{
NOTIFYICONDATAW trayIconData = {};
std::unique_ptr<AudioPipeline> audioPipeline;
std::unique_ptr<CustomWindow> deviceSettingsWindow;
std::unique_ptr<CustomWindow> pluginManagerWindow;

UiLanguage currentLanguage()
{
    return audioPipeline != nullptr ? audioPipeline->getUiLanguage() : UiLanguage::korean;
}

void appendLocalizedMenuItem(HMENU menu, UINT flags, UINT id, UiText text)
{
    auto label = localizedText(currentLanguage(), text);
    AppendMenuW(menu, flags, id, label.toWideCharPointer());
}

void showAudioDeviceSettings();

void openPluginManager()
{
    if (audioPipeline == nullptr)
        return;

    if (pluginManagerWindow == nullptr)
    {
        pluginManagerWindow = std::make_unique<CustomWindow>(
            localizedText(currentLanguage(), UiText::managerTitle),
            juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId),
            juce::DocumentWindow::closeButton | juce::DocumentWindow::minimiseButton);
        pluginManagerWindow->setContentOwned(new PluginManagerComponent(
            *audioPipeline,
            [] { showAudioDeviceSettings(); },
            []
            {
                if (pluginManagerWindow != nullptr)
                    pluginManagerWindow->setName(localizedText(currentLanguage(), UiText::managerTitle));
                if (deviceSettingsWindow != nullptr)
                    deviceSettingsWindow->setName(localizedText(currentLanguage(), UiText::audioSettingsTitle));
            }), true);
        pluginManagerWindow->setResizable(true, false);
        pluginManagerWindow->centreWithSize(760, 500);
    }

    pluginManagerWindow->setVisible(true);
    pluginManagerWindow->toFront(true);
}

void showAudioDeviceSettings()
{
    if (audioPipeline == nullptr)
        return;

    if (deviceSettingsWindow != nullptr)
    {
        deviceSettingsWindow->setVisible(true);
        deviceSettingsWindow->toFront(true);
        return;
    }

    auto selector = std::make_unique<juce::AudioDeviceSelectorComponent>(
        audioPipeline->getDeviceManager(),
        1, 2,
        1, 2,
        false,
        false,
        true,
        true);

    selector->setSize(520, 560);

    deviceSettingsWindow = std::make_unique<CustomWindow>(
        localizedText(currentLanguage(), UiText::audioSettingsTitle),
        juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId),
        juce::DocumentWindow::closeButton);

    deviceSettingsWindow->setContentOwned(selector.release(), true);
    deviceSettingsWindow->centreWithSize(520, 560);
    deviceSettingsWindow->setVisible(true);
}

void browseAndAddVst3()
{
    auto vst3Directory = juce::File::getSpecialLocation(juce::File::globalApplicationsDirectory)
                            .getChildFile("Common Files/VST3");

    auto chooser = std::make_shared<juce::FileChooser>(
        localizedText(currentLanguage(), UiText::chooseVst3Plugin), vst3Directory, "*.vst3");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [chooser](const juce::FileChooser& fileChooser)
        {
            auto file = fileChooser.getResult();
            if (file.exists() && audioPipeline != nullptr)
                audioPipeline->addVst3Plugin(file, true);
        });
}

void showTrayMenu(HWND window)
{
    POINT position;
    GetCursorPos(&position);

    HMENU menu = CreatePopupMenu();
    appendLocalizedMenuItem(menu, MF_STRING, IDM_OPEN_MANAGER, UiText::trayOpen);
    appendLocalizedMenuItem(menu, MF_STRING, IDM_DEVICE_SETTINGS, UiText::audioSettingsMenu);
    appendLocalizedMenuItem(menu, MF_STRING, IDM_ADD_VST3, UiText::addVst3Menu);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

    if (audioPipeline != nullptr && audioPipeline->getPluginCount() > 0)
    {
        for (size_t index = 0; index < audioPipeline->getPluginCount(); ++index)
        {
            HMENU pluginMenu = CreatePopupMenu();
            UINT openId = IDM_PLUGIN_BASE + static_cast<UINT>(index) * 10 + 1;
            UINT upId = IDM_PLUGIN_BASE + static_cast<UINT>(index) * 10 + 2;
            UINT downId = IDM_PLUGIN_BASE + static_cast<UINT>(index) * 10 + 3;
            UINT deleteId = IDM_PLUGIN_BASE + static_cast<UINT>(index) * 10 + 4;

            auto editorLabel = localizedText(currentLanguage(), UiText::openPluginEditor);
            auto moveUpLabel = localizedText(currentLanguage(), UiText::moveUp);
            auto moveDownLabel = localizedText(currentLanguage(), UiText::moveDown);
            auto removeLabel = localizedText(currentLanguage(), UiText::removePlugin);
            AppendMenuW(pluginMenu, MF_STRING, openId, editorLabel.toWideCharPointer());
            AppendMenuW(pluginMenu, MF_STRING | (index == 0 ? MF_GRAYED : 0), upId, moveUpLabel.toWideCharPointer());
            AppendMenuW(pluginMenu, MF_STRING | (index == audioPipeline->getPluginCount() - 1 ? MF_GRAYED : 0), downId, moveDownLabel.toWideCharPointer());
            AppendMenuW(pluginMenu, MF_STRING, deleteId, removeLabel.toWideCharPointer());

            std::wstring title = L"[" + std::to_wstring(index + 1) + L"] "
                                 + audioPipeline->getPluginName(index).toWideCharPointer();
            AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(pluginMenu), title.c_str());
        }

        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        appendLocalizedMenuItem(menu, MF_STRING, IDM_CLEAR_VST3, UiText::clearPluginsMenu);
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    }

    appendLocalizedMenuItem(menu, MF_STRING, IDM_EXIT, UiText::exitMenu);
    SetForegroundWindow(window);
    int command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY,
                                 position.x, position.y, 0, window, nullptr);
    DestroyMenu(menu);

    if (command == IDM_OPEN_MANAGER)
        openPluginManager();
    else if (command == IDM_DEVICE_SETTINGS)
        showAudioDeviceSettings();
    else if (command == IDM_ADD_VST3)
        browseAndAddVst3();
    else if (command == IDM_CLEAR_VST3 && audioPipeline != nullptr)
        audioPipeline->clearPlugins();
    else if (command == IDM_EXIT)
    {
        Shell_NotifyIconW(NIM_DELETE, &trayIconData);
        DestroyIcon(trayIconData.hIcon);
        PostQuitMessage(0);
    }
    else if (command >= IDM_PLUGIN_BASE)
    {
        UINT offset = static_cast<UINT>(command - IDM_PLUGIN_BASE);
        size_t pluginIndex = offset / 10;
        UINT subCommand = offset % 10;

        if (audioPipeline != nullptr && pluginIndex < audioPipeline->getPluginCount())
        {
            if (subCommand == 1)
                audioPipeline->togglePluginEditor(pluginIndex);
            else if (subCommand == 2)
                audioPipeline->movePluginUp(pluginIndex);
            else if (subCommand == 3)
                audioPipeline->movePluginDown(pluginIndex);
            else if (subCommand == 4)
                audioPipeline->removePlugin(pluginIndex);
        }
    }
}

LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_TRAYICON:
        if (lParam == WM_LBUTTONDBLCLK)
            openPluginManager();
        else if (lParam == WM_RBUTTONUP)
            showTrayMenu(window);
        break;

    case WM_DESTROY:
        Shell_NotifyIconW(NIM_DELETE, &trayIconData);
        DestroyIcon(trayIconData.hIcon);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(window, message, wParam, lParam);
    }

    return 0;
}

void initializeTrayIcon(HWND window, HINSTANCE instance)
{
    trayIconData.cbSize = sizeof(NOTIFYICONDATAW);
    trayIconData.hWnd = window;
    trayIconData.uID = 1;
    trayIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    trayIconData.uCallbackMessage = WM_TRAYICON;

    wchar_t executablePath[MAX_PATH];
    GetModuleFileNameW(nullptr, executablePath, MAX_PATH);

    HICON icon = nullptr;
    ExtractIconExW(executablePath, 0, nullptr, &icon, 1);
    if (icon == nullptr)
    {
        icon = static_cast<HICON>(LoadImageW(instance, L"IDI_ICON1", IMAGE_ICON,
                                            GetSystemMetrics(SM_CXSMICON),
                                            GetSystemMetrics(SM_CYSMICON),
                                            LR_DEFAULTCOLOR));
    }

    if (icon == nullptr)
        juce::Logger::writeToLog("Tray icon load failed, error=" + juce::String(static_cast<int>(GetLastError())));

    trayIconData.hIcon = icon != nullptr ? icon : LoadIcon(nullptr, IDI_APPLICATION);
    wcscpy_s(trayIconData.szTip, L"Microphone VST Effector");
    Shell_NotifyIconW(NIM_ADD, &trayIconData);
}
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int)
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    auto logFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                       .getSiblingFile("MicEffector.log");
    juce::FileLogger fileLogger(logFile, "MicEffector log");
    juce::Logger::setCurrentLogger(&fileLogger);

    WNDCLASSEXW windowClass = { sizeof(WNDCLASSEXW) };
    windowClass.lpfnWndProc = windowProcedure;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = L"MicEffectorTrayClass";
    RegisterClassExW(&windowClass);

    HWND window = CreateWindowExW(0, L"MicEffectorTrayClass", L"MicEffectorTray",
                                  0, 0, 0, 0, 0, nullptr, nullptr, instance, nullptr);
    initializeTrayIcon(window, instance);
    audioPipeline = std::make_unique<AudioPipeline>();

    MSG message;
    while (GetMessage(&message, nullptr, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    deviceSettingsWindow.reset();
    pluginManagerWindow.reset();
    audioPipeline.reset();
    juce::Logger::setCurrentLogger(nullptr);
    return static_cast<int>(message.wParam);
}
