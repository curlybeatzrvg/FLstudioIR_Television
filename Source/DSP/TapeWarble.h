#pragma once
#include <JuceHeader.h>

namespace lulu_dsp 
{
    class TapeWarble 
    {
    public:
        TapeWarble() = default;

        void prepare(double sampleRate, int samplesPerBlock) 
        {
            currentSampleRate = sampleRate;
            delayBuffer.setSize(2, static_cast<int>(sampleRate) + 1); 
            delayBuffer.clear();
            lfoPhase = 0.0f;
            writePosition = 0;
        }

        void setParameters(float rateHz, float depthMs) 
        {
            lfoRate = rateHz;
            lfoDepth = depthMs * (currentSampleRate / 1000.0f);
        }

        void process(juce::AudioBuffer<float>& buffer) 
        {
            const int numChannels = buffer.getNumChannels();
            const int numSamples = buffer.getNumSamples();

            for (int channel = 0; channel < numChannels; ++channel) 
            {
                float* channelData = buffer.getWritePointer(channel);
                for (int i = 0; i < numSamples; ++i) 
                {
                    float lfoValue = std::sin(lfoPhase * juce::MathConstants<float>::twoPi);
                    float delayTime = lfoDepth * (0.5f + 0.5f * lfoValue); 
                    float readPosition = std::fmod(writePosition - delayTime + delayBuffer.getNumSamples(), delayBuffer.getNumSamples());
                    float interpolatedSample = getLinearInterpolatedSample(channel, readPosition);
                    delayBuffer.setSample(channel, writePosition, channelData[i]);
                    channelData[i] = interpolatedSample;
                }
            }
            lfoPhase += (lfoRate / currentSampleRate) * numSamples;
            if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
            writePosition = (writePosition + numSamples) % delayBuffer.getNumSamples();
        }

    private:
        float getLinearInterpolatedSample(int channel, float readPos) 
        {
            int index1 = static_cast<int>(readPos);
            int index2 = (index1 + 1) % delayBuffer.getNumSamples();
            float fraction = readPos - index1;
            return delayBuffer.getSample(channel, index1) * (1.0f - fraction) + delayBuffer.getSample(channel, index2) * fraction;
        }

        juce::AudioBuffer<float> delayBuffer;
        double currentSampleRate = 44100.0;
        float lfoPhase = 0.0f;
        float lfoRate = 1.0f;
        float lfoDepth = 10.0f;
        int writePosition = 0;
    };
}
