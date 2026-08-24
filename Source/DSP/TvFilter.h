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
            // رزرو کردن قطعی حافظه قبل از روشن شدن فیلتر (جلوگیری از کرش)
            double sr = std::max(10.0, spec.sampleRate);
            lowPassFilter.state = juce::dsp::IIR::Coefficients<float>::makeLowPass(sr, 20000.0f);
            highPassFilter.state = juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, 20.0f);

            lowPassFilter.prepare(spec);
            highPassFilter.prepare(spec);
            lowPassFilter.reset();
            highPassFilter.reset();
        }

        void setFilterAmount(float amount, double sampleRate) 
        {
            double sr = std::max(10.0, sampleRate);
            float lpFreq = juce::jmap(amount, 0.0f, 1.0f, 20000.0f, 3000.0f);
            float hpFreq = juce::jmap(amount, 0.0f, 1.0f, 20.0f, 300.0f);
            
            // مقداردهی امن بدون ساخت مموری جدید
            if (lowPassFilter.state != nullptr)
                *lowPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sr, lpFreq, 0.707f);
            
            if (highPassFilter.state != nullptr)
                *highPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, hpFreq, 0.707f);
        }

        void process(juce::dsp::AudioBlock<float>& block) 
        {
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
