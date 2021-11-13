/*
  ==============================================================================

    This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include <JuceHeader.h>

#include "audio/choc_SampleBufferUtilities.h"
#include "audio/choc_SincInterpolator.h"
#include "text/choc_StringUtilities.h"



//==============================================================================
class EnvelopeFromSample
{
public:
    EnvelopeFromSample (choc::buffer::ChannelArrayBuffer<float>& s, float sampleRate_, int framesPerSample_, int framesBetweenSamples_, float threshhold_)
        : sample (s), sampleRate (sampleRate_), framesPerSample (framesPerSample_), framesBetweenSamples (framesBetweenSamples_), threshhold (threshhold_)
    {
        window.resize (framesPerSample);

        for (int i = 0; i < framesPerSample; i++)
            window[i] = raisedCosine (float (i) / framesPerSample);
    }

    std::vector<float> getEnvelope (float frequency)
    {
        std::vector<float> result;

        int offset = 0;

        float phaseIncrement = frequency / sampleRate;
        const float twoPi = 3.14159265f * 2.0f;

        while (offset < sample.getNumFrames())
        {
            float phase = 0.0f;
            float real = 0.0f, imag = 0.0f;

            // Correlate the signal with our frequency
            for (int i = 0; i < framesPerSample; i++)
            {
                float s = window[i] * sample.getSampleIfInRange (0, offset + i);
                real += s * sin (phase * twoPi);
                imag += s * cos (phase * twoPi);

                phase += phaseIncrement;

                if (phase > 1.0f)
                    phase -= 1.0f;
            }

            float v = sqrt ((real * real) + (imag * imag)) / framesPerSample;

            v = v * 4.0f;   // Correct amplitude

            if (v < threshhold)
                v = 0.0f;

            result.push_back (v);

            offset += framesBetweenSamples;
        }

        return result;
    }

private:
     float raisedCosine (float pos)
     {
         return (1.0f - cos(2.0 * 3.14159265f * pos)) * 0.5f;
     }

    choc::buffer::ChannelArrayBuffer<float>& sample;
    float sampleRate;
    std::vector<float> window;
    int framesPerSample;
    int framesBetweenSamples;
    float threshhold;
};


//==============================================================================
struct FileReader
{
    FileReader (juce::AudioFormatReader* r) : reader (r)
    {}

    double getSampleRate()      { return reader->sampleRate; }
    uint64_t getFrameCount()    { return static_cast<uint64_t> (reader->lengthInSamples); }
    uint32_t getChannelCount()  { return reader->numChannels; }

    bool readFrames (uint64_t frame, choc::buffer::ChannelArrayView<float> buffer)
    {
        float* offsets[buffer.getNumChannels()];

        for (uint32_t i = 0; i < buffer.getNumChannels(); i++)
            offsets[i] = buffer.data.channels[i] + buffer.data.offset;

        return reader->read (offsets, (int) buffer.getNumChannels(), (juce::int64) frame, (int) buffer.getNumFrames());
    }

    std::unique_ptr<juce::AudioFormatReader> reader;
};


std::unique_ptr<FileReader> readAudioFile (const juce::File& file)
{
    juce::AudioFormatManager audioFormatManager;
    audioFormatManager.registerBasicFormats();

    if (! file.exists())
        return {};

    auto* reader = audioFormatManager.createReaderFor (file);

    return std::make_unique<FileReader> (reader);
}

float midiNoteToFrequency (float note)
{
    return (440.0f * pow (2.0f, (note - 69.0f) * (1.0f / 12.0f)));
}

float fromDb (float db)
{
    return pow (10.0f, db / 20.0f);
}

void showHelp()
{
    std::cerr << R"(
    analyse file.wav [--frequency=n] [--harmonics=n] [--framesPerSample=n] [--framesBetweenSamples=n] [--template=filename]

    --frequency=n               Specify the frequenqcy of the note being analysed
    --harmonics=n               How many harmonics to generate - defaults to 64
    --framesPerSample=n         How many frames to analyse to determine the harmonic amplitudes - defaults to 1024
    --framesBetweenSamples=n    How often to analyse the harmonics - defaults to 1024 frames
    --template=filename         Specifies the template to be used for the generated code
    --threshhold=n              Minimum amplitude that is considered relevant above the noise floor, specified in db

)";
}

void reportFailure (const char* msg)
{
    std::cerr << msg << std::endl;

    showHelp();
    exit (-1);
}

//==============================================================================
int main (int argc, char* argv[])
{
    juce::ArgumentList args (argc, argv);

    if (! args.containsOption("--template"))
        reportFailure ("template option not specified");

    if (! args.containsOption ("--frequency"))
        reportFailure ("frequency option not specified");

    auto soulTemplate = juce::File (args.getValueForOption ("--template").toStdString());
    float noteFrequency = args.getValueForOption ("--frequency").getFloatValue();

    int framesPerSample = 1024;
    int framesBetweenSamples = 1024;
    float threshhold = 0;
    int harmonicsToGenerate = 16;

    if (args.containsOption ("--framesPerSample"))
        framesPerSample = args.getValueForOption ("--framesPerSample").getIntValue();

    if (args.containsOption ("--framesBetweenSamples"))
        framesBetweenSamples = args.getValueForOption ("--framesBetweenSamples").getIntValue();

    if (args.containsOption ("--threshhold"))
        threshhold = fromDb (args.getValueForOption ("--threshhold").getIntValue());

    if (args.containsOption ("--harmonics"))
        harmonicsToGenerate = args.getValueForOption ("--harmonics").getIntValue();

    auto file = args.arguments.begin()->resolveAsExistingFile();

    if (auto reader = readAudioFile (file))
    {
        choc::buffer::ChannelArrayBuffer<float> sourceBuffer (reader->getChannelCount(),
                                                              static_cast<uint32_t> (reader->getFrameCount()));

        reader->readFrames (0, sourceBuffer);

        std::cerr << "sample rate: " << reader->getSampleRate() << std::endl;
        std::cerr << "channels: " << reader->getChannelCount() << std::endl;
        std::cerr << "frames:" << reader->getFrameCount() << std::endl;
        std::cerr << "Note frequency: " << noteFrequency << std::endl;

        EnvelopeFromSample envelopeFromSample (sourceBuffer, reader->getSampleRate(), framesPerSample, framesBetweenSamples, threshhold);

        std::vector<std::vector<float>> harmonics;

        for (int i = 1; i <= harmonicsToGenerate; i++)
            harmonics.push_back (envelopeFromSample.getEnvelope (noteFrequency * i));

        int timeSlots = int (harmonics.front().size());

        std::ostringstream amplitudes;

        for (int i = 0; i < timeSlots; i++)
        {
            bool lastSlot = (i == (timeSlots-1));

            amplitudes << "(";

            for (int j = 0; j < harmonicsToGenerate; j++)
            {
                if (j != 0)
                    amplitudes << ", ";

                if (harmonics[j][i] != 0.0)
                    amplitudes << harmonics[j][i] << "f";
                else
                    amplitudes << "0.0f";
            }

            amplitudes << (lastSlot ? ")" : "),") << std::endl;
        }

        auto t = soulTemplate.loadFileAsString().toStdString();

        t = choc::text::replace (t, "${HARMONICS}", std::to_string (harmonicsToGenerate),
                                    "${AMPLITUDES}", amplitudes.str(),
                                    "${FRAMES_BETWEEN_SAMPLES}", std::to_string (framesBetweenSamples),
                                    "${FREQUENCY}", std::to_string (noteFrequency));

        std::cout << t << std::endl;
    }

    return 0;
}
