#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class FLstudioIR_TelevisionAudioProcessorEditor : public juce::AudioProcessorEditor
                                              , public juce::Button::Listener
{
public:
    FLstudioIR_TelevisionAudioProcessorEditor (FLstudioIR_TelevisionAudioProcessor&);
    ~FLstudioIR_TelevisionAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void buttonClicked(juce::Button* b) override;

private:
    FLstudioIR_TelevisionAudioProcessor& audioProcessor;

    juce::Slider crushKnob;
    juce::Slider filterKnob;
    juce::Slider noiseKnob;
    juce::Slider warbleKnob;
    juce::Slider mixKnob;

    juce::Label crushLabel;
    juce::Label filterLabel;
    juce::Label noiseLabel;
    juce::Label warbleLabel;
    juce::Label mixLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> crushAttachment;
    std::unique_ptr<SliderAttachment> filterAttachment;
    std::unique_ptr<SliderAttachment> noiseAttachment;
    std::unique_ptr<SliderAttachment> warbleAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;

    juce::TextEditor licenseInputBox;
    juce::TextButton unlockButton;
    juce::Label machineIdDisplay;
    
    // دکمه جدید برای کپی کردن Machine ID
    juce::TextButton copyIdButton; 

    juce::Colour flirOrange = juce::Colour::fromString("#FFFF6600"); 
    juce::Colour flirGreen  = juce::Colour::fromString("#FF00FF66");
    juce::Colour darkCarbon = juce::Colour::fromString("#FF121212");
    juce::Colour tvGrey     = juce::Colour::fromString("#FF2A2A2A");

    void setupKnobAndLabel(juce::Slider& slider, juce::Label& label, const juce::String& text, std::unique_ptr<SliderAttachment>& attach, const juce::String& paramId);
    void drawCrtScreen(juce::Graphics& g, juce::Rectangle<int> bounds);
    void hideActivationPanel();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FLstudioIR_TelevisionAudioProcessorEditor)
};
