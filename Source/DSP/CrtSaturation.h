#pragma once
#include <JuceHeader.h>
#include <cmath>

namespace lulu_dsp 
{
    class CrtSaturation 
    {
    public:
        CrtSaturation() = default;

        void prepare(double sampleRate) 
        {
        }

        void setCrushAmount(float amount) 
        {
            crushAmount = juce::jlimit(0.0f, 1.0f, amount);
            bitDepth = 16.0f - (crushAmount * 12.0f);
            driveAmount = 1.0f + (crushAmount * 5.0f);
        }

        void process(juce::AudioBuffer<float>& buffer) 
        {
            const int numChannels = buffer.getNumChannels();
            const int numSamples = buffer.getNumSamples();

            float q = std::pow(2.0f, bitDepth - 1.0f);

            for (int channel = 0; channel < numChannels; ++channel) 
            {
                float* channelData = buffer.getWritePointer(channel);
                for (int i = 0; i < numSamples; ++i) 
                {
                    float sample = channelData[i];
                    if (crushAmount > 0.05f) {
                        sample = std::round(sample * q) / q;
                    }
                    sample *= driveAmount;
                    sample = std::tanh(sample);
                    sample /= (1.0f + (crushAmount * 0.5f)); 
                    channelData[i] = sample;
                }
            }
        }

    private:
        float crushAmount = 0.0f;
        float bitDepth = 16.0f;
        float driveAmount = 1.0f;
    };
}
