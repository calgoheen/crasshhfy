#include "PluginProcessor.h"
#include "PluginEditor.h"

Text2SampleAudioProcessor::Text2SampleAudioProcessor()
  : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    _parameters(*this, nullptr, "PARAMS", createParameterLayout())
{
    for (int i = 0; i < numSounds; i++)
    {
        auto sound = new DrumSound(baseMidiNote + i);
        _sounds.push_back(sound);
        _synth.addSound(sound);
    }

    for (int i = 0; i < numVoices; i++)
    {
        auto voice = new Voice();
        _voices.push_back(voice);
        _synth.addVoice(voice);
    }
}

Text2SampleAudioProcessor::~Text2SampleAudioProcessor()
{
}

bool Text2SampleAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void Text2SampleAudioProcessor::prepareToPlay(double sampleRate, int)
{
    // Unused functionality but the Synthesiser class requires a real sample rate value
    _synth.setCurrentPlaybackSampleRate(sampleRate);

    // Sample rates are actually handled by the Sound class
    for (int i = 0; i < _synth.getNumSounds(); i++)
        if (auto s = dynamic_cast<Sound*>(_synth.getSound(i).get()))
            s->setSampleRate(sampleRate);

    _midiState.reset();
}

void Text2SampleAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    _midiState.processNextMidiBuffer(midi, 0, buffer.getNumSamples(), true);

    buffer.clear();
    _synth.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());

    midi.clear();
}

void Text2SampleAudioProcessor::releaseResources()
{
}

void Text2SampleAudioProcessor::getStateInformation (juce::MemoryBlock&)
{
}

void Text2SampleAudioProcessor::setStateInformation (const void*, int)
{
}

bool Text2SampleAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* Text2SampleAudioProcessor::createEditor()
{
    return new Text2SampleAudioProcessorEditor (*this);
}

juce::MidiKeyboardState& Text2SampleAudioProcessor::getMidiKeyboardState()
{
    return _midiState;
}

DrumSound* Text2SampleAudioProcessor::getSound(int soundIndex)
{
    jassert(juce::isPositiveAndBelow(soundIndex, _sounds.size()));
    return _sounds[soundIndex];
}

void Text2SampleAudioProcessor::saveSample(int soundIndex, const juce::File& file)
{
    auto sample = getSound(soundIndex)->getSample();

    if (sample != nullptr)
        Utils::writeWavFile(sample->data, sample->sampleRate, file);
}

void Text2SampleAudioProcessor::generateSample(int soundIndex)
{
    juce::AudioBuffer<float> data{ UnetModelInference::numChannels, UnetModelInference::outputSize };
    size_t classification = 0;
    float confidence = 0;

    unetModelInference.process(data.getWritePointer(0));
    classifierModelInference.process(data.getReadPointer(0), &classification, &confidence);

    Utils::normalize(data);
    data.applyGain(juce::Decibels::decibelsToGain(-3.0f));
    
    // 0 = Kick, 1 = Hat, 2 = Snare
    Drum d;
    d.sample = new Sample{ std::move(data), UnetModelInference::sampleRate };
    d.drumType = static_cast<DrumType>(classification + 1);
    d.confidence = confidence;

    getSound(soundIndex)->loadDrum(d);
}

void Text2SampleAudioProcessor::drumifySample(int soundIndex, const juce::File& file)
{
    auto [inputData, fs] = Utils::readWavFile(file);

    if (inputData.getNumSamples() == 0)
        return;

    inputData.setSize(UnetModelInference::numChannels, UnetModelInference::outputSize, true, true);

    juce::AudioBuffer<float> outputData{ UnetModelInference::numChannels, UnetModelInference::outputSize };
    size_t classification = 0;
    float confidence = 0;

    Utils::normalize(inputData);

    unetModelInference.processSeeded(outputData.getWritePointer(0), inputData.getReadPointer(0));
    classifierModelInference.process(outputData.getReadPointer(0), &classification, &confidence);

    Utils::normalize(outputData);
    outputData.applyGain(juce::Decibels::decibelsToGain(-3.0f));
    
    // 0 = Kick, 1 = Hat, 2 = Snare
    Drum d;
    d.sample = new Sample{ std::move(outputData), UnetModelInference::sampleRate };
    d.drumType = static_cast<DrumType>(classification);
    d.confidence = confidence;

    getSound(soundIndex)->loadDrum(d);
}

void Text2SampleAudioProcessor::inpaintSample(int soundIndex, const juce::File& file, bool half)
{
    auto [inputData, fs] = Utils::readWavFile(file);

    if (inputData.getNumSamples() == 0)
        return;

    inputData.setSize(UnetModelInference::numChannels, UnetModelInference::outputSize, true, true);

    juce::AudioBuffer<float> outputData{ UnetModelInference::numChannels, UnetModelInference::outputSize };
    size_t classification = 0;
    float confidence = 0;

    Utils::normalize(inputData);

    unetModelInference.processSeededInpainting(outputData.getWritePointer(0), inputData.getReadPointer(0), half);
    classifierModelInference.process(outputData.getReadPointer(0), &classification, &confidence);

    Utils::normalize(outputData);
    outputData.applyGain(juce::Decibels::decibelsToGain(-3.0f));

    // 0 = Kick, 1 = Hat, 2 = Snare
    Drum d;
    d.sample = new Sample{ std::move(outputData), UnetModelInference::sampleRate };
    d.drumType = static_cast<DrumType>(classification);
    d.confidence = confidence;

    getSound(soundIndex)->loadDrum(d);
}

const juce::String Text2SampleAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Text2SampleAudioProcessor::acceptsMidi() const
{
    return true;
}

bool Text2SampleAudioProcessor::producesMidi() const
{
    return false;
}

bool Text2SampleAudioProcessor::isMidiEffect() const
{
    return false;
}

double Text2SampleAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int Text2SampleAudioProcessor::getNumPrograms()
{
    return 1;
}

int Text2SampleAudioProcessor::getCurrentProgram()
{
    return 0;
}

void Text2SampleAudioProcessor::setCurrentProgram(int)
{
}

const juce::String Text2SampleAudioProcessor::getProgramName(int)
{
    return {};
}

void Text2SampleAudioProcessor::changeProgramName(int, const juce::String&)
{
}

juce::AudioProcessorValueTreeState::ParameterLayout Text2SampleAudioProcessor::createParameterLayout()
{
    std::vector<ParameterDefinition> definitions = {
        { "Gain", "Gain", "", { 0.0f, 1.0f }, 0.5f }
    };

    std::vector<std::unique_ptr<juce::AudioParameterFloat>> params;
    for (auto& d : definitions)
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ d.id, 1 }, 
                                                                     d.name, 
                                                                     d.range, 
                                                                     d.defaultValue,
                                                                     d.label));

    return { params.begin(), params.end() };
}

Drum Text2SampleAudioProcessor::renderCRASHSample()
{
    juce::AudioBuffer<float> data{ UnetModelInference::numChannels, UnetModelInference::outputSize };
    size_t classification = 0;
    float confidence = 0;

    unetModelInference.process(data.getWritePointer(0));
    classifierModelInference.process(data.getReadPointer(0), &classification, &confidence);
    
    // 0 = Kick, 1 = Hat, 2 = Snare
    Drum output;
    output.sample = new Sample{ std::move(data), UnetModelInference::sampleRate };
    output.drumType = static_cast<DrumType>(classification);
    output.confidence = confidence;

    return output;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Text2SampleAudioProcessor();
}
