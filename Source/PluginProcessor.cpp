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
    spec.sampleRate = std::max(10.0, sampleRate);
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(numChannels);
    tvFilter.prepare(spec);

    // آماده‌سازی امن حافظه بدون ایجاد بار روی سی‌پی‌یو
    dryBuffer.setSize(numChannels, samplesPerBlock);
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

    int activeChannels = std::min(totalNumInputChannels, buffer.getNumChannels());
    if (activeChannels == 0 || buffer.getNumSamples() == 0) return; // محافظت از خالی بودن صدا

    auto* crushParam = parameters.getRawParameterValue("crush");
    auto* filterParam = parameters.getRawParameterValue("filter");
    auto* noiseParam = parameters.getRawParameterValue("noise");
    auto* warbleParam = parameters.getRawParameterValue("warble");
    auto* mixParam = parameters.getRawParameterValue("mix");

    float crushVal  = crushParam != nullptr ? crushParam->load() : 0.0f;
    float filterVal = filterParam != nullptr ? filterParam->load() : 0.0f;
    float noiseVal  = noiseParam != nullptr ? noiseParam->load() : -40.0f;
    float warbleVal = warbleParam != nullptr ? warbleParam->load() : 0.2f;
    float mixVal    = mixParam != nullptr ? mixParam->load() : 1.0f;

    // کپی کاملاً امن و ضدکرش برای Dry/Wet
    for (int ch = 0; ch < activeChannels; ++ch) {
        if (ch < dryBuffer.getNumChannels()) {
            dryBuffer.copyFrom(ch, 0, buffer.getReadPointer(ch), buffer.getNumSamples());
        }
    }

    tapeWarble.setParameters(2.0f, warbleVal * 15.0f);
    tapeWarble.process(buffer);

    crtSaturation.setCrushAmount(crushVal);
    crtSaturation.process(buffer);

    tvFilter.setFilterAmount(filterVal, getSampleRate());
    juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), activeChannels, buffer.getNumSamples());
    tvFilter.process(block);

    smartNoise.setNoiseLevel(noiseVal);
    smartNoise.process(buffer);

    for (int channel = 0; channel < activeChannels; ++channel)
    {
        if (channel < dryBuffer.getNumChannels()) {
            auto* channelData = buffer.getWritePointer(channel);
            const auto* dryData = dryBuffer.getReadPointer(channel);
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                channelData[sample] = (dryData[sample] * (1.0f - mixVal)) + (channelData[sample] * mixVal);
            }
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

juce::File FLstudioIR_TelevisionAudioProcessor::getLicenseFile()
{
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
           .getChildFile("FLstudioIR")
           .getChildFile("Television.lic");
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
        juce::File licenseFile = getLicenseFile();
        
        licenseFile.getParentDirectory().createDirectory(); 
        licenseFile.replaceWithText(inputKey);
        return true;
    }
    return false;
}

void FLstudioIR_TelevisionAudioProcessor::checkSavedLicense()
{
    juce::File licenseFile = getLicenseFile();
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
