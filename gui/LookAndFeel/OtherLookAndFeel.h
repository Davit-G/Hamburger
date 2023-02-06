#pragma once

#include <JuceHeader.h>

class OtherLookAndFeel : public juce::LookAndFeel_V4 
{
public:
    OtherLookAndFeel() {
        setColour(juce::Slider::thumbColourId, juce::Colours::orange);
        
    };

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider&) override
    {
        auto radius = (float) juce::jmin (width / 2, height / 2) - 4.0f;
        auto centreX = (float) x + (float) width  * 0.5f;
        auto centreY = (float) y + (float) height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        float displacement3d = 2.0f;

        // // anisotropic image texture

        




        // // 3d effect fill
        // // juce::ColourGradient cg3d (juce::Colour::fromRGB(130,0,186), centreX, centreY - radius,
        // //                            juce::Colour::fromRGB(148,0,205), centreX, centreY + radius, false);
        // // g.setGradientFill (cg3d);
        // // g.fillEllipse (rx + displacement3d, ry + displacement3d, rw + displacement3d, rw + displacement3d);

        // // // thin outline over 3d effect
        // // g.setColour (juce::Colours::black.withAlpha (0.5f));
        // // g.drawEllipse (rx + displacement3d, ry + displacement3d, rw + displacement3d, rw + displacement3d, 1.0f);

        // 3d effect fill
        juce::ColourGradient cg (juce::Colour::fromRGB(130,0,186), centreX, ry,
                                 juce::Colour::fromRGB(148,0,205), centreX, centreY + radius, false);
        g.setGradientFill (cg);
        g.fillEllipse (rx + displacement3d, ry + displacement3d, rw + displacement3d, rw + displacement3d);

        // thin outline over 3d effect
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawEllipse (rx + displacement3d, ry + displacement3d, rw + displacement3d, rw + displacement3d, 1.0f);

        // shadow
        juce::ColourGradient cg2 (juce::Colour::fromRGB(0,0,0), centreX, centreY - radius,
                                  juce::Colour::fromRGB(0,0,0), centreX, centreY + radius, false);
        cg2.addColour (0.5, juce::Colour::fromRGB(0,0,0).withAlpha (0.0f));
        g.setGradientFill (cg2);
        g.fillEllipse (rx + displacement3d, ry + displacement3d, rw + displacement3d, rw + displacement3d);


        
        // fill circle with anisotropic image texture
        // first, create a circular path
        // juce::Path p;
        // p.addEllipse(rx, ry, rw, rw);
        // Image anisotropy = ImageCache::getFromMemory(BinaryData::anisotropiccolour_jpg, BinaryData::anisotropiccolour_jpgSize);
        // // scale image to width and height of circle
        // anisotropy = anisotropy.rescaled(rw, rw);
        // // g.setOpacity(0.4f);
        // // reduceClipRegion() will clip the image to the circular path
        // g.reduceClipRegion(p);
        // // draw the image
        // g.drawImageAt(anisotropy, rx, ry);

        // black outline
        g.setColour (juce::Colours::black);
        g.drawEllipse (rx, ry, rw, rw, 9.0f);

        

            
        juce::Path p2;
        auto pointerLength = radius * 0.5f;
        auto pointerThickness = 3.0f;
        p2.addRectangle (-pointerThickness * 0.5f, -radius + 4, pointerThickness, pointerLength);
        p2.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));

        // pointer
        g.setColour (juce::Colours::white);
        g.fillPath (p2);

        juce::Path p3;
        pointerLength = radius * 0.3f;
        pointerThickness = 2.8f;
        p3.addRectangle (-pointerThickness * 0.4f, -radius + 6, pointerThickness, pointerLength);
        p3.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));

        // pointer
        g.setColour (juce::Colours::black);
        g.fillPath (p3);
    }
};