// #pragma once
// #include <JuceHeader.h>
// #include "./gui/FilmStripKnob.h"

// struct AttachedParameter {
//     AttachedParameter(juce::AudioProcessorValueTreeState& state, juce::String paramID, juce::String paramName, float minRange, float maxRange, float defaultValue, int knobSize, int x, int y)
//     : treeState(state) {
        
//         parameter = std::make_unique<juce::AudioParameterFloat>(paramID, paramName, minRange, maxRange, defaultValue);

//         this->paramID = paramID;
//         this->defaultValue = defaultValue;

//         this->x = x;
//         this->y = y;
//         this->minRange = minRange;
//         this->maxRange = maxRange;
//         this->paramName = paramName;
//         this->knobSize = knobSize;
//     };

//     ~AttachedParameter() {
//         attachment = nullptr; // important, otherwise plugin will crash
//         // parameter = nullptr; // might not need this tbh

//         // parameterPtr = nullptr;
//     }

//     void refresh() { // refresh reallocates the parameter to the one in the apvts
//         if (parameter == nullptr) {
//             parameter.reset(dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter(paramID)));
//         }
//     }

//     std::unique_ptr<juce::AudioParameterFloat> parameter = nullptr; // the parameter binds to the APVTS which is used in the processor (dsp)
//     std::unique_ptr<FilmStripKnob> knob; // the knob is used in the editor to show the funky rotary knob (gui)
//     std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment = nullptr; // the attachment ties the dsp and the gui together

//     // a raw pointer to the AudioParameterFloat for easy access after it's added to the AudioProcessorValueTreeState
//     // juce::AudioParameterFloat* parameterPtr = nullptr;


//     juce::String paramID;
//     juce::String paramName;
    
//     float minRange;
//     float maxRange;
//     float knobSize;
//     int x;
//     int y;
//     float defaultValue;

// private:
//     juce::AudioProcessorValueTreeState& treeState;

    
// };