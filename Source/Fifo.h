#include <JuceHeader.h>

class Fifo
{
public:
    Fifo() = default;
    ~Fifo() = default;

    void prepare(int numChannels, int maxNumSamples)
    {
        jassert(numChannels > 0);
        _buffer.setSize(numChannels, maxNumSamples);

        for (int i = 0; i < numChannels; i++)
            _abstractFifos.add(new juce::SingleThreadedAbstractFifo(maxNumSamples));
    }

    void write(int channel, float sample)
    {
        jassert(getNumAvailableToWrite(channel) >= 1);

        auto ranges = _abstractFifos[channel]->write(1);
        _buffer.setSample(channel, ranges[0].getStart(), sample);
    }

    float read(int channel)
    {
        jassert(getNumAvailableToRead(channel) >= 1);

        auto ranges = _abstractFifos[channel]->read(1);
        return _buffer.getSample(channel, ranges[0].getStart());
    }

    int getNumAvailableToWrite(int channel) const
    {
        jassert(channel < _buffer.getNumChannels());

        return _abstractFifos[channel]->getRemainingSpace();
    }

    int getNumAvailableToRead(int channel) const
    {
        jassert(channel < _buffer.getNumChannels());
        
        return _abstractFifos[channel]->getNumReadable();
    }

private:
    juce::OwnedArray<juce::SingleThreadedAbstractFifo> _abstractFifos;
    juce::AudioBuffer<float> _buffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Fifo)
};
