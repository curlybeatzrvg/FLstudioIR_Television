#include "PluginProcessor.h"
#include "PluginEditor.h"

FLstudioIR_TelevisionAudioProcessorEditor::FLstudioIR_TelevisionAudioProcessorEditor (FLstudioIR_TelevisionAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (800, 500);

    setupKnobAndLabel(crushKnob, crushLabel, "CRUSH", crushAttachment, "crush");
    setupKnobAndLabel(filterKnob, filterLabel, "FILTER", filterAttachment, "filter");
    setupKnobAndLabel(noiseKnob, noiseLabel, "NOISE", noiseAttachment, "noise");
    setupKnobAndLabel(warbleKnob, warbleLabel, "WARBLE", warbleAttachment, "warble");
    setupKnobAndLabel(mixKnob, mixLabel, "MIX", mixAttachment, "mix");

    // تنظیمات باکس لایسنس (ویژگی Ghost Text اضافه شد)
    licenseInputBox.setMultiLine(false);
    licenseInputBox.setTextToShowWhenEmpty("Enter FLstudioIR License Key here...", juce::Colours::grey); // متن گوست
    licenseInputBox.setColour(juce::TextEditor::backgroundColourId, darkCarbon);
    licenseInputBox.setColour(juce::TextEditor::textColourId, flirOrange);
    addAndMakeVisible(licenseInputBox);

    unlockButton.setButtonText("UNLOCK PLUGIN");
    unlockButton.setColour(juce::TextButton::buttonColourId, flirOrange);
    unlockButton.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
    unlockButton.addListener(this);
    addAndMakeVisible(unlockButton);

    machineIdDisplay.setText("Your Machine ID: " + audioProcessor.getMachineId(), juce::dontSendNotification);
    machineIdDisplay.setJustificationType(juce::Justification::centred);
    machineIdDisplay.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(machineIdDisplay);

    // تنظیمات دکمه کپی کردن Machine ID
    copyIdButton.setButtonText("COPY ID");
    copyIdButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
    copyIdButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    copyIdButton.addListener(this);
    addAndMakeVisible(copyIdButton);

    if (audioProcessor.isPluginUnlocked()) {
        hideActivationPanel();
    }
}

FLstudioIR_TelevisionAudioProcessorEditor::~FLstudioIR_TelevisionAudioProcessorEditor()
{
}

void FLstudioIR_TelevisionAudioProcessorEditor::setupKnobAndLabel(juce::Slider& slider, juce::Label& label, const juce::String& text, std::unique_ptr<SliderAttachment>& attach, const juce::String& paramId)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setColour(juce::Slider::rotarySliderFillColourId, flirGreen); 
    slider.setColour(juce::Slider::thumbColourId, flirOrange);
    addAndMakeVisible(slider);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(label);

    attach = std::make_unique<SliderAttachment>(audioProcessor.parameters, paramId, slider);
}

void FLstudioIR_TelevisionAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (darkCarbon);

    if (!audioProcessor.isPluginUnlocked()) 
    {
        g.setColour(juce::Colours::black.withAlpha(0.8f));
        g.fillRect(getLocalBounds());
        
        g.setColour(flirGreen);
        g.setFont(24.0f);
        g.drawFittedText("FLstudioIR TELEVISION is Locked", getLocalBounds().withY(50).withHeight(50), juce::Justification::centred, 1);
        
        g.setColour(juce::Colours::lightgrey);
        g.setFont(14.0f);
        
        // اینجا هم پیام را آپدیت کردم تا مشتری بداند اگر سوالی داشت می‌تواند از بات تلگرامت استفاده کند
        g.drawFittedText("Send your Machine ID to @FLstudioIR on Instagram to get your unique license key.\nFor support or plugin requests, use our Telegram Bot.", getLocalBounds().withY(100).withHeight(60), juce::Justification::centred, 2);
    }
    else 
    {
        juce::Rectangle<int> crtBounds (50, 50, 400, 300);
        drawCrtScreen(g, crtBounds);

        g.setColour(flirOrange);
        g.setFont(juce::Font("Helvetica", 20.0f, juce::Font::bold));
        g.drawFittedText("FLstudioIR TELEVISION", 50, 370, 400, 30, juce::Justification::centred, 1);
    }
}

void FLstudioIR_TelevisionAudioProcessorEditor::resized()
{
    if (!audioProcessor.isPluginUnlocked())
    {
        // چیدمان جدید برای دکمه کپی کنار نوشته Machine ID
        machineIdDisplay.setBounds(100, 170, 500, 30);
        copyIdButton.setBounds(600, 175, 80, 20); 

        licenseInputBox.setBounds(200, 220, 400, 30);
        unlockButton.setBounds(300, 270, 200, 40);
    }
    else
    {
        int knobSize = 80;
        int xStart = 500;
        int yStart = 50;
        int spacing = 100;

        crushKnob.setBounds(xStart, yStart, knobSize, knobSize);
        crushLabel.setBounds(xStart, yStart - 20, knobSize, 20);

        filterKnob.setBounds(xStart + spacing, yStart, knobSize, knobSize);
        filterLabel.setBounds(xStart + spacing, yStart - 20, knobSize, 20);

        noiseKnob.setBounds(xStart, yStart + spacing + 30, knobSize, knobSize);
        noiseLabel.setBounds(xStart, yStart + spacing + 10, knobSize, 20);

        warbleKnob.setBounds(xStart + spacing, yStart + spacing + 30, knobSize, knobSize);
        warbleLabel.setBounds(xStart + spacing, yStart + spacing + 10, knobSize, 20);

        mixKnob.setBounds(xStart + (spacing/2), yStart + (spacing*2) + 30, knobSize, knobSize);
        mixLabel.setBounds(xStart + (spacing/2), yStart + (spacing*2) + 10, knobSize, 20);
    }
}

void FLstudioIR_TelevisionAudioProcessorEditor::buttonClicked(juce::Button* b)
{
    // اگر کاربر روی دکمه Copy کلیک کرد
    if (b == &copyIdButton) 
    {
        juce::SystemClipboard::copyTextToClipboard(audioProcessor.getMachineId());
        copyIdButton.setButtonText("COPIED!");
        copyIdButton.setColour(juce::TextButton::buttonColourId, flirGreen);
        copyIdButton.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
    }
    // اگر کاربر روی دکمه Unlock کلیک کرد
    else if (b == &unlockButton) 
    {
        juce::String keyEntered = licenseInputBox.getText();
        if (audioProcessor.verifyLicenseKey(keyEntered))
        {
            hideActivationPanel();
            resized(); 
            repaint();
        }
        else
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "License Error", "Invalid License Key! Please contact @FLstudioIR.");
        }
    }
}

void FLstudioIR_TelevisionAudioProcessorEditor::hideActivationPanel()
{
    licenseInputBox.setVisible(false);
    unlockButton.setVisible(false);
    machineIdDisplay.setVisible(false);
    copyIdButton.setVisible(false); // دکمه کپی هم مخفی شود
}

void FLstudioIR_TelevisionAudioProcessorEditor::drawCrtScreen(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour(tvGrey);
    g.fillRoundedRectangle(bounds.toFloat(), 15.0f);
    
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(bounds.reduced(10).toFloat(), 10.0f);

    g.setColour(juce::Colours::darkgrey.withAlpha(0.3f));
    for (int y = bounds.getY() + 10; y < bounds.getBottom() - 10; y += 4)
    {
        g.drawLine(bounds.getX() + 10, y, bounds.getRight() - 10, y, 1.0f);
    }
}
