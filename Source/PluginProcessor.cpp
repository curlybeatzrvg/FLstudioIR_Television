#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <fstream>
#include <functional>
#include <algorithm>

static const juce::String SECRET_SALT = "FLstudioIR_Secret_LoFi_Key_2026_!@#";

FLstudioIR_TelevisionAudioProcessor::FLstudioIR_TelevisionAudioProcessor()
    : AudioProcessor (BusesProperties()
                     .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "Parameters", createParameterLayout())
{
    checkSavedLicense();
}

FLstudioIR_TelevisionAudioProcessor::~FLstudioIR_TelevisionAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout FLstudioIR_TelevisionAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterFloat>("crush", "Crush", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("filter", "Filter", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("noise", "Noise", juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -40.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("warble", "Warble", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("mix", "Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));
    return { params.begin(), params.end() };
}

void FLstudioIR_TelevisionAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    int numChannels = juce::jmax(1, getTotalNumOutputChannels());
    tapeWarble.prepare(sampleRate, samplesPerBlock, numChannels);
    smartNoise.prepare(sampleRate);
    crtSaturation.prepare(sampleRate);
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(numChannels);
    tvFilter.prepare(spec);
}

void FLstudioIR_TelevisionAudioProcessor::releaseResources()
{
}

void FLstudioIR_TelevisionAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    if (!isUnlocked) {
        return; 
    }

    float crushVal  = parameters.getRawParameterValue("crush")->load();
    float filterVal = parameters.getRawParameterValue("filter")->load();
    float noiseVal  = parameters.getRawParameterValue("noise")->load();
    float warbleVal = parameters.getRawParameterValue("warble")->load();
    float mixVal    = parameters.getRawParameterValue("mix")->load();

    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    tapeWarble.setParameters(2.0f, warbleVal * 15.0f);
    tapeWarble.process(buffer);

    crtSaturation.setCrushAmount(crushVal);
    crtSaturation.process(buffer);

    // محافظت از کرش فیلترها
    int activeChannels = std::min(totalNumInputChannels, buffer.getNumChannels());
    tvFilter.setFilterAmount(filterVal, getSampleRate());
    juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), activeChannels, buffer.getNumSamples());
    tvFilter.process(block);

    smartNoise.setNoiseLevel(noiseVal);
    smartNoise.process(buffer);

    for (int channel = 0; channel < activeChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        const auto* dryData = dryBuffer.getReadPointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            channelData[sample] = (dryData[sample] * (1.0f - mixVal)) + (channelData[sample] * mixVal);
        }
    }
}

juce::String FLstudioIR_TelevisionAudioProcessor::getMachineId()
{
    juce::String rawId = juce::SystemStats::getComputerName() + juce::SystemStats::getLogonName();
    juce::SHA256 sha(rawId.toRawUTF8(), (size_t) rawId.getNumBytesAsUTF8());
    auto hash = sha.toHexString().toUpperCase();
    return "FLIR-" + hash.substring(0, 16);
}

bool FLstudioIR_TelevisionAudioProcessor::verifyLicenseKey(const juce::String& inputKey)
{
    juce::String machineId = getMachineId();
    juce::String rawData = machineId + SECRET_SALT;
    juce::SHA256 sha(rawData.toRawUTF8(), (size_t) rawData.getNumBytesAsUTF8());
    auto expectedHash = sha.toHexString().toUpperCase();
    
    juce::String expectedKey = "FLIR-" + expectedHash.substring(0, 4) + "-" +
                                       expectedHash.substring(4, 8) + "-" +
                                       expectedHash.substring(8, 12) + "-" +
                                       expectedHash.substring(12, 16);

    if (inputKey.trim() == expectedKey)
    {
        isUnlocked = true;
        juce::File licenseFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                 .getChildFile("FLstudioIR")
                                 .getChildFile("Television.lic");
        // باید حتماً قبل از ذخیره، پوشه ساخته شود
        licenseFile.getParentDirectory().createDirectory(); 
        licenseFile.replaceWithText(inputKey);
        return true;
    }
    return false;
}

void FLstudioIR_TelevisionAudioProcessor::checkSavedLicense()
{
    juce::File licenseFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                             .getChildFile("FLstudioIR")
                             .getChildFile("Television.lic");
    if (licenseFile.existsAsFile())
    {
        juce::String savedKey = licenseFile.loadFileAsString();
        if (verifyLicenseKey(savedKey))
        {
            isUnlocked = true;
        }
    }
}

void FLstudioIR_TelevisionAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void FLstudioIR_TelevisionAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessorEditor* FLstudioIR_TelevisionAudioProcessor::createEditor()
{
    return new FLstudioIR_TelevisionAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FLstudioIR_TelevisionAudioProcessor();
}
