#pragma once

#include "LabWidgets.h"
#include "dev/diagnostics/PresetValidator.h"

namespace am::dev
{

/**
    PRESET VALIDATOR VIEW (§86)

    Drives the background PresetValidator and lists what it found: parameter
    bounds, JSON round trip, and the level / non-finite / safety / CPU figures
    of an offline render of every factory preset through the validator's own
    engine. Selecting a row shows the issues it raised.
*/
class PresetValidatorView : public LabView
{
public:
    PresetValidatorView();
    ~PresetValidatorView() override;

    void updateFrame (const LabFrame& f) override;
    void resized() override;

    /** Starts validating every factory preset on the background thread. */
    void startValidation();
    bool isValidating() const { return validator.busy(); }

private:
    void rebuildTable();
    void refreshDetail();

    PresetValidator validator;
    std::vector<PresetValidationResult> results;

    LabPanel controls { "Preset validator  (§86)" };
    juce::TextButton startButton { "VALIDATE ALL" }, cancelButton { "CANCEL" };
    juce::Slider holdSeconds;
    juce::Label holdLabel;
    juce::ProgressBar progressBar;
    double progress = 0.0;

    LabPanel tablePanel { "Results" };
    LabTable table;
    KeyValueTable detail { "Selected preset" };

    int selectedIndex = -1;
    size_t lastResultCount = 0;
    bool wasBusy = false;
};

} // namespace am::dev
