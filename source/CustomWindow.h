#pragma once

#include <JuceHeader.h>

class CustomWindow : public juce::DocumentWindow
{
public:
    CustomWindow(const juce::String& name, juce::Colour backgroundColour, int requiredButtons)
        : juce::DocumentWindow(name, backgroundColour, requiredButtons)
    {
        setUsingNativeTitleBar(true);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }
};
