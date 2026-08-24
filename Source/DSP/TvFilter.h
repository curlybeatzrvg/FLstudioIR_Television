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
            if (sampleRate < 10.0) sampleRate = 44100.0; // محافظت در برابر کرش
            
            float lpFreq = juce::jmap(amount, 0.0f, 1.0f, 20000.0f, 3000.0f);
            float hpFreq = juce::jmap(amount, 0.0f, 1.0f, 20.0f, 300.0f);
            
            // باگ مهلک اینجا بود! تغییر روش مقداردهی به پوینترها
            lowPassFilter.state = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, lpFreq, 0.707f);
            highPassFilter.state = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, hpFreq, 0.707f);
        }

        void process(juce::dsp::AudioBlock<float>& block) 
        {
            // فقط در صورتی پردازش کن که حافظه با موفقیت ساخته شده باشد
            if (lowPassFilter.state != nullptr && highPassFilter.state != nullptr)
            {
                juce::dsp::ProcessContextReplacing<float> context(block);
                lowPassFilter.process(context);
                highPassFilter.process(context);
            }
        }

    private:
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> lowPassFilter;
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> highPassFilter;
    };
}
