#pragma once

#include "../utils/Params.h"
#include "MacroParam.h"

namespace EffectInfos
{
    /*
        macro 0 is always the effect's primary control:
        main / module1 / module2 for amount / drive
        dynamics 0 is threshold
        postClip 0 is the input gain
        etc
    */

    static const ParamIDs::EffectInfo grill {
        {
            ParamIDs::saturationAmount,
            ParamIDs::grillBias,
            ParamIDs::diode,
            ParamIDs::fold,
            ParamIDs::grillDcTiming
        }
    };

    static const ParamIDs::EffectInfo tube {
        {
            ParamIDs::tubeAmount,
            ParamIDs::tubeTone,
            ParamIDs::tubeBias,
            ParamIDs::jeffAmount
        }
    };

    static const ParamIDs::EffectInfo phase {
        {
            ParamIDs::phaseAmount,
            ParamIDs::phaseDistTone,
            ParamIDs::phaseDistStereo,
            ParamIDs::phaseRectify,
            ParamIDs::phaseShift
        }
    };

    static const ParamIDs::EffectInfo rubidium {
        {
            ParamIDs::rubidiumAmount,
            ParamIDs::rubidiumTone,
            ParamIDs::rubidiumMojo,
            ParamIDs::rubidiumAsym,
            ParamIDs::rubidiumBias
        }
    };

    static const ParamIDs::EffectInfo tape {
        {
            ParamIDs::tapeDrive,
            ParamIDs::tapeBias,
            ParamIDs::tapeWidth
        }
    };

    static const ParamIDs::EffectInfo slew {
        {
            ParamIDs::alphaParam,
            ParamIDs::slewSpeed,
            ParamIDs::directionality
        }
    };

    static const ParamIDs::EffectInfo waveshape {
        {
            ParamIDs::waveshapeDrive,
            ParamIDs::waveshapeX,
            ParamIDs::waveshapeY,
            ParamIDs::waveshapeSmooth,
            ParamIDs::waveshapeBias,
            ParamIDs::waveshapeAsym
        }
    };

    static const ParamIDs::EffectInfo opto {
        {
            ParamIDs::optoDrive,
            ParamIDs::optoBias,
            ParamIDs::optoDcSpeed,
            ParamIDs::optoDamping
        }
    };

    static const ParamIDs::EffectInfo optoComp {
        {
            ParamIDs::optoThreshold,
            ParamIDs::optoRatio,
            ParamIDs::optoCompSpeed,
            ParamIDs::compStereoLink
        }
    };

    static const ParamIDs::EffectInfo mbOptoComp {
        {
            ParamIDs::optoThreshold,
            ParamIDs::optoRatio,
            ParamIDs::optoCompSpeed,
            ParamIDs::compBandTilt
        }
    };

    static const ParamIDs::EffectInfo stereoComp {
        {
            ParamIDs::stereoCompThreshold,
            ParamIDs::compRatio,
            ParamIDs::compSpeed,
            ParamIDs::compStereoLink
        }
    };

    static const ParamIDs::EffectInfo mbComp {
        {
            ParamIDs::MBCompThreshold,
            ParamIDs::compRatio,
            ParamIDs::compBandTilt,
            ParamIDs::MBCompSpeed
        }
    };

    static const ParamIDs::EffectInfo msComp {
        {
            ParamIDs::MSCompThreshold,
            ParamIDs::compRatio,
            ParamIDs::compBandTilt,
            ParamIDs::MSCompSpeed
        }
    };

    static const ParamIDs::EffectInfo typeA {
        {
            ParamIDs::TypeAThreshold,
            ParamIDs::TypeARatio,
            ParamIDs::TypeATilt,
            ParamIDs::TypeACompSpeed
        }
    };

    static const ParamIDs::EffectInfo transient {
        {
            ParamIDs::transientAttack,
            ParamIDs::transientSustain,
            ParamIDs::transientSpeed,
            ParamIDs::transientLink
        }
    };

    static const ParamIDs::EffectInfo mbTransient {
        {
            ParamIDs::transientAttack,
            ParamIDs::transientSustain,
            ParamIDs::transientTilt,
            ParamIDs::transientSpeed
        }
    };

    static const ParamIDs::EffectInfo sizzle {
        {
            ParamIDs::sizzleAmount,
            ParamIDs::sizzleFrequency,
            ParamIDs::sizzleQ,
            ParamIDs::fizzAmount
        }
    };

    static const ParamIDs::EffectInfo erosion {
        {
            ParamIDs::erosionAmount,
            ParamIDs::erosionFrequency,
            ParamIDs::erosionQ
        }
    };

    static const ParamIDs::EffectInfo redux {
        {
            ParamIDs::downsampleFreq,
            ParamIDs::bitReduction
        }
    };

    static const ParamIDs::EffectInfo gate {
        {
            ParamIDs::gateAmt,
            ParamIDs::gateSmooth
        }
    };

    static const ParamIDs::EffectInfo disperser {
        {
            ParamIDs::allPassAmount,
            ParamIDs::allPassFreq,
            ParamIDs::allPassQ
        }
    };

    static const ParamIDs::EffectInfo grunge {
        {
            ParamIDs::grungeAmt,
            ParamIDs::grungeTone
        }
    };

    static const ParamIDs::EffectInfo subGen {
        {
            ParamIDs::subGenAmount
        }
    };

    static const ParamIDs::EffectInfo hilbertStack {
        {
            ParamIDs::hilbertStacks
        }
    };

    // the limiter keeps the gain and swaps the knee for its release time
    static const ParamIDs::EffectInfo limiter {
        {
            ParamIDs::postClipGain,
            ParamIDs::postClipTime
        }
    };

    static const ParamIDs::EffectInfo postClip {
        {
            ParamIDs::postClipGain,
            ParamIDs::postClipKnee
        }
    };

    struct LayoutList
    {
        const ParamIDs::EffectInfo* const* data = nullptr;
        int count = 0;

        const ParamIDs::EffectInfo* const* begin() const { return data; }
        const ParamIDs::EffectInfo* const* end()   const { return data + count; }
    };

    inline LayoutList layoutsFor (ModuleId id)
    {
        static const ParamIDs::EffectInfo* const mainTypes[]     { &grill, &tube, &phase, &rubidium, &tape, &slew, &waveshape, &opto };
        static const ParamIDs::EffectInfo* const dynamicsTypes[] { &stereoComp, &mbComp, &msComp, &typeA, &transient, &mbTransient, &optoComp, &mbOptoComp };
        static const ParamIDs::EffectInfo* const noiseTypes[]    { &sizzle, &erosion, &redux, &gate, &sizzle }; // FIZZ reuses sizzle
        static const ParamIDs::EffectInfo* const preDistTypes[]  { &disperser, &grunge, &subGen, &hilbertStack };
        static const ParamIDs::EffectInfo* const postClipTypes[] { &postClip, &limiter };

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

    // returns the correct param id when given parameter info
    inline juce::String paramIdForDescriptor (const ParamIDs::ParameterInfo& which, int sub = 0)
    {
        for (int m = 0; m < (int) ModuleId::count; ++m)
        {
            const auto module = (ModuleId) m;

            for (const auto* layout : layoutsFor (module))
                for (const auto& descriptor : layout->params)
                    if (descriptor.id == which.id)
                        return paramIdFor (SlotId { module, sub }, which).getParamID();
        }

        return {};
    }

    // relabels every parameter belonging to a slot. pass an empty prefix to restore original names. 
    // message thread only. follow with updateHostDisplay (ChangeDetails{}.withParameterInfoChanged (true))
    inline void setSlotNamePrefix (juce::AudioProcessorValueTreeState& state,
                                   SlotId slot,
                                   const juce::String& prefix)
    {
        for (const auto* layout : layoutsFor (slot.module))
        {
            for (const auto& descriptor : layout->params)
            {
                if (descriptor.id.isNull())
                    continue;

                if (auto* param = dynamic_cast<MacroParam*> (
                        state.getParameter (paramIdFor (slot, descriptor).getParamID())))
                {
                    if (prefix.isEmpty())
                        param->clearNameOverride();
                    else
                        param->setNameOverride (prefix + " " + descriptor.displayName);
                }
            }
        }
    }
}
