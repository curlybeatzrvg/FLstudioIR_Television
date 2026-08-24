#pragma once
#include <JuceHeader.h>

namespace lulu_dsp 
{
    class TvFilter 
    {
    public:
        TvFilter() = default;

        void prepare(juce::dsp::ProcessSpec& spec) 
        {
            lowPassFilter.prepare(spec);
            highPassFilter.prepare(spec);
            lowPassFilter.reset();
            highPassFilter.reset();
        }

        void setFilterAmount(float amount, double sampleRate) 
        {
            float lpFreq = juce::jmap(amount, 0.0f, 1.0f, 20000.0f, 3000.0f);
            float hpFreq = juce::jmap(amount, 0.0f, 1.0f, 20.0f, 300.0f);
            *lowPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, lpFreq, 0.707f);
            *highPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, hpFreq, 0.707f);
        }

        void process(juce::dsp::AudioBlock<float>& block) 
        {
            juce::dsp::ProcessContextReplacing<float> context(block);
            lowPassFilter.process(context);
            highPassFilter.process(context);
        }

    private:
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> lowPassFilter;
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> highPassFilter;
    };
}
