#pragma once
#include <JuceHeader.h>
#include "DSP/TapeWarble.h"
#include "DSP/NoiseGenerator.h"
#include "DSP/CrtSaturation.h"
#include "DSP/TvFilter.h"

class FLstudioIR_TelevisionAudioProcessor : public juce::AudioProcessor
{
public:
    FLstudioIR_TelevisionAudioProcessor();
    ~FLstudioIR_TelevisionAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Television"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 25; } 
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override {}
    const juce::String getProgramName (int index) override { return {}; }
    void changeProgramName (int index, const juce::String& imageName) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool verifyLicenseKey(const juce::String& inputKey);
    juce::String getMachineId();
    bool isPluginUnlocked() const { return isUnlocked; }

    juce::AudioProcessorValueTreeState parameters;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    lulu_dsp::TapeWarble tapeWarble;
    lulu_dsp::SmartNoise smartNoise;
    lulu_dsp::CrtSaturation crtSaturation;
    lulu_dsp::TvFilter tvFilter;

    bool isUnlocked = false; 
    void checkSavedLicense(); 

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FLstudioIR_TelevisionAudioProcessor)
};
