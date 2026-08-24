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

// ساختار جدید و استاندارد برای جلوگیری از کرش گرافیکی
juce::AudioProcessorValueTreeState::ParameterLayout FLstudioIR_TelevisionAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("crush", 1), "Crush", 0.0f, 1.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("filter", 1), "Filter", 0.0f, 1.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("noise", 1), "Noise", -60.0f, 0.0f, -40.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("warble", 1), "Warble", 0.0f, 1.0f, 0.2f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("mix", 1), "Mix", 0.0f, 1.0f, 1.0f));
    return layout;
}

void FLstudioIR_TelevisionAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // آماده‌سازی حافظه برای سنگین‌ترین حالت اف ال استودیو
    int maxCh = 4; 
    int maxSamples = samplesPerBlock * 4; 

    tapeWarble.prepare(sampleRate, maxSamples, maxCh);
    smartNoise.prepare(sampleRate);
    crtSaturation.prepare(sampleRate);
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = std::max(10.0, sampleRate);
    spec.maximumBlockSize = static_cast<juce::uint32>(maxSamples);
    spec.numChannels = static_cast<juce::uint32>(maxCh);
    tvFilter.prepare(spec);

    dryBuffer.setSize(maxCh, maxSamples);
}

void FLstudioIR_TelevisionAudioProcessor::releaseResources()
{
}

void FLstudioIR_TelevisionAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    if (!isUnlocked) return; 

    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    // محافظت مطلق: اگر اف ال استودیو صدای غیرمجاز فرستاد پردازش نکن
    if (numSamples == 0  numChannels == 0  numSamples > dryBuffer.getNumSamples() || numChannels > dryBuffer.getNumChannels()) return;

    int activeChannels = std::min(numChannels, dryBuffer.getNumChannels());

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

    for (int ch = 0; ch < activeChannels; ++ch) {
        dryBuffer.copyFrom(ch, 0, buffer.getReadPointer(ch), numSamples);
    }

    tapeWarble.setParameters(2.0f, warbleVal * 15.0f);
    tapeWarble.process(buffer);

    crtSaturation.setCrushAmount(crushVal);
    crtSaturation.process(buffer);

    tvFilter.setFilterAmount(filterVal, getSampleRate());
    juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), activeChannels, numSamples);
    tvFilter.process(block);

    smartNoise.setNoiseLevel(noiseVal);
    smartNoise.process(buffer);

    for (int ch = 0; ch < activeChannels; ++ch)
    {
        auto* channelData = buffer.getWritePointer(ch);
        const auto* dryData = dryBuffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            channelData[i] = (dryData[i] * (1.0f - mixVal)) + (channelData[i] * mixVal);
        }
    }
}

// الگوریتم بومی و ضدکرش MD5
juce::String FLstudioIR_TelevisionAudioProcessor::getMachineId()
{
    juce::String rawId = juce::SystemStats::getComputerName() + juce::SystemStats::getLogonName();
    juce::MD5 md5(rawId.toUTF8());
    return "FLIR-" + md5.toHexString().toUpperCase().substring(0, 16);
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
    juce::MD5 md5(rawData.toUTF8());
    auto expectedHash = md5.toHexString().toUpperCase();
    
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
    if (xml != nullptr) copyXmlToBinary (*xml, destData);
}

void FLstudioIR_TelevisionAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
    {
        if (xmlState->hasTagName (parameters.state.getType()))
        {
            juce::ValueTree tree = juce::ValueTree::fromXml (*xmlState);
            if (tree.isValid()) parameters.replaceState (tree);
        }
    }
}

juce::AudioProcessorEditor* FLstudioIR_TelevisionAudioProcessor::createEditor()
{
    return new FLstudioIR_TelevisionAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FLstudioIR_TelevisionAudioProcessor();
}
