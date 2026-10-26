#pragma once

#include "PluginProcessor.h"

//==============================================================================
class Boom8LookAndFeel : public juce::LookAndFeel_V4
{
public:
    Boom8LookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;

    static const juce::Colour face, faceDark, ink, strip, accent, cream;
};

//==============================================================================
// Gain-reduction meter, 0..24 dB, VU-style decay.
class GrMeter : public juce::Component, private juce::Timer
{
public:
    explicit GrMeter (std::atomic<float>& src) : source (src) { startTimerHz (30); }
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    std::atomic<float>& source;
    float shown = 0.0f;
};

//==============================================================================
class Boom8AudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit Boom8AudioProcessorEditor (Boom8AudioProcessor&);
    ~Boom8AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    void setupKnob (Knob&, const juce::String& paramId, const juce::String& text);
    void placeKnob (Knob&, juce::Rectangle<int> area);
    void refreshPresetBox();

    Boom8AudioProcessor& processor;
    Boom8LookAndFeel lnf;

    juce::ComboBox presetBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" };

    std::array<Knob, 3> splits;
    struct BandUI
    {
        Knob thr, ratio, att, rel, gain;
        juce::TextButton solo { "SOLO" }, byp { "BYPASS" };
        std::unique_ptr<ButtonAttachment> soloAtt, bypAtt;
        std::unique_ptr<GrMeter> meter;
    };
    std::array<BandUI, Boom8AudioProcessor::numBands> bands;

    Knob drive, grit, mix, output;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Boom8AudioProcessorEditor)
};
