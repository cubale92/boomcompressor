// Offline checks: flat band sum, every preset stable, and a faceplate snapshot.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <iostream>

static double rms (const juce::AudioBuffer<float>& b, int start)
{
    double s = 0; int n = 0;
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = start; i < b.getNumSamples(); ++i) { s += b.getSample (ch, i) * b.getSample (ch, i); ++n; }
    return std::sqrt (s / n);
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const double fs = 48000.0; const int block = 512;
    int failures = 0;

    // 1) Init preset: band split must sum flat (sine sweep, magnitude within 0.1 dB)
    {
        Boom8AudioProcessor p; p.setCurrentProgram (0);
        p.prepareToPlay (fs, block);
        float worst = 0.0f;
        for (float f : { 40.f, 90.f, 120.f, 300.f, 800.f, 2000.f, 5000.f, 9000.f, 15000.f })
        {
            juce::AudioBuffer<float> buf (2, (int) fs); juce::MidiBuffer m;
            for (int i = 0; i < buf.getNumSamples(); ++i)
                buf.setSample (0, i, 0.25f * std::sin (juce::MathConstants<float>::twoPi * f * i / (float) fs)), buf.setSample (1, i, buf.getSample (0, i));
            const double inR = rms (buf, (int) fs / 2);
            for (int s = 0; s < buf.getNumSamples(); s += block)
            {
                juce::AudioBuffer<float> v (buf.getArrayOfWritePointers(), 2, s, juce::jmin (block, buf.getNumSamples() - s));
                p.processBlock (v, m);
            }
            const float dev = (float) juce::Decibels::gainToDecibels (rms (buf, (int) fs / 2) / inR);
            worst = juce::jmax (worst, std::abs (dev));
        }
        std::cout << "Init flatness: worst deviation " << worst << " dB\n";
        if (worst > 0.1f) ++failures;
    }

    // 2) Every preset on a drum-ish loop: finite, sane level, reports gain reduction
    for (int pi = 0; pi < Boom8AudioProcessor().getNumPrograms(); ++pi)
    {
        Boom8AudioProcessor p; p.setCurrentProgram (pi);
        p.prepareToPlay (fs, block);
        juce::Random rng (1);
        juce::AudioBuffer<float> buf (2, (int) fs * 2); juce::MidiBuffer m;
        for (int i = 0; i < buf.getNumSamples(); ++i)
        {
            const int beat = i % (int) (fs * 0.333);      // ~90 bpm 8ths
            const float t = (float) beat / (float) fs;
            const float kick  = 0.8f * std::sin (juce::MathConstants<float>::twoPi * (50.0f + 80.0f * std::exp (-t * 40.f)) * t) * std::exp (-t * 9.0f);
            const float snare = ((i / (int) (fs * 0.333)) % 2 ? 0.5f : 0.0f) * (rng.nextFloat() * 2 - 1) * std::exp (-t * 25.0f);
            const float hat   = 0.15f * (rng.nextFloat() * 2 - 1) * std::exp (-std::fmod (t, 0.083f) * 120.0f);
            const float s = kick + snare + hat;
            buf.setSample (0, i, s); buf.setSample (1, i, s);
        }
        const double inR = rms (buf, 0);
        float maxGr = 0.0f; bool finite = true;
        for (int s = 0; s < buf.getNumSamples(); s += block)
        {
            juce::AudioBuffer<float> v (buf.getArrayOfWritePointers(), 2, s, juce::jmin (block, buf.getNumSamples() - s));
            p.processBlock (v, m);
            for (auto& g : p.grMeters) maxGr = juce::jmax (maxGr, g.load());
        }
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < buf.getNumSamples(); ++i) finite &= std::isfinite (buf.getSample (ch, i));
        const float peak = buf.getMagnitude (0, buf.getNumSamples());
        const float lvl = (float) juce::Decibels::gainToDecibels (rms (buf, 0) / inR);
        std::cout << juce::String (p.getProgramName (pi)).paddedRight (' ', 20) << " level " << juce::String (lvl, 1)
                  << " dB, peak " << juce::String (peak, 2) << ", max GR " << juce::String (maxGr, 1) << " dB" << (finite ? "" : "  NON-FINITE!") << "\n";
        if (! finite || peak > 4.0f || std::abs (lvl) > 12.0f) ++failures;
    }

    // 3) Faceplate snapshot
    if (argc > 1)
    {
        Boom8AudioProcessor p; p.setCurrentProgram (1);
        p.prepareToPlay (fs, block);
        for (int b = 0; b < 4; ++b) p.grMeters[(size_t) b].store (3.0f + 2.5f * b);
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        ed->setVisible (true);
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File out { juce::String (argv[1]) };
        out.deleteFile();
        juce::FileOutputStream os (out);
        juce::PNGImageFormat().writeImageToStream (img, os);
        std::cout << "snapshot: " << out.getFullPathName() << "\n";
        ed.reset();
    }

    std::cout << (failures == 0 ? "ALL PASS\n" : "FAILURES: " + std::to_string (failures) + "\n");
    return failures;
}
