#pragma once

#include "../utils/Params.h"

// one effect's macro layout; which module and type name it belongs to lives in pluginModules
struct EffectInfo
{
    std::array<ParamIDs::ParameterInfo, ParamIDs::maxParamsPerEffect> params;
};


namespace EffectInfos
{
    /*
        macro 0 is always the effect's primary control:
        main / module1 / module2 for amount / drive
        dynamics 0 is threshold
        postClip 0 is the input gain
        etc
    */

    using namespace ParamIDs;

    static const EffectInfo grill {
        {
            saturationAmount,
            grillBias,
            diode,
            fold,
            grillDcTiming
        }
    };

    static const EffectInfo tube {
        {
            tubeAmount,
            tubeTone,
            tubeBias,
            jeffAmount
        }
    };

    static const EffectInfo phase {
        {
            phaseAmount,
            phaseDistTone,
            phaseDistStereo,
            phaseRectify,
            phaseShift
        }
    };

    static const EffectInfo rubidium {
        {
            rubidiumAmount,
            rubidiumTone,
            rubidiumMojo,
            rubidiumAsym,
            rubidiumBias
        }
    };

    static const EffectInfo tape {
        {
            tapeDrive,
            tapeBias,
            tapeWidth
        }
    };

    static const EffectInfo slew {
        {
            alphaParam,
            slewSpeed,
            directionality
        }
    };

    static const EffectInfo waveshape {
        {
            waveshapeDrive,
            waveshapeX,
            waveshapeY,
            waveshapeSmooth,
            waveshapeBias,
            waveshapeAsym
        }
    };

    static const EffectInfo dll {
        {
            dllDrive,
            dllA,
            dllB,
            dllC,
            dllD,
            dllE,
            dllF
        }
    };

    static const EffectInfo optoComp {
        {
            optoThreshold,
            optoRatio,
            optoCompSpeed,
            compStereoLink
        }
    };

    static const EffectInfo mbOptoComp {
        {
            optoThreshold,
            optoRatio,
            optoCompSpeed,
            compBandTilt
        }
    };

    static const EffectInfo stereoComp {
        {
            stereoCompThreshold,
            compRatio,
            compSpeed,
            compStereoLink
        }
    };

    static const EffectInfo mbComp {
        {
            MBCompThreshold,
            compRatio,
            compBandTilt,
            MBCompSpeed
        }
    };

    static const EffectInfo msComp {
        {
            MSCompThreshold,
            compRatio,
            compBandTilt,
            MSCompSpeed
        }
    };

    static const EffectInfo typeA {
        {
            TypeAThreshold,
            TypeARatio,
            TypeATilt,
            TypeACompSpeed
        }
    };

    static const EffectInfo transient {
        {
            transientAttack,
            transientSustain,
            transientSpeed,
            transientLink
        }
    };

    static const EffectInfo mbTransient {
        {
            transientAttack,
            transientSustain,
            transientTilt,
            transientSpeed
        }
    };

    static const EffectInfo sizzle {
        {
            sizzleAmount,
            sizzleFrequency,
            sizzleQ,
            fizzAmount
        }
    };

    static const EffectInfo erosion {
        {
            erosionAmount,
            erosionFrequency,
            erosionQ
        }
    };

    static const EffectInfo redux {
        {
            downsampleFreq,
            bitReduction
        }
    };

    static const EffectInfo gate {
        {
            gateAmt,
            gateSmooth
        }
    };

    static const EffectInfo disperser {
        {
            allPassAmount,
            allPassFreq,
            allPassQ
        }
    };

    static const EffectInfo grunge {
        {
            grungeAmt,
            grungeTone
        }
    };

    static const EffectInfo subGen {
        {
            subGenAmount
        }
    };

    static const EffectInfo hilbertStack {
        {
            hilbertStacks
        }
    };

    // the limiter keeps the gain and swaps the knee for its release time
    static const EffectInfo limiter {
        {
            postClipGain,
            postClipTime
        }
    };

    static const EffectInfo postClip {
        {
            postClipGain,
            postClipKnee
        }
    };

    struct LayoutList
    {
        const EffectInfo* const* data = nullptr;
        int count = 0;

        const EffectInfo* const* begin() const { return data; }
        const EffectInfo* const* end()   const { return data + count; }
    };

    inline LayoutList layoutsFor (ModuleId id)
    {
        static const EffectInfo* const mainTypes[]     { &grill, &tube, &phase, &rubidium, &tape, &slew, &waveshape,
#if JUCE_DEBUG
                                                                   &dll
#endif
                                                                   };
        static const EffectInfo* const dynamicsTypes[] { &stereoComp, &mbComp, &msComp, &typeA, &transient, &mbTransient, &optoComp, &mbOptoComp };
        static const EffectInfo* const noiseTypes[]    { &sizzle, &erosion, &redux, &gate, &sizzle }; // FIZZ reuses sizzle
        static const EffectInfo* const preDistTypes[]  { &disperser, &grunge, &subGen, &hilbertStack };
        static const EffectInfo* const postClipTypes[] { &postClip, &limiter };

        switch (id)
        {
            case ModuleId::main:         return { mainTypes,     (int) std::size (mainTypes) };
            case ModuleId::preDistortion:         return { mainTypes,     (int) std::size (mainTypes) };
            case ModuleId::postDistortion:         return { mainTypes,     (int) std::size (mainTypes) };
            case ModuleId::dynamics:     return { dynamicsTypes, (int) std::size (dynamicsTypes) };
            case ModuleId::module1:      return { noiseTypes,    (int) std::size (noiseTypes) };
            case ModuleId::module2:      return { preDistTypes,  (int) std::size (preDistTypes) };
            case ModuleId::postClip:     return { postClipTypes, (int) std::size (postClipTypes) };
            case ModuleId::preEmphasis:
            case ModuleId::postEmphasis:
            case ModuleId::count:
            default:                     return {};
        }
    }

    // // relabels every parameter belonging to a slot. pass an empty prefix to restore original names. 
    // // message thread only. follow with updateHostDisplay (ChangeDetails{}.withParameterInfoChanged (true))
    // inline void setSlotNamePrefix (juce::AudioProcessorValueTreeState& state,
    //                                SlotId slot,
    //                                const juce::String& prefix)
    // {
    //     for (const auto* layout : layoutsFor (slot.module))
    //     {
    //         for (const auto& descriptor : layout->params)
    //         {
    //             if (descriptor.id.isNull())
    //                 continue;

    //             if (auto* param = dynamic_cast<MacroParam*> (
    //                     state.getParameter (paramIdFor (slot, descriptor).getParamID())))
    //             {
    //                 if (prefix.isEmpty())
    //                     param->clearNameOverride();
    //                 else
    //                     param->setNameOverride (prefix + " " + descriptor.displayName);
    //             }
    //         }
    //     }
    // }
}
