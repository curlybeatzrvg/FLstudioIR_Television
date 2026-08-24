#pragma once
#include <JuceHeader.h>
#include <random>

namespace lulu_dsp 
{
    class SmartNoise 
    {
    public:
        SmartNoise() 
        {
            generator.seed(std::random_device{}());
            distribution = std::uniform_real_distribution<float>(-1.0f, 1.0f);
        }

        void prepare(double sampleRate) 
        {
            envelopeValue = 0.0f;
            envelopeRelease = std::exp(-1.0f / (sampleRate * 0.5f));
        }

        void setNoiseLevel(float levelDb) 
        {
            noiseGain = juce::Decibels::decibelsToGain(levelDb);
        }

        void process(juce::AudioBuffer<float>& buffer) 
        {
            const int numSamples = buffer.getNumSamples();
            const int numChannels = buffer.getNumChannels();

            for (int i = 0; i < numSamples; ++i) 
            {
                float maxInput = 0.0f;
                for (int ch = 0; ch < numChannels; ++ch) 
                {
                    maxInput = std::max(maxInput, std::abs(buffer.getSample(ch, i)));
                }

                if (maxInput > envelopeValue)
                    envelopeValue = maxInput;
                else
                    envelopeValue = envelopeValue * envelopeRelease;

                float noiseSample = distribution(generator) * noiseGain * envelopeValue;

                for (int ch = 0; ch < numChannels; ++ch) 
                {
                    buffer.addSample(ch, i, noiseSample);
                }
            }
        }

    private:
        std::mt19937 generator;
        std::uniform_real_distribution<float> distribution;
        float noiseGain = 0.0f;
        float envelopeValue = 0.0f;
        float envelopeRelease = 0.99f;
    };
}
