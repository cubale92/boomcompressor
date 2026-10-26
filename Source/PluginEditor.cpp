#include "PluginEditor.h"

const juce::Colour Boom8LookAndFeel::face     { 0xffe6dcc4 }; // aged cream faceplate
const juce::Colour Boom8LookAndFeel::faceDark { 0xffd3c6a8 };
const juce::Colour Boom8LookAndFeel::ink      { 0xff2a211b }; // dark brown print
const juce::Colour Boom8LookAndFeel::strip    { 0xff2e2520 }; // header / trim
const juce::Colour Boom8LookAndFeel::accent   { 0xffc8502a }; // burnt orange
const juce::Colour Boom8LookAndFeel::cream    { 0xfff3ead6 };

namespace
{
    constexpr int W = 1000, H = 560;
    constexpr int colX = 20, colW = 175, colGap = 10;
    const char* bandTitles[] = { "LOW", "LOW-MID", "HI-MID", "HIGH" };
    const char* bandHints[]  = { "kick / sub", "bass / body", "snare / horns", "hats / air" };

    juce::Font plateFont (float h, bool bold = true)
    {
        return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain)).withExtraKerningFactor (0.08f);
    }
}

//==============================================================================
Boom8LookAndFeel::Boom8LookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, ink);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, ink);
    setColour (juce::ComboBox::textColourId, cream);
    setColour (juce::ComboBox::arrowColourId, accent);
    setColour (juce::PopupMenu::backgroundColourId, strip);
    setColour (juce::PopupMenu::textColourId, cream);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accent);
    setColour (juce::PopupMenu::highlightedTextColourId, cream);
    setColour (juce::TextButton::textColourOffId, ink);
    setColour (juce::TextButton::textColourOnId, cream);
}

void Boom8LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);

    // Scale ticks printed on the faceplate
    g.setColour (slider.findColour (juce::Slider::textBoxTextColourId).withAlpha (0.7f));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
        const float r1 = radius * 0.92f, r2 = radius * (i % 5 == 0 ? 1.0f : 0.97f);
        g.drawLine (c.x + r1 * std::sin (a), c.y - r1 * std::cos (a),
                    c.x + r2 * std::sin (a), c.y - r2 * std::cos (a), i % 5 == 0 ? 1.6f : 1.0f);
    }

    // Skirt
    const float skirt = radius * 0.82f;
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillEllipse (c.x - skirt + 1.5f, c.y - skirt + 2.5f, skirt * 2, skirt * 2);
    juce::ColourGradient sg (juce::Colour (0xff3b3b3b), c.x, c.y - skirt, juce::Colour (0xff0d0d0d), c.x, c.y + skirt, false);
    g.setGradientFill (sg);
    g.fillEllipse (c.x - skirt, c.y - skirt, skirt * 2, skirt * 2);

    // Knurled ring
    g.setColour (juce::Colour (0xff4a4a4a));
    for (int i = 0; i < 36; ++i)
    {
        const float a = angle + (float) i * juce::MathConstants<float>::twoPi / 36.0f;
        g.drawLine (c.x + skirt * 0.86f * std::sin (a), c.y - skirt * 0.86f * std::cos (a),
                    c.x + skirt * 0.98f * std::sin (a), c.y - skirt * 0.98f * std::cos (a), 1.0f);
    }

    // Cap
    const float cap = skirt * 0.68f;
    juce::ColourGradient cg (juce::Colour (0xff2c2c2c), c.x - cap, c.y - cap, juce::Colour (0xff111111), c.x + cap, c.y + cap, false);
    g.setGradientFill (cg);
    g.fillEllipse (c.x - cap, c.y - cap, cap * 2, cap * 2);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawEllipse (c.x - cap, c.y - cap, cap * 2, cap * 2, 1.0f);

    // Pointer line
    juce::Path p;
    p.addRoundedRectangle (-1.6f, -skirt * 0.98f, 3.2f, skirt * 0.62f, 1.2f);
    g.setColour (cream);
    g.fillPath (p, juce::AffineTransform::rotation (angle).translated (c));
}

void Boom8LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    g.setColour (on ? accent : (over ? faceDark.darker (0.08f) : faceDark));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (ink.withAlpha (down ? 0.9f : 0.6f));
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
}

void Boom8LookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (juce::Colour (0xff17120f));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (accent.withAlpha (0.6f));
    g.drawRoundedRectangle (r, 3.0f, 1.0f);

    juce::Path tri;
    const float ax = (float) w - 16.0f, ay = (float) h * 0.5f;
    tri.addTriangle (ax - 5, ay - 2.5f, ax + 5, ay - 2.5f, ax, ay + 3.5f);
    g.setColour (accent);
    g.fillPath (tri);
}

juce::Font Boom8LookAndFeel::getComboBoxFont (juce::ComboBox&) { return plateFont (15.0f); }
juce::Font Boom8LookAndFeel::getTextButtonFont (juce::TextButton&, int) { return plateFont (11.0f); }

juce::Label* Boom8LookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (plateFont (12.0f, false));
    l->setJustificationType (juce::Justification::centred);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::outlineWhenEditingColourId, accent);
    l->setColour (juce::Label::backgroundWhenEditingColourId, cream);
    return l;
}

//==============================================================================
void GrMeter::timerCallback()
{
    const float target = source.load();
    shown = target > shown ? target : shown * 0.85f + target * 0.15f;
    repaint();
}

void GrMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff17120f));
    g.fillRoundedRectangle (r, 2.0f);

    const auto inner = r.reduced (2.0f);
    const float frac = juce::jlimit (0.0f, 1.0f, shown / 24.0f);
    // Reduction grows from the right edge, like a needle falling from 0
    g.setColour (Boom8LookAndFeel::accent);
    g.fillRect (inner.withLeft (inner.getRight() - inner.getWidth() * frac));

    g.setColour (Boom8LookAndFeel::cream.withAlpha (0.35f));
    for (int db : { 3, 6, 12, 18 })
    {
        const float xx = inner.getRight() - inner.getWidth() * (float) db / 24.0f;
        g.drawVerticalLine ((int) xx, inner.getY(), inner.getBottom());
    }
}

//==============================================================================
Boom8AudioProcessorEditor::Boom8AudioProcessorEditor (Boom8AudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lnf);

    // Presets
    addAndMakeVisible (presetBox);
    refreshPresetBox();
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0 && idx != processor.getCurrentProgram())
            processor.setCurrentProgram (idx);
    };
    auto step = [this] (int delta)
    {
        const int n = processor.getNumPrograms();
        processor.setCurrentProgram ((processor.getCurrentProgram() + delta + n) % n);
        presetBox.setSelectedId (processor.getCurrentProgram() + 1, juce::dontSendNotification);
    };
    prevButton.onClick = [step] { step (-1); };
    nextButton.onClick = [step] { step (1); };
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);
    processor.onPresetChanged = [this] { presetBox.setSelectedId (processor.getCurrentProgram() + 1, juce::dontSendNotification); };

    // Crossovers
    setupKnob (splits[0], "xo1", "SPLIT");
    setupKnob (splits[1], "xo2", "SPLIT");
    setupKnob (splits[2], "xo3", "SPLIT");

    for (int b = 0; b < Boom8AudioProcessor::numBands; ++b)
    {
        auto& ui = bands[(size_t) b];
        const juce::String pre = "b" + juce::String (b + 1) + "_";
        setupKnob (ui.thr,   pre + "thr",   "THRESHOLD");
        setupKnob (ui.ratio, pre + "ratio", "RATIO");
        setupKnob (ui.att,   pre + "att",   "ATTACK");
        setupKnob (ui.rel,   pre + "rel",   "RELEASE");
        setupKnob (ui.gain,  pre + "gain",  "GAIN");

        for (auto* btn : { &ui.solo, &ui.byp })
        {
            btn->setClickingTogglesState (true);
            addAndMakeVisible (*btn);
        }
        ui.soloAtt = std::make_unique<ButtonAttachment> (processor.apvts, pre + "solo", ui.solo);
        ui.bypAtt  = std::make_unique<ButtonAttachment> (processor.apvts, pre + "byp",  ui.byp);

        ui.meter = std::make_unique<GrMeter> (processor.grMeters[(size_t) b]);
        addAndMakeVisible (*ui.meter);
    }

    setupKnob (drive,  "drive",  "DRIVE");
    setupKnob (grit,   "grit",   "GRIT");
    setupKnob (mix,    "mix",    "MIX");
    setupKnob (output, "output", "OUTPUT");

    setSize (W, H);
}

Boom8AudioProcessorEditor::~Boom8AudioProcessorEditor()
{
    processor.onPresetChanged = nullptr;
    setLookAndFeel (nullptr);
}

void Boom8AudioProcessorEditor::refreshPresetBox()
{
    presetBox.clear (juce::dontSendNotification);
    for (int i = 0; i < processor.getNumPrograms(); ++i)
        presetBox.addItem (processor.getProgramName (i), i + 1);
    presetBox.setSelectedId (processor.getCurrentProgram() + 1, juce::dontSendNotification);
}

void Boom8AudioProcessorEditor::setupKnob (Knob& k, const juce::String& paramId, const juce::String& text)
{
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
    k.slider.setPopupDisplayEnabled (false, false, nullptr);
    k.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    k.slider.setColour (juce::Slider::textBoxTextColourId, Boom8LookAndFeel::ink);
    addAndMakeVisible (k.slider);
    k.attachment = std::make_unique<SliderAttachment> (processor.apvts, paramId, k.slider);

    k.label.setText (text, juce::dontSendNotification);
    k.label.setFont (plateFont (10.5f));
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (k.label);
}

void Boom8AudioProcessorEditor::placeKnob (Knob& k, juce::Rectangle<int> area)
{
    k.label.setBounds (area.removeFromTop (14));
    k.slider.setBounds (area);
}

//==============================================================================
void Boom8AudioProcessorEditor::paint (juce::Graphics& g)
{
    using L = Boom8LookAndFeel;

    // Faceplate with a little grain
    g.fillAll (L::face);
    juce::Random rng (8);
    for (int i = 0; i < 2500; ++i)
    {
        g.setColour (L::ink.withAlpha (rng.nextFloat() * 0.05f));
        g.fillRect (rng.nextInt (W), rng.nextInt (H), 1, 1);
    }

    // Header strip
    g.setColour (L::strip);
    g.fillRect (0, 0, W, 56);
    g.setColour (L::accent);
    g.fillRect (0, 56, W, 3);

    g.setColour (L::cream);
    g.setFont (plateFont (26.0f));
    g.drawText ("8cee", 22, 8, 90, 40, juce::Justification::centredLeft);
    g.setColour (L::accent);
    g.setFont (plateFont (22.0f));
    g.drawText ("BOOM-8", 92, 8, 120, 40, juce::Justification::centredLeft);
    g.setColour (L::cream.withAlpha (0.6f));
    g.setFont (plateFont (10.5f, false));
    g.drawText ("MULTIBAND DYNAMICS", 214, 8, 160, 40, juce::Justification::centredLeft);

    g.setColour (L::cream.withAlpha (0.55f));
    g.drawText ("PRESET", 548, 8, 60, 40, juce::Justification::centredRight);

    // Crossover scale line connecting the band sections
    g.setColour (L::ink.withAlpha (0.5f));
    g.drawHorizontalLine (102, (float) colX, (float) (colX + 4 * colW + 3 * colGap));
    g.setFont (plateFont (10.0f));
    g.setColour (L::ink.withAlpha (0.55f));
    g.drawText ("CROSSOVER", colX, 64, 120, 14, juce::Justification::centredLeft);

    // Band sections
    for (int b = 0; b < Boom8AudioProcessor::numBands; ++b)
    {
        const auto r = juce::Rectangle<int> (colX + b * (colW + colGap), 146, colW, 396).toFloat();
        g.setColour (L::faceDark);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (L::ink.withAlpha (0.35f));
        g.drawRoundedRectangle (r, 6.0f, 1.0f);

        auto plate = r.withHeight (30.0f).reduced (8.0f, 4.0f);
        g.setColour (L::strip);
        g.fillRoundedRectangle (plate, 3.0f);
        g.setColour (L::cream);
        g.setFont (plateFont (13.0f));
        g.drawText (bandTitles[b], plate.reduced (8, 0), juce::Justification::centredLeft);
        g.setColour (L::accent.brighter (0.2f));
        g.setFont (plateFont (10.0f, false));
        g.drawText (bandHints[b], plate.reduced (8, 0), juce::Justification::centredRight);

        g.setColour (L::ink.withAlpha (0.6f));
        g.setFont (plateFont (8.5f));
        g.drawText ("GR  24 . . . . 0 dB", (int) r.getX() + 12, 182, colW - 24, 10, juce::Justification::centred);
    }

    // Character section
    const auto cr = juce::Rectangle<int> (770, 146, 210, 396).toFloat();
    g.setColour (L::strip);
    g.fillRoundedRectangle (cr, 6.0f);
    auto plate = cr.withHeight (30.0f).reduced (8.0f, 4.0f);
    g.setColour (L::accent);
    g.setFont (plateFont (13.0f));
    g.drawText ("CHARACTER", plate, juce::Justification::centred);

    g.setColour (L::cream.withAlpha (0.5f));
    g.setFont (plateFont (9.5f, false));
    juce::StringArray notes { "DRIVE   warm asymmetric saturation",
                              "GRIT    12-bit / 26 kHz sampler crunch",
                              "MIX     parallel blend (phase-aligned)",
                              "OUTPUT  final level" };
    for (int i = 0; i < notes.size(); ++i)
        g.drawText (notes[i], (int) cr.getX() + 14, 462 + i * 17, (int) cr.getWidth() - 20, 14, juce::Justification::centredLeft);

    // Screws
    for (auto pt : { juce::Point<float> (10, 70), juce::Point<float> (W - 10.0f, 70), juce::Point<float> (10, H - 10.0f), juce::Point<float> (W - 10.0f, H - 10.0f) })
    {
        g.setColour (juce::Colour (0xff9a9080));
        g.fillEllipse (pt.x - 5, pt.y - 5, 10, 10);
        g.setColour (L::ink.withAlpha (0.6f));
        g.drawLine (pt.x - 3.5f, pt.y - 3.5f, pt.x + 3.5f, pt.y + 3.5f, 1.2f);
    }
}

void Boom8AudioProcessorEditor::resized()
{
    prevButton.setBounds (616, 15, 28, 28);
    presetBox.setBounds  (648, 15, 280, 28);
    nextButton.setBounds (932, 15, 28, 28);

    // Split knobs sit on the boundaries between bands
    for (int i = 0; i < 3; ++i)
    {
        const int cx = colX + (i + 1) * (colW + colGap) - colGap / 2;
        placeKnob (splits[(size_t) i], { cx - 40, 64, 80, 80 });
        splits[(size_t) i].label.setText (i == 0 ? "LOW | MID" : i == 1 ? "MID | HI" : "HI | AIR", juce::dontSendNotification);
    }

    for (int b = 0; b < Boom8AudioProcessor::numBands; ++b)
    {
        auto& ui = bands[(size_t) b];
        const int x = colX + b * (colW + colGap);

        ui.meter->setBounds (x + 12, 194, colW - 24, 12);
        placeKnob (ui.thr, { x + 25, 212, colW - 50, 116 });

        const int half = colW / 2;
        placeKnob (ui.ratio, { x + 4,        332, half - 4, 86 });
        placeKnob (ui.att,   { x + half,     332, half - 4, 86 });
        placeKnob (ui.rel,   { x + 4,        420, half - 4, 86 });
        placeKnob (ui.gain,  { x + half,     420, half - 4, 86 });

        ui.solo.setBounds (x + 12,   508, 70, 24);
        ui.byp.setBounds  (x + 93,   508, 70, 24);
    }

    // Character section knobs
    placeKnob (drive,  { 778, 184, 97, 112 });
    placeKnob (grit,   { 877, 184, 97, 112 });
    placeKnob (mix,    { 778, 320, 97, 112 });
    placeKnob (output, { 877, 320, 97, 112 });

    for (auto* k : { &drive, &grit, &mix, &output })
    {
        k->label.setColour (juce::Label::textColourId, Boom8LookAndFeel::cream);
        k->slider.setColour (juce::Slider::textBoxTextColourId, Boom8LookAndFeel::cream);
    }
}
