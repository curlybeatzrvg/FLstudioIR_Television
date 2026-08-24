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
    // قفل کردن کل سیستم روی ۲ کانال (استریو) برای جلوگیری از کرش سایدچین اف ال استودیو
    int maxCh = 2; 
    int safeSamples = std::max(samplesPerBlock, 4096); 

    tapeWarble.prepare(sampleRate, safeSamples, maxCh);
    smartNoise.prepare(sampleRate);
    crtSaturation.prepare(sampleRate);
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = std::max(10.0, sampleRate);
    spec.maximumBlockSize = static_cast<juce::uint32>(safeSamples);
    spec.numChannels = static_cast<juce::uint32>(maxCh);
    tvFilter.prepare(spec);

    dryBuffer.setSize(maxCh, safeSamples);
}

void FLstudioIR_TelevisionAudioProcessor::releaseResources()
{
}

void FLstudioIR_TelevisionAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    int totalNumInputChannels = getTotalNumInputChannels();
    int totalNumOutputChannels = getTotalNumOutputChannels();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    if (!isUnlocked.load()) return;

    int numSamples = buffer.getNumSamples();
    
    // سد امنیتی اصلی: حتی اگر اف ال استودیو ۴ کانال فرستاد، ما فقط ۲ تای اول را پردازش می‌کنیم
    int activeChannels = std::min(buffer.getNumChannels(), 2); 

    if (numSamples <= 0 || activeChannels <= 0) return;

    // داینامیک مموری برای سایزهای عجیب اف ال استودیو
    if (dryBuffer.getNumSamples() < numSamples) {
        dryBuffer.setSize(2, numSamples, true, true, true);
    }

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

// ایزوله کردن کانال‌ها برای فیلتر
    juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t)activeChannels, (size_t)numSamples);

    tapeWarble.setParameters(2.0f, warbleVal * 15.0f);
    tapeWarble.process(buffer); 

    crtSaturation.setCrushAmount(crushVal);
    crtSaturation.process(buffer);

    tvFilter.setFilterAmount(filterVal, getSampleRate());
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

// ساخت امن مموری بلاک برای الگوریتم لایسنس
juce::String FLstudioIR_TelevisionAudioProcessor::getMachineId()
{
    juce::String rawId = juce::SystemStats::getComputerName() + juce::SystemStats::getLogonName();
    juce::MemoryBlock mb (rawId.toRawUTF8(), rawId.getNumBytesAsUTF8());
    juce::MD5 md5 (mb);
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
    juce::MemoryBlock mb (rawData.toRawUTF8(), rawData.getNumBytesAsUTF8());
    juce::MD5 md5 (mb);
    auto expectedHash = md5.toHexString().toUpperCase();
    
    juce::String expectedKey = "FLIR-" + expectedHash.substring(0, 4) + "-" +
                                       expectedHash.substring(4, 8) + "-" +
                                       expectedHash.substring(8, 12) + "-" +
                                       expectedHash.substring(12, 16);

    if (inputKey.trim() == expectedKey)
    {
        isUnlocked.store(true); // ذخیره امن برای موتور صوتی
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
            isUnlocked.store(true);
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
