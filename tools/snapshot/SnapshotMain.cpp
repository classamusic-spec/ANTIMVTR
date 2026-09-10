/*
    AntiMatrSnapshot — renders the real editor to a PNG for visual review.

    Usage (run under xvfb-run on headless Linux):
      AntiMatrSnapshot --out shot.png [--width 1600 --height 1000] [--wait 800] [--set id=value ...] [--labtab N]
                       [--sample path|builtin:N] [--analyze]
                       [--page 0..7] [--modtab N] [--note 60] [--preset "Void Bloom"]
                       [--mod "lfo1>shape.decay:0.5" ...]

    --mod adds a modulation routing before the editor opens, so the knob
    modulation rings and the ROUTINGS panel can be captured with real state.
    Same syntax as AntiMatrRender: "<source>><target>:<depth>[:uni|:bi][:curve]".

    The processor runs offline on the message thread so taps, meters and the
    visualizer show real engine state.
*/

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "plugin/AntiMatrProcessor.h"
#include "plugin/AntiMatrEditor.h"
#include "dev/dsplab/DSPLabView.h"
#include "ui/views/Pages.h"
#include "state/ModRouting.h"

namespace
{
    /** Accepts both "--name value" and "--name=value". */
    juce::String optionValue (const juce::ArgumentList& args, const juce::String& name, const juce::String& fallback = {})
    {
        for (int i = 0; i < args.size(); ++i)
        {
            const auto& t = args[i].text;
            if (t == name) return i + 1 < args.size() ? args[i + 1].text : fallback;
            if (t.startsWith (name + "=")) return t.fromFirstOccurrenceOf ("=", false, false);
        }
        return fallback;
    }

    bool hasOption (const juce::ArgumentList& args, const juce::String& name)
    {
        for (int i = 0; i < args.size(); ++i)
            if (args[i].text == name || args[i].text.startsWith (name + "=")) return true;
        return false;
    }

    /** Parses one --mod "src>target:depth[:uni|:bi][:curve]" specification. */
    bool parseModRouting (const juce::String& spec, am::ModRouting& out)
    {
        const auto sourceId = spec.upToFirstOccurrenceOf (">", false, false).trim();
        const auto rest = spec.fromFirstOccurrenceOf (">", false, false);
        juce::StringArray parts;
        parts.addTokens (rest, ":", "");
        if (sourceId.isEmpty() || parts.size() < 2) return false;

        out.source = am::modSourceFromId (sourceId.toRawUTF8());
        const auto target = am::ParameterRegistry::fromID (parts[0].trim().toStdString());
        if (out.source == am::ModSource::None || ! target.has_value()) return false;
        out.target = *target;
        out.depth = parts[1].getFloatValue();
        out.bipolar = true;
        out.curve = 0.0f;
        for (int i = 2; i < parts.size(); ++i)
        {
            const auto option = parts[i].trim().toLowerCase();
            if (option == "uni") out.bipolar = false;
            else if (option == "bi") out.bipolar = true;
            else out.curve = option.getFloatValue();
        }
        return am::ModRoutingTable::isValid (out);
    }
}

class SnapshotApp : public juce::JUCEApplication, private juce::Timer
{
public:
    const juce::String getApplicationName() override { return "AntiMatrSnapshot"; }
    const juce::String getApplicationVersion() override { return ANTIMATR_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise (const juce::String& commandLine) override
    {
        juce::ArgumentList args ("AntiMatrSnapshot", commandLine);
        outFile = hasOption (args, "--out") ? optionValue (args, "--out") : juce::String ("antimatr.png");
        width  = hasOption (args, "--width") ? optionValue (args, "--width").getIntValue() : 1600;
        height = hasOption (args, "--height") ? optionValue (args, "--height").getIntValue() : 1000;
        waitMs = hasOption (args, "--wait") ? optionValue (args, "--wait").getIntValue() : 800;
        page   = hasOption (args, "--page") ? optionValue (args, "--page").getIntValue() : 0;
        note   = hasOption (args, "--note") ? optionValue (args, "--note").getIntValue() : 60;

        processor = std::make_unique<am::AntiMatrProcessor>();
        processor->lastEditorWidth = width;
        processor->lastEditorHeight = height;
        if (hasOption (args, "--preset"))
        {
            const int idx = processor->presets().findFactory (optionValue (args, "--preset"));
            if (idx >= 0) processor->loadFactoryPreset (idx);
        }
        // --set id=value pairs (applied through the host parameter tree)
        for (int i = 0; i < args.size(); ++i)
        {
            if (args[i].text == "--set" && i + 1 < args.size())
            {
                const auto kv = args[i + 1].text;
                const auto id = kv.upToFirstOccurrenceOf ("=", false, false);
                const float value = kv.fromFirstOccurrenceOf ("=", false, false).getFloatValue();
                if (auto* p = processor->parameters().getParameter (id))
                    p->setValueNotifyingHost (p->convertTo0to1 (value));
                else
                    std::cerr << "Unknown parameter: " << id << std::endl;
            }
        }

        // Modulation routings (--mod), published exactly the way the editor does it.
        {
            am::ModRoutingTable routings;
            for (int i = 0; i < args.size(); ++i)
            {
                if (args[i].text != "--mod" || i + 1 >= args.size()) continue;
                am::ModRouting r;
                if (parseModRouting (args[i + 1].text, r) && routings.add (r) >= 0) continue;
                std::cerr << "Bad --mod routing: " << args[i + 1].text << std::endl;
            }
            if (! routings.isEmpty()) processor->setModRoutings (routings);
        }
        // Sample selection and ANALYZE -> MATTER, so the SOURCE page can be reviewed with real state.
        if (hasOption (args, "--sample"))
        {
            const auto spec = optionValue (args, "--sample");
            if (spec.startsWithIgnoreCase ("builtin:")) processor->selectBuiltInSample (spec.fromFirstOccurrenceOf (":", false, false).getIntValue());
            else if (! processor->loadSampleFile (juce::File::getCurrentWorkingDirectory().getChildFile (spec)))
                std::cerr << "Could not load sample: " << spec << std::endl;
        }
        if (hasOption (args, "--analyze") && ! processor->analyzeSampleToMatter())
            std::cerr << "Nothing to analyze" << std::endl;

        processor->prepareToPlay (48000.0, 512);
        if (note > 0) processor->keyboardState().noteOn (1, note, 0.8f);

        editor.reset (processor->createEditorIfNeeded());
        if (auto* e = dynamic_cast<am::AntiMatrEditor*> (editor.get()))
        {
            e->showPage (page);
            if (hasOption (args, "--labtab"))
                if (auto* lab = dynamic_cast<am::dev::DSPLabView*> (e->labViewComponent()))
                    lab->selectTab (optionValue (args, "--labtab").getIntValue());
            if (hasOption (args, "--modtab"))
                if (auto* group = dynamic_cast<am::ui::GroupPage*> (e->pageComponent (page)))
                    if (auto* mod = dynamic_cast<am::ui::ModPage*> (group->designedPage()))
                        mod->showTab (optionValue (args, "--modtab").getIntValue());
        }

        window = std::make_unique<juce::DocumentWindow> ("ANTI-MATR", juce::Colours::black, juce::DocumentWindow::allButtons);
        window->setUsingNativeTitleBar (false);
        window->setContentNonOwned (editor.get(), false);
        window->setContentComponentSize (width, height);
        window->setVisible (true);

        startTimerHz (50);
    }

    void timerCallback() override
    {
        // Feed the engine so meters / taps / visualizer have real signal.
        juce::AudioBuffer<float> buffer (2, 960);
        juce::MidiBuffer midi;
        processor->processBlock (buffer, midi);

        elapsed += 20;
        if (elapsed >= waitMs)
        {
            stopTimer();
            auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
            juce::File f (outFile);
            f.deleteFile();
            juce::FileOutputStream stream (f);
            juce::PNGImageFormat png;
            const bool ok = png.writeImageToStream (image, stream);
            std::cout << (ok ? "Wrote " : "FAILED ") << f.getFullPathName() << " (" << image.getWidth() << "x" << image.getHeight() << ")" << std::endl;
            setApplicationReturnValue (ok ? 0 : 1);
            quit();
        }
    }

    void shutdown() override
    {
        window = nullptr;
        if (editor != nullptr && processor != nullptr) processor->editorBeingDeleted (editor.get());
        editor = nullptr;
        processor = nullptr;
    }

private:
    std::unique_ptr<am::AntiMatrProcessor> processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    std::unique_ptr<juce::DocumentWindow> window;
    juce::String outFile;
    int width = 1600, height = 1000, waitMs = 800, page = 0, note = 60, elapsed = 0;
};

START_JUCE_APPLICATION (SnapshotApp)
