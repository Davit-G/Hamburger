#include "Theme.h"

#include <SimpleIni.h>

static std::vector<ThemeSection> makeSections()
{
    std::vector<ThemeSection> sections;

    auto section = [&] (juce::String id, juce::String title, juce::String description)
    {
        sections.push_back ({ id, title, description, {} });
    };

    auto addGetter = [&] (juce::String id, juce::String name, juce::String description, std::function<juce::Colour& (Theme&)> in,
                          juce::String type = {}, juce::String family = {})
    {
        sections.back().colours.push_back ({ id, name, description, std::move (in), type, family });
    };

    auto add = [&] (juce::String id, juce::String name, juce::String description, juce::Colour Theme::* member, juce::String family = {})
    {
        addGetter (id, name, description, [member] (Theme& t) -> juce::Colour& { return t.*member; }, {}, family);
    };

    struct AccentPart
    {
        juce::Colour AccentColours::* field;
        juce::String id, name, description;
    };

    const AccentPart knob { &AccentColours::main, "knob", "Knob", "Its main colour, for the big knob and wherever this type is named across the plugin" };
    const AccentPart knobHeld { &AccentColours::mainHeld, "knob_held", "Knob Held", "What the big knob fades to while it is being dragged" };
    const AccentPart slider { &AccentColours::slider, "slider", "Sliders", "The bars of the smaller sliders" };
    const AccentPart sliderHeld { &AccentColours::sliderHeld, "slider_held", "Sliders Held", "What a slider's bar fades to while it is being dragged" };
    const AccentPart icon { &AccentColours::icon, "icon", "Icon", "The icon in the middle of the big knob" };
    const AccentPart text { &AccentColours::text, "text", "Text", "Control names and the box's heading while this type is showing" };

    auto addAccent = [&] (juce::String id, juce::String name, AccentColours Theme::* group, std::initializer_list<AccentPart> parts,
                          juce::String family = {})
    {
        for (const auto& part : parts)
            addGetter (id + "_" + part.id, name + " " + part.name, part.description,
                       [group, field = part.field] (Theme& t) -> juce::Colour& { return (t.*group).*field; }, name, family);
    };

    section ("distortions", "Distortions", "The main, pre and post distortion boxes. Each distortion type has its own set");

    addAccent ("grill", "Grill", &Theme::grill, { knob, knobHeld, slider, sliderHeld, icon, text });
    addAccent ("tube", "Tube", &Theme::tube, { knob, knobHeld, slider, sliderHeld, icon, text });
    addAccent ("phase", "Phase", &Theme::phase, { knob, knobHeld, slider, sliderHeld, icon, text });
    addAccent ("rubidium", "Rubidium", &Theme::rubidium, { knob, knobHeld, slider, sliderHeld, icon, text });
    addAccent ("tape", "Tape", &Theme::tape, { knob, knobHeld, slider, sliderHeld, icon, text });
    addAccent ("slew", "Slew", &Theme::slew, { knob, knobHeld, slider, sliderHeld, icon, text });
    addAccent ("waveshape", "Waveshape", &Theme::waveshape, { knob, slider, sliderHeld, text });
    addAccent ("opto", "Opto", &Theme::opto, { knob, knobHeld, slider, sliderHeld, icon, text });
    addAccent ("start", "Start", &Theme::start, { knob, knobHeld, slider, sliderHeld, text });
    add ("start_drive_ring", "Start Drive Outer Ring", "The outer ring of the start page's drive knob, around a ring for each distortion", &Theme::startDriveRing);

    add ("waveshape_analog", "Waveshape Analog", "Analog shapes on the map and their legend pill", &Theme::waveshapeAnalog);
    add ("waveshape_digital", "Waveshape Digital", "Digital shapes on the map and their legend pill", &Theme::waveshapeDigital);
    add ("waveshape_folding", "Waveshape Folding", "Folding shapes on the map and their legend pill", &Theme::waveshapeFolding);
    add ("waveshape_heavy", "Waveshape Heavy", "Heavy shapes on the map and their legend pill", &Theme::waveshapeHeavy);
    add ("waveshape_ring_fill", "Waveshape Ring Fill", "Inside of the round waveshape map", &Theme::waveshapeRingFill);
    add ("waveshape_ring_outline", "Waveshape Ring Outline", "Edge of the round waveshape map", &Theme::waveshapeRingOutline);
    add ("waveshape_handle", "Waveshape Handle", "The circle you drag around the map", &Theme::waveshapeHandle);
    add ("waveshape_blend_line", "Waveshape Blend Lines", "Lines from the handle to the shapes being blended, fainter for quieter shapes", &Theme::waveshapeBlendLine);
    add ("waveshape_pill_off", "Waveshape Pill Excluded", "A legend pill for a group that has been clicked off", &Theme::waveshapePillOff);
    add ("waveshape_pill_off_text", "Waveshape Pill Excluded Text", "Text on an excluded legend pill", &Theme::waveshapePillOffText);
    add ("waveshape_pill_text", "Waveshape Pill Text", "Text on the legend pills that are in use", &Theme::waveshapePillText);

    addGetter ("stack_accent", "Stack Accent", "The filter type button under a stack", [] (Theme& t) -> juce::Colour& { return t.stack.main; }, "Stack");
    add ("stack_echo", "Stack Echoes", "The trailing copies of the stage count, each one fainter than the last", &Theme::stackEcho);
    addAccent ("stack", "Stack", &Theme::stack, { slider, sliderHeld, text });
    add ("stack_arrow", "Stack Arrows", "The arrows beside the stage count", &Theme::stackArrow);
    add ("stack_arrow_hover", "Stack Arrows Hovered", "An arrow under the mouse", &Theme::stackArrowHover);
    add ("stack_arrow_disabled", "Stack Arrows Disabled", "An arrow with no more stages to step to", &Theme::stackArrowDisabled);

    section ("modules", "Other Modules", "Compressors, noise, pre distortion, emphasis and the footer. Each type has its own set");

    addAccent ("stereo_comp", "Stereo Comp", &Theme::stereoComp, { knob, knobHeld, text }, "Compressor");
    addAccent ("multiband_comp", "Multiband Comp", &Theme::multibandComp, { knob, knobHeld, text }, "Compressor");
    addAccent ("mid_side_comp", "Mid Side Comp", &Theme::midSideComp, { knob, knobHeld, text }, "Compressor");
    addAccent ("type_a_comp", "Type A Comp", &Theme::typeAComp, { knob, knobHeld, text }, "Compressor");
    addAccent ("transient", "Transient", &Theme::transient, { knob, knobHeld, text }, "Compressor");
    addAccent ("multiband_transient", "Multiband Transient", &Theme::multibandTransient, { knob, knobHeld, text }, "Compressor");
    addAccent ("opto_comp", "Opto Comp", &Theme::optoComp, { knob, knobHeld, text }, "Compressor");
    addAccent ("multiband_opto_comp", "Multiband Opto Comp", &Theme::multibandOptoComp, { knob, knobHeld, text }, "Compressor");
    addAccent ("allpass", "Allpass", &Theme::allpass, { knob, knobHeld, text }, "Pre Distortion");
    addAccent ("grunge", "Grunge", &Theme::grunge, { knob, knobHeld, text }, "Pre Distortion");
    addAccent ("sub_gen", "Sub Gen", &Theme::subGen, { knob, knobHeld, text }, "Pre Distortion");
    addAccent ("hilbert_stack", "Hilbert Stack", &Theme::hilbertStack, { knob, knobHeld, text }, "Pre Distortion");
    addAccent ("sizzle", "Sizzle", &Theme::sizzle, { knob, knobHeld, text }, "Noise");
    addAccent ("erosion", "Erosion", &Theme::erosion, { knob, knobHeld, text }, "Noise");
    addAccent ("bit_reduction", "Bit Reduction", &Theme::bitReduction, { knob, knobHeld, text }, "Noise");
    addAccent ("gate", "Gate", &Theme::gate, { knob, knobHeld, text }, "Noise");
    addAccent ("fizz", "Fizz", &Theme::fizz, { knob, knobHeld, text }, "Noise");
    addAccent ("emphasis", "Emphasis", &Theme::emphasis, { knob, knobHeld, text }, "Emphasis");
    addAccent ("tilt", "Tilt", &Theme::tilt, { knob, knobHeld, text }, "Emphasis");

    addGetter ("footer_accent", "Footer Accent", "What the footer's IN and OUT readouts lean towards as they turn up",
               [] (Theme& t) -> juce::Colour& { return t.footer.main; }, "Footer");
    addAccent ("footer", "Footer", &Theme::footer, { text });

    add ("clip_dot_idle", "Clip Dot Idle", "The clipper's dot while nothing is clipping", &Theme::clipDotIdle);
    add ("clip_dot_knee", "Clip Dot Soft", "The clipper's dot while the level is in the knee", &Theme::clipDotKnee, "Clipping");
    add ("clip_dot_hard", "Clip Dot Hard", "The clipper's dot while the level is past the ceiling", &Theme::clipDotHard, "Clipping");

    section ("scope", "Scope", "The scope in the top right, with its compressor and clipper views");

    add ("scope_background", "Background", "Behind every scope view. Also what the in out trails fade into", &Theme::scopeBackground);
    add ("scope_grid", "Grid Lines", "Centre lines of the stereo scope and the in out view", &Theme::scopeGrid);
    add ("scope_left", "Left Channel", "The left channel in the stereo scope", &Theme::scopeLeft);
    add ("scope_right", "Right Channel", "The right channel in the stereo scope", &Theme::scopeRight);
    add ("scope_transfer", "In Out Trace", "The input against output trace", &Theme::scopeTransfer);
    add ("scope_waveshape_curve", "Waveshape Curve", "The waveshape's curve behind the in out trace", &Theme::scopeWaveshapeCurve);
    add ("scope_noise", "Noise Wave", "The wave in the noise view", &Theme::scopeNoise);
    add ("scope_noise_axis", "Noise Centre Line", "The centre line in the noise view", &Theme::scopeNoiseAxis);
    add ("scope_spectrum_line", "Spectrum Line", "Top edge of the spectrum in the emphasis and tilt views", &Theme::scopeSpectrumLine, "Spectrum");
    add ("scope_spectrum_fill", "Spectrum Fill", "Area under the spectrum in the emphasis and tilt views", &Theme::scopeSpectrumFill, "Spectrum");
    add ("scope_curve_pre", "Curve Before", "The emphasis and tilt curves going into the distortion", &Theme::scopeCurvePre);
    add ("scope_curve_post", "Curve After", "The emphasis and tilt curves coming out of the distortion", &Theme::scopeCurvePost);
    add ("scope_watermark", "Watermark", "The view's name tiled faintly behind it", &Theme::scopeWatermark);
    add ("scope_header", "Readout Background", "Behind the readouts along the top", &Theme::scopeHeader);
    add ("scope_header_text", "Readout Text", "The readouts along the top", &Theme::scopeHeaderText);

    add ("scope_clip_knee_region", "Clipper Knee Region", "The strip showing where the knee is", &Theme::scopeClipKneeRegion);
    add ("scope_clip_axis", "Clipper Axis", "The line along the bottom of the clipper view", &Theme::scopeClipAxis);
    add ("scope_clip_guide", "Clipper Guides", "The unclipped diagonal and the ceiling line", &Theme::scopeClipGuide);
    add ("scope_clip_curve", "Clipper Curve", "The clipping curve", &Theme::scopeClipCurve);
    add ("scope_clip_fill", "Clipper Curve Fill", "Area under the clipping curve", &Theme::scopeClipFill);
    add ("scope_clip_below", "Clipper Level Clean", "The level while it is below the knee", &Theme::scopeClipBelow);
    add ("scope_clip_knee", "Clipper Level Soft", "The level while it is in the knee", &Theme::scopeClipKnee, "Clipping");
    add ("scope_clip_hard", "Clipper Level Hard", "The level while it is past the ceiling", &Theme::scopeClipHard, "Clipping");

    add ("comp_cell", "Comp Band Background", "Behind each band of the compressor view", &Theme::compCell);
    add ("comp_over_threshold", "Comp Above Threshold", "The part of a band above its threshold", &Theme::compOverThreshold);
    add ("comp_stacked_over_threshold", "Comp Stacked Above Threshold", "The same for a band sharing another band's cell", &Theme::compStackedOverThreshold);
    add ("comp_stack_divider", "Comp Stack Divider", "The line between two bands sharing a cell", &Theme::compStackDivider);
    add ("comp_knee", "Comp Knee", "The strip around the threshold where the knee is", &Theme::compKnee);
    add ("comp_ratio_lines", "Comp Ratio Lines", "Lines above the threshold showing how hard the ratio pulls", &Theme::compRatioLines);
    add ("comp_level", "Comp Level", "A band's level while it is under the threshold", &Theme::compLevel);
    add ("comp_level_over", "Comp Level Peaking", "A band's level while it is over the threshold", &Theme::compLevelOver);
    add ("comp_threshold", "Comp Threshold", "The threshold line", &Theme::compThreshold);
    add ("comp_text", "Comp Text", "Threshold readouts and band names", &Theme::compText);
    add ("comp_gain_offset_text", "Comp Gain Offset Text", "How much a band's makeup is trimmed", &Theme::compGainOffsetText);
    add ("comp_cell_edge", "Comp Band Edges", "Lines between the bands", &Theme::compCellEdge);

    section ("multiband", "Multiband", "The band view under the distortion box in the multiband and exciter routings");

    add ("multiband_background", "Background", "Behind the band view", &Theme::multibandBackground);
    add ("multiband_top_edge", "Top Edge", "The line along the top of the band view", &Theme::multibandTopEdge);
    add ("multiband_spectrum_line", "Spectrum Line", "Top edge of the output spectrum", &Theme::multibandSpectrumLine, "Spectrum");
    add ("multiband_spectrum_fill", "Spectrum Fill", "Area under the output spectrum", &Theme::multibandSpectrumFill, "Spectrum");
    add ("multiband_input_spectrum", "Input Spectrum", "The line of the spectrum going in", &Theme::multibandInputSpectrum);
    add ("multiband_in_gain", "In Gain Lines", "Each band's in gain, for reference against its level", &Theme::multibandInGain);
    add ("multiband_level", "Level Lines", "Each band's level line and handle", &Theme::multibandLevel);
    add ("multiband_level_highlight", "Level Lines Highlighted", "The level of the band being edited or hovered", &Theme::multibandLevelHighlight);
    add ("multiband_inactive_shade", "Unselected Shade", "Laid over every band other than the one being edited", &Theme::multibandInactiveShade);
    add ("multiband_silenced_shade", "Silenced Shade", "Laid over a muted band, or one another band's solo silences", &Theme::multibandSilencedShade);
    add ("multiband_band_highlight", "Band Highlight", "Laid over a band while it is dragged or its level is hovered", &Theme::multibandBandHighlight);
    add ("multiband_label", "Labels", "The FULL and HIGH names in the exciter routing", &Theme::multibandLabel);
    add ("multiband_drive", "Drive Readout", "Each band's drive along the bottom", &Theme::multibandDrive);
    add ("multiband_drive_active", "Drive Readout Selected", "The drive of the band being edited", &Theme::multibandDriveActive);
    add ("multiband_drive_hot", "Drive Readout Hovered", "A drive readout under the mouse or being dragged", &Theme::multibandDriveHot);
    add ("multiband_button", "Buttons", "Behind the power, mute, solo and remove buttons", &Theme::multibandButton);
    add ("multiband_button_text", "Button Text", "Mute and solo while they are off", &Theme::multibandButtonText);
    add ("multiband_button_text_on", "Button Text On", "Mute and solo while they are on", &Theme::multibandButtonTextOn);
    add ("multiband_mute", "Mute", "A mute button that is on", &Theme::multibandMute);
    add ("multiband_solo", "Solo", "A solo button that is on", &Theme::multibandSolo);
    add ("multiband_remove", "Remove", "The bin icon", &Theme::multibandRemove);
    add ("multiband_remove_hot", "Remove Hovered", "The bin icon under the mouse", &Theme::multibandRemoveHot);
    add ("multiband_add_line", "Add Line", "Where a click would split a new band", &Theme::multibandAddLine);
    add ("multiband_add_badge", "Add Badge", "The circle behind the plus at the top", &Theme::multibandAddBadge);
    add ("multiband_add_cross", "Add Plus", "The plus at the top", &Theme::multibandAddCross);
    add ("multiband_crossover", "Dividers", "The frequency dividers between bands", &Theme::multibandCrossover);
    add ("multiband_crossover_hot", "Dividers Hovered", "A divider under the mouse or being dragged", &Theme::multibandCrossoverHot);
    add ("multiband_crossover_highlight", "Divider Grab Strip", "The strip a hovered divider can be grabbed by", &Theme::multibandCrossoverHighlight);
    add ("multiband_crossover_modulated", "Divider Modulated", "Where modulation has moved a divider to, the divider itself staying where it's set", &Theme::multibandCrossoverModulated);
    add ("multiband_crossover_text", "Divider Frequency", "The frequency beside each divider", &Theme::multibandCrossoverText);
    add ("multiband_crossover_text_hot", "Divider Frequency Hovered", "The frequency of a hovered divider", &Theme::multibandCrossoverTextHot);

    section ("other", "Other", "The background, boxes, text, buttons and everything else");

    add ("background_base", "Background Base", "Under the background shapes, only seen where they are see through", &Theme::backgroundBase);

    for (int i = 0; i < (int) Theme().backgroundRegions.size(); ++i)
        addGetter ("background_region_" + juce::String (i + 1), "Background Region " + juce::String (i + 1),
                   "A stripe of the background, counting from the left",
                   [i] (Theme& t) -> juce::Colour& { return t.backgroundRegions[(size_t) i]; }, {}, "Background Region");

    for (int i = 0; i < (int) Theme().backgroundLines.size(); ++i)
        addGetter ("background_line_" + juce::String (i + 1), "Background Line " + juce::String (i + 1),
                   "The outline of the matching background stripe",
                   [i] (Theme& t) -> juce::Colour& { return t.backgroundLines[(size_t) i]; }, {}, "Background Line");

    add ("logo_top", "Logo Top", "The top word of the logo", &Theme::logoTop);
    add ("logo_bottom", "Logo Bottom", "The bottom word of the logo", &Theme::logoBottom);
    add ("box", "Boxes", "The rounded boxes everything sits in", &Theme::box);
    add ("box_disabled", "Boxes Off", "A box whose power button is off", &Theme::boxDisabled);
    add ("box_border", "Box Border", "Outline of the boxes, when box_borders is on", &Theme::boxBorder);
    add ("box_border_disabled", "Box Border Off", "Outline of a box whose power button is off", &Theme::boxBorderDisabled);
    add ("knob_thumb", "Knob Pointer", "The line on every knob showing where it is set", &Theme::knobThumb);
    add ("scaled_marker", "Scaled Value Marker", "The thinner pointer on a knob and the dot on a slider, where global drive, the compressor amount or EQ strength really put it", &Theme::scaledMarker);
    add ("slider_track", "Slider Track", "The empty part of every slider", &Theme::sliderTrack);
    add ("power_on", "Power On", "Power buttons that are on", &Theme::powerOn, "Power and Lock");
    add ("power_off", "Power Off", "Power buttons that are off", &Theme::powerOff, "Power and Lock");
    add ("lock_on", "Lock On", "The scope lock and gain link locks while locked", &Theme::lockOn, "Power and Lock");
    add ("lock_off", "Lock Off", "The scope lock while unlocked", &Theme::lockOff, "Power and Lock");
    add ("gain_text_low", "Gain Readout Quiet", "What the IN and OUT readouts fade to as they turn down", &Theme::gainTextLow);
    add ("modulation_highlight", "Modulation", "Anything being modulated, and how far each modulation can move it", &Theme::modulationHighlight);

    {
        const char* const names[] { "Drive", "Macro 1", "Macro 2", "Macro 3", "Macro 4", "Mod 1", "Mod 2", "Mod 3", "Mod 4", "Mod 5" };
        static_assert (std::size (names) == std::tuple_size_v<decltype (Theme::modSources)>);

        for (int i = 0; i < (int) std::size (names); ++i)
            addGetter ("mod_source_" + juce::String (names[i]).toLowerCase().replace (" ", "_"), "Mod Source " + juce::String (names[i]),
                       "This source's name, and where it's moving whatever it modulates",
                       [i] (Theme& t) -> juce::Colour& { return t.modSources[(size_t) i]; }, {}, "Mod Source");
    }

    add ("text_header", "Header Text", "The selected PRE MAIN or POST tab", &Theme::textHeader);
    add ("text_header_hover", "Header Text Hovered", "A tab under the mouse", &Theme::textHeaderHover);
    add ("text_header_idle", "Header Text Idle", "The other tabs", &Theme::textHeaderIdle);
    add ("header_divider", "Header Dividers", "The lines between the tabs", &Theme::headerDivider);
    addGetter ("text_settings", "Settings Text", "Everything written in the settings page", [] (Theme& t) -> juce::Colour& { return t.settings.text; });
    add ("text_settings_dim", "Settings Text Dim", "The numbers in the FX order", &Theme::textSettingsDim);
    add ("text_presets", "Preset Text", "Preset names, folders and buttons", &Theme::textPresets);
    add ("text_presets_author", "Preset Author Text", "Who made each preset", &Theme::textPresetsAuthor);
    add ("text_info", "Info Text", "The info page and the help text under the logo", &Theme::textInfo);

    add ("row", "Rows", "FX order rows and settings buttons", &Theme::row);
    add ("row_dragged", "Row Dragged", "The FX order row being dragged", &Theme::rowDragged);
    add ("row_selected", "Row Selected", "The selected preset", &Theme::rowSelected);
    add ("divider", "Dividers", "Lines under preset folders and between the emphasis bands", &Theme::divider);
    add ("scrollbar", "Scrollbar", "The preset list's scrollbar", &Theme::scrollbar);
    add ("popup_background", "Menu Background", "Behind dropdown menus", &Theme::popupBackground);
    add ("popup_text", "Menu Text", "Dropdown menu items", &Theme::popupText);
    add ("popup_highlight", "Menu Highlight", "The dropdown menu item under the mouse", &Theme::popupHighlight);
    add ("tooltip_background", "Tooltip Background", "Behind tooltips and the scale readout", &Theme::tooltipBackground);
    add ("tooltip_outline", "Tooltip Outline", "Around tooltips and the scale readout", &Theme::tooltipOutline);
    add ("tooltip_text", "Tooltip Text", "Tooltips and the scale readout", &Theme::tooltipText);
    add ("alert_background", "Popup Window Background", "Behind the save, delete and update popups", &Theme::alertBackground);
    add ("alert_outline", "Popup Window Outline", "Around those popups", &Theme::alertOutline);
    add ("alert_text", "Popup Window Text", "Text and buttons in those popups", &Theme::alertText);
    add ("alert_accent", "Popup Window Accent", "The delete and update buttons", &Theme::alertAccent);
    add ("alert_accent_text", "Popup Window Accent Text", "Text on the delete and update buttons", &Theme::alertAccentText);

    return sections;
}

const std::vector<ThemeSection>& themeSections()
{
    static const auto sections = makeSections();
    return sections;
}

std::optional<juce::Colour> parseColour (juce::String text)
{
    text = text.trim().toLowerCase().removeCharacters (" \t()").replace ("rgba", "").replace ("rgb", "");

    if (text.containsChar (','))
    {
        const auto parts = juce::StringArray::fromTokens (text, ",", "");

        if (parts.size() < 3 || parts.size() > 4)
            return {};

        for (const auto& part : parts)
            if (part.isEmpty() || ! part.containsOnly ("0123456789."))
                return {};

        auto channel = [&] (int i) { return (juce::uint8) juce::jlimit (0, 255, parts[i].getIntValue()); };

        // 0 to 1 the css way, or 0 to 255 like the colour channels
        auto alpha = 1.0f;

        if (parts.size() == 4)
            alpha = parts[3].getFloatValue() <= 1.0f ? parts[3].getFloatValue() : parts[3].getFloatValue() / 255.0f;

        return juce::Colour (channel (0), channel (1), channel (2), juce::jlimit (0.0f, 1.0f, alpha));
    }

    text = text.trimCharactersAtStart ("#");

    if (text.startsWith ("0x"))
        text = text.substring (2);

    if (! text.containsOnly ("0123456789abcdef"))
        return {};

    // shorthand doubles every digit, so f0a is ff00aa
    if (text.length() == 3 || text.length() == 4)
    {
        juce::String full;

        for (auto c : text)
            full << juce::String::charToString (c) << juce::String::charToString (c);

        text = full;
    }

    if (text.length() == 6)
        text << "ff";

    if (text.length() != 8)
        return {};

    const auto rgba = (juce::uint32) text.getHexValue64();
    return juce::Colour ((rgba << 24) | (rgba >> 8));
}

juce::String colourToHex (juce::Colour colour)
{
    auto hex = "#" + colour.toDisplayString (false);

    if (! colour.isOpaque())
        hex << juce::String::toHexString (colour.getAlpha()).paddedLeft ('0', 2).toUpperCase();

    return hex;
}

void applyThemeToSvg (juce::XmlElement& svg)
{
    static const auto byId = []
    {
        std::map<juce::String, const ThemeColour*> colours;

        for (const auto& section : themeSections())
            for (const auto& colour : section.colours)
                colours[colour.id] = &colour;

        return colours;
    }();

    const juce::StringArray colourProperties { "fill", "stroke", "stop-color", "color" };

    auto themed = [&] (const juce::String& property, const juce::String& value)
    {
        const auto found = byId.find (value.trim());

        if (! colourProperties.contains (property.trim()) || found == byId.end())
            return value;

        return colourToHex (found->second->in (theme()));
    };

    for (int i = 0; i < svg.getNumAttributes(); ++i)
    {
        const auto name = svg.getAttributeName (i);
        auto value = svg.getAttributeValue (i);

        // the same inside a style, like style="fill:logo_top"
        if (name == "style")
        {
            auto properties = juce::StringArray::fromTokens (value, ";", "");

            for (auto& property : properties)
                if (property.containsChar (':'))
                    property = property.upToFirstOccurrenceOf (":", true, false)
                             + themed (property.upToFirstOccurrenceOf (":", false, false), property.fromFirstOccurrenceOf (":", false, false));

            value = properties.joinIntoString (";");
        }
        else
        {
            value = themed (name, value);
        }

        svg.setAttribute (name, value);
    }

    for (auto* child : svg.getChildIterator())
        applyThemeToSvg (*child);
}

static const char* bordersKey = "box_borders";

juce::String themeToIni (Theme theme, const ThemeInfo& info)
{
    CSimpleIniA ini;

    const auto header = juce::String ("; Hamburger theme\n"
                                      "; Colours are written as #RRGGBB, or #RRGGBBAA where the last two digits are the opacity\n"
                                      "; They can also be written as r, g, b or r, g, b, a with every value out of 255\n"
                                      "; Anything missing from this file falls back to the default theme\n"
                                      ";\n"
                                      "; background can name an svg in the themes folder to draw behind everything instead of the built in one\n"
                                      "; Write a colour id from this file where the svg would have a colour, like fill=\"background_region_1\",\n"
                                      "; and it's drawn in that colour. Hamburger's own background svg is written the same way\n"
                                      "; The description below is only for people reading this file\n"
                                      ";\n"
                                      "; description: ") + info.description.replace ("\n", " ");

    ini.SetValue ("theme", nullptr, nullptr, header.toRawUTF8());

    ini.SetValue ("theme", "name", info.name.toRawUTF8(), "; Shown in the theme list as author - name");
    ini.SetValue ("theme", "author", info.author.toRawUTF8());
    ini.SetValue ("theme", "background", info.background.toRawUTF8());

    for (const auto& section : themeSections())
    {
        const auto rule = juce::String::repeatedString ("-", 80);
        const auto heading = "; " + rule + "\n; " + section.title.toUpperCase() + "\n; " + section.description + "\n; " + rule;

        ini.SetValue (section.id.toRawUTF8(), nullptr, nullptr, heading.toRawUTF8());

        if (section.id == "other")
            ini.SetValue ("other", bordersKey, theme.boxBorders ? "true" : "false", "; true to draw an outline around every box");

        for (const auto& colour : section.colours)
            ini.SetValue (section.id.toRawUTF8(), colour.id.toRawUTF8(), colourToHex (colour.in (theme)).toRawUTF8(),
                          ("; " + colour.description).toRawUTF8());
    }

    std::string text;
    ini.Save (text);

    return juce::String::fromUTF8 (text.data(), (int) text.size());
}

Theme themeFromIni (const juce::String& text, ThemeInfo& info)
{
    CSimpleIniA ini;
    ini.LoadData (text.toStdString());

    info.name = juce::String::fromUTF8 (ini.GetValue ("theme", "name", "Untitled"));
    info.author = juce::String::fromUTF8 (ini.GetValue ("theme", "author", ""));
    info.background = juce::String::fromUTF8 (ini.GetValue ("theme", "background", ""));
    info.description = {};

    for (const auto& line : juce::StringArray::fromLines (text))
        if (line.trim().startsWith ("; description:"))
            info.description = line.fromFirstOccurrenceOf ("description:", false, false).trim();

    // by id whichever section it's under, so moving a colour to another section doesn't lose it from older files
    std::map<juce::String, juce::String> values;

    CSimpleIniA::TNamesDepend sections, keys;
    ini.GetAllSections (sections);

    for (const auto& section : sections)
    {
        ini.GetAllKeys (section.pItem, keys);

        for (const auto& key : keys)
            values[key.pItem] = juce::String::fromUTF8 (ini.GetValue (section.pItem, key.pItem, ""));
    }

    Theme theme;

    for (const auto& section : themeSections())
        for (const auto& colour : section.colours)
            if (auto found = values.find (colour.id); found != values.end())
                if (auto parsed = parseColour (found->second))
                    colour.in (theme) = *parsed;

    if (auto found = values.find (bordersKey); found != values.end())
        theme.boxBorders = found->second.trim().equalsIgnoreCase ("true");

    return theme;
}
