#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

namespace
{
    juce::String bandId (int band, const char* name) { return "b" + juce::String (band + 1) + "_" + name; }

    float getP (juce::AudioProcessorValueTreeState& s, const juce::String& id)
    {
        return s.getRawParameterValue (id)->load();
    }

    void setP (juce::AudioProcessorValueTreeState& s, const juce::String& id, float value)
    {
        if (auto* p = s.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout Boom8AudioProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto freqRange = [] (float lo, float hi) { NormalisableRange<float> r (lo, hi); r.setSkewForCentre (std::sqrt (lo * hi)); return r; };

    auto hz = AudioParameterFloatAttributes().withLabel ("Hz")
                 .withStringFromValueFunction ([] (float v, int) { return v >= 1000.0f ? String (v / 1000.0f, 2) + " k" : String ((int) v); });

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "xo1", 1 }, "Split Low",  freqRange (40.0f, 400.0f),    120.0f,  hz));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "xo2", 1 }, "Split Mid",  freqRange (250.0f, 3000.0f),  800.0f,  hz));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "xo3", 1 }, "Split High", freqRange (2000.0f, 12000.0f), 5000.0f, hz));

    const char* bandNames[] = { "Low", "Low-Mid", "Hi-Mid", "High" };
    auto db = AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction ([] (float v, int) { return String (v, 1); });
    auto ms = AudioParameterFloatAttributes().withLabel ("ms").withStringFromValueFunction ([] (float v, int) { return v < 10.0f ? String (v, 1) : String ((int) v); });
    auto rt = AudioParameterFloatAttributes().withLabel (":1").withStringFromValueFunction ([] (float v, int) { return String (v, 1); });

    for (int b = 0; b < numBands; ++b)
    {
        const String n (bandNames[b]);
        NormalisableRange<float> ratioR (1.0f, 20.0f);  ratioR.setSkewForCentre (4.0f);
        NormalisableRange<float> attR (0.1f, 100.0f);   attR.setSkewForCentre (10.0f);
        NormalisableRange<float> relR (10.0f, 1000.0f); relR.setSkewForCentre (120.0f);

        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { bandId (b, "thr"), 1 },   n + " Threshold", NormalisableRange<float> (-60.0f, 0.0f, 0.1f), 0.0f, db));
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { bandId (b, "ratio"), 1 }, n + " Ratio",     ratioR, 1.0f, rt));
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { bandId (b, "att"), 1 },   n + " Attack",    attR, 10.0f, ms));
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { bandId (b, "rel"), 1 },   n + " Release",   relR, 100.0f, ms));
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { bandId (b, "gain"), 1 },  n + " Gain",      NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, db));
        p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { bandId (b, "solo"), 1 },  n + " Solo",   false));
        p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { bandId (b, "byp"), 1 },   n + " Bypass", false));
    }

    auto pct = AudioParameterFloatAttributes().withLabel ("%").withStringFromValueFunction ([] (float v, int) { return String ((int) v); });
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "drive", 1 },  "Drive",  NormalisableRange<float> (0.0f, 100.0f), 0.0f, pct));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "grit", 1 },   "Grit",   NormalisableRange<float> (0.0f, 100.0f), 0.0f, pct));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 },    "Mix",    NormalisableRange<float> (0.0f, 100.0f), 100.0f, pct));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "output", 1 }, "Output", NormalisableRange<float> (-18.0f, 18.0f, 0.1f), 0.0f, db));

    return { p.begin(), p.end() };
}

//==============================================================================
Boom8AudioProcessor::Boom8AudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Boom8", createLayout())
{
    for (auto& m : grMeters) m.store (0.0f);

    xo1.setType (LR::Type::lowpass);  xo2.setType (LR::Type::lowpass);  xo3.setType (LR::Type::lowpass);
    for (auto* f : { &apLow2, &apLow3, &apMid3, &apDry1, &apDry2, &apDry3 })
        f->setType (LR::Type::allpass);
}

bool Boom8AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void Boom8AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };

    for (auto* f : { &xo1, &xo2, &xo3, &apLow2, &apLow3, &apMid3, &apDry1, &apDry2, &apDry3 })
    {
        f->prepare (spec);
        f->reset();
    }
    updateFilters();

    for (auto& c : comps) c.prepare (sampleRate);

    dcX = {}; dcY = {}; gritHold = {}; gritPhase = 0.0;
    dryBuffer.setSize (2, samplesPerBlock);

    for (auto* s : { &driveSm, &gritSm, &mixSm, &outSm }) s->reset (sampleRate, 0.03);
    for (auto& s : makeupSm) s.reset (sampleRate, 0.03);

    driveSm.setCurrentAndTargetValue (getP (apvts, "drive") * 0.01f);
    gritSm.setCurrentAndTargetValue  (getP (apvts, "grit") * 0.01f);
    mixSm.setCurrentAndTargetValue   (getP (apvts, "mix") * 0.01f);
    outSm.setCurrentAndTargetValue   (juce::Decibels::decibelsToGain (getP (apvts, "output")));
    for (int b = 0; b < numBands; ++b)
        makeupSm[(size_t) b].setCurrentAndTargetValue (juce::Decibels::decibelsToGain (getP (apvts, bandId (b, "gain"))));
}

void Boom8AudioProcessor::updateFilters()
{
    const float nyq = (float) sr * 0.45f;
    float f1 = juce::jmin (getP (apvts, "xo1"), nyq);
    float f2 = juce::jlimit (f1 * 1.25f, nyq, getP (apvts, "xo2"));
    float f3 = juce::jlimit (juce::jmin (f2 * 1.25f, nyq), nyq, getP (apvts, "xo3"));

    xo1.setCutoffFrequency (f1);  apDry1.setCutoffFrequency (f1);
    xo2.setCutoffFrequency (f2);  apLow2.setCutoffFrequency (f2);  apDry2.setCutoffFrequency (f2);
    xo3.setCutoffFrequency (f3);  apLow3.setCutoffFrequency (f3);  apMid3.setCutoffFrequency (f3);  apDry3.setCutoffFrequency (f3);
}

void Boom8AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n = buffer.getNumSamples();
    if (numCh == 0 || n == 0) return;

    if (dryBuffer.getNumSamples() < n) dryBuffer.setSize (2, n, false, false, true);

    updateFilters();

    bool anySolo = false;
    std::array<bool, numBands> solo {}, byp {};
    for (int b = 0; b < numBands; ++b)
    {
        auto& c = comps[(size_t) b];
        c.setParams (getP (apvts, bandId (b, "thr")), getP (apvts, bandId (b, "ratio")),
                     getP (apvts, bandId (b, "att")), getP (apvts, bandId (b, "rel")));
        solo[(size_t) b] = getP (apvts, bandId (b, "solo")) > 0.5f;
        byp[(size_t) b]  = getP (apvts, bandId (b, "byp"))  > 0.5f;
        anySolo |= solo[(size_t) b];
        makeupSm[(size_t) b].setTargetValue (juce::Decibels::decibelsToGain (getP (apvts, bandId (b, "gain"))));
    }

    driveSm.setTargetValue (getP (apvts, "drive") * 0.01f);
    gritSm.setTargetValue  (getP (apvts, "grit") * 0.01f);
    mixSm.setTargetValue   (getP (apvts, "mix") * 0.01f);
    outSm.setTargetValue   (juce::Decibels::decibelsToGain (getP (apvts, "output")));

    // Grit: sample-and-hold at the classic 26.04 kHz sampler rate + 12-bit quantise
    const double holdStep = 26040.0 / sr;
    const float  q = 2048.0f; // 12-bit (signed)
    const float  dcR = 1.0f - (float) (juce::MathConstants<double>::twoPi * 10.0 / sr);

    std::array<float, numBands> grPeak {};
    float outPeak = 0.0f;

    auto* L = buffer.getWritePointer (0);
    auto* R = numCh > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < n; ++i)
    {
        const float drive = driveSm.getNextValue();
        const float grit  = gritSm.getNextValue();
        const float mix   = mixSm.getNextValue();
        const float out   = outSm.getNextValue();
        std::array<float, numBands> mk;
        for (int b = 0; b < numBands; ++b) mk[(size_t) b] = makeupSm[(size_t) b].getNextValue();

        float in[2] = { L[i], R ? R[i] : L[i] };
        float dry[2], bands[numBands][2];

        for (int ch = 0; ch < numCh; ++ch)
        {
            float x = in[ch];

            // Dry path, phase-matched to the band sum
            dry[ch] = apDry3.processSample (ch, apDry2.processSample (ch, apDry1.processSample (ch, x)));

            // Drive: warm asymmetric saturation (2nd-harmonic lean), unity at low levels
            if (drive > 0.0005f)
            {
                const float k = 1.0f + drive * 4.0f;
                float s = x + 0.25f * drive * x * x;
                s = std::tanh (k * s) / k;
                // DC blocker for the asymmetric term
                const float y = s - dcX[(size_t) ch] + dcR * dcY[(size_t) ch];
                dcX[(size_t) ch] = s; dcY[(size_t) ch] = y;
                x = x + (y * (1.0f + drive * 0.5f) - x) * juce::jmin (1.0f, drive * 2.0f);
            }

            // 4-band split (Linkwitz-Riley 24 dB/oct) with all-pass compensation
            float low, rest, mid, rest2, hiMid, high;
            xo1.processSample (ch, x, low, rest);
            xo2.processSample (ch, rest, mid, rest2);
            xo3.processSample (ch, rest2, hiMid, high);

            bands[0][ch] = apLow3.processSample (ch, apLow2.processSample (ch, low));
            bands[1][ch] = apMid3.processSample (ch, mid);
            bands[2][ch] = hiMid;
            bands[3][ch] = high;
        }

        if (numCh == 1)
        {
            dry[1] = dry[0];
            for (auto& b : bands) b[1] = b[0];
        }

        // Compress each band (stereo-linked) and sum
        float wet[2] = { 0.0f, 0.0f };
        for (int b = 0; b < numBands; ++b)
        {
            const auto bi = (size_t) b;
            float g = 1.0f;
            const float det = juce::jmax (std::abs (bands[b][0]), std::abs (bands[b][1]));
            const float cg = comps[bi].process (det);
            if (! byp[bi]) g = cg * mk[bi];
            grPeak[bi] = juce::jmax (grPeak[bi], byp[bi] ? 0.0f : comps[bi].getGainReductionDb());

            if (! anySolo || solo[bi])
            {
                wet[0] += bands[b][0] * g;
                wet[1] += bands[b][1] * g;
            }
        }

        // Grit
        gritPhase += holdStep;
        const bool latch = gritPhase >= 1.0;
        if (latch) gritPhase -= 1.0;

        for (int ch = 0; ch < 2; ++ch)
        {
            float w = wet[ch];
            if (grit > 0.0005f)
            {
                if (latch) gritHold[(size_t) ch] = std::round (juce::jlimit (-1.0f, 1.0f, w) * q) / q;
                w += (gritHold[(size_t) ch] - w) * grit;
            }

            // A soloed band is heard on its own, so the dry signal is muted while soloing.
            const float d = anySolo ? 0.0f : dry[ch];
            wet[ch] = (d + (w - d) * (anySolo ? 1.0f : mix)) * out;
        }

        L[i] = wet[0];
        if (R) R[i] = wet[1];
        outPeak = juce::jmax (outPeak, std::abs (wet[0]), std::abs (wet[1]));
    }

    for (int b = 0; b < numBands; ++b) grMeters[(size_t) b].store (grPeak[(size_t) b]);
    outMeter.store (outPeak);

    for (int ch = numCh; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, n);
}

//==============================================================================
int Boom8AudioProcessor::getNumPrograms() { return (int) getFactoryPresets().size(); }

const juce::String Boom8AudioProcessor::getProgramName (int index)
{
    const auto& ps = getFactoryPresets();
    return juce::isPositiveAndBelow (index, (int) ps.size()) ? juce::String (ps[(size_t) index].name) : juce::String();
}

void Boom8AudioProcessor::setCurrentProgram (int index)
{
    const auto& ps = getFactoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) ps.size())) return;

    const auto& p = ps[(size_t) index];
    currentPreset = index;

    setP (apvts, "xo1", p.xo1);
    setP (apvts, "xo2", p.xo2);
    setP (apvts, "xo3", p.xo3);
    for (int b = 0; b < numBands; ++b)
    {
        const auto& s = p.bands[(size_t) b];
        setP (apvts, bandId (b, "thr"),   s.thr);
        setP (apvts, bandId (b, "ratio"), s.ratio);
        setP (apvts, bandId (b, "att"),   s.att);
        setP (apvts, bandId (b, "rel"),   s.rel);
        setP (apvts, bandId (b, "gain"),  s.gain);
        setP (apvts, bandId (b, "solo"),  0.0f);
        setP (apvts, bandId (b, "byp"),   0.0f);
    }
    setP (apvts, "drive",  p.drive);
    setP (apvts, "grit",   p.grit);
    setP (apvts, "mix",    p.mix);
    setP (apvts, "output", p.output);

    if (onPresetChanged)
        juce::MessageManager::callAsync ([safe = juce::WeakReference<Boom8AudioProcessor> (this)]
        {
            if (safe != nullptr && safe->onPresetChanged) safe->onPresetChanged();
        });
}

//==============================================================================
void Boom8AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void Boom8AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentPreset = tree.getProperty ("preset", 0);
            apvts.replaceState (tree);
        }
}

juce::AudioProcessorEditor* Boom8AudioProcessor::createEditor() { return new Boom8AudioProcessorEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Boom8AudioProcessor(); }
