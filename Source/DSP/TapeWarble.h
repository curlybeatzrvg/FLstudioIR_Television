#pragma once
#include <JuceHeader.h>
#include <algorithm>
#include <cmath>

namespace lulu_dsp 
{
    class TapeWarble 
    {
    public:
        TapeWarble() = default;

        void prepare(double sampleRate, int /*samplesPerBlock*/, int maxChannels) 
        {
            currentSampleRate = std::max(10.0, sampleRate);
            delayBuffer.setSize(maxChannels, static_cast<int>(currentSampleRate) + 1); 
            delayBuffer.clear();
            lfoPhase = 0.0f;
            writePosition = 0;
        }

        void setParameters(float rateHz, float depthMs) 
        {
            lfoRate = rateHz;
            lfoDepth = depthMs * static_cast<float>(currentSampleRate / 1000.0);
        }

        void process(juce::AudioBuffer<float>& buffer) 
        {
            if (delayBuffer.getNumSamples() == 0 || delayBuffer.getNumChannels() == 0) return;

            const int numChannels = std::min(buffer.getNumChannels(), delayBuffer.getNumChannels());
            const int numSamples = buffer.getNumSamples();

            for (int channel = 0; channel < numChannels; ++channel) 
            {
                float* channelData = buffer.getWritePointer(channel);
                int currentWritePos = writePosition;
                float currentLfoPhase = lfoPhase;

                for (int i = 0; i < numSamples; ++i) 
                {
                    float lfoValue = std::sin(currentLfoPhase * juce::MathConstants<float>::twoPi);
                    float delayTime = lfoDepth * (0.5f + 0.5f * lfoValue); 
                    
                    float readPosition = static_cast<float>(currentWritePos) - delayTime;
                    while (readPosition < 0.0f) readPosition += static_cast<float>(delayBuffer.getNumSamples());
                    while (readPosition >= static_cast<float>(delayBuffer.getNumSamples())) readPosition -= static_cast<float>(delayBuffer.getNumSamples());

                    float interpolatedSample = getLinearInterpolatedSample(channel, readPosition);
                    delayBuffer.setSample(channel, currentWritePos, channelData[i]);
                    channelData[i] = interpolatedSample;

                    currentWritePos++;
                    if (currentWritePos >= delayBuffer.getNumSamples()) currentWritePos = 0;
                    
                    currentLfoPhase += (lfoRate / static_cast<float>(currentSampleRate));
                    if (currentLfoPhase >= 1.0f) currentLfoPhase -= 1.0f;
                }
            }
            
            writePosition = (writePosition + numSamples) % delayBuffer.getNumSamples();
            lfoPhase += (lfoRate / static_cast<float>(currentSampleRate)) * static_cast<float>(numSamples);
            while (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
        }

    private:
        float getLinearInterpolatedSample(int channel, float readPos) 
        {
            int index1 = static_cast<int>(readPos);
            if (index1 >= delayBuffer.getNumSamples()) index1 = delayBuffer.getNumSamples() - 1;
            if (index1 < 0) index1 = 0;
            
            int index2 = (index1 + 1) % delayBuffer.getNumSamples();
            float fraction = readPos - static_cast<float>(index1);
            
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
