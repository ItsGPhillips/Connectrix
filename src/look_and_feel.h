#pragma once

namespace cntx
{
   class LookAndFeel: public juce::LookAndFeel_V4
   {
   public:
      LookAndFeel ()
      {
         setColour (juce::ToggleButton::ColourIds::textColourId, juce::Colour::fromRGB (46, 46, 45));
         setColour (juce::ToggleButton::ColourIds::tickColourId, juce::Colour::fromRGB (46, 46, 45));
         setColour (juce::ToggleButton::ColourIds::tickDisabledColourId, juce::Colour::fromRGB (46, 46, 45));
      }

      void drawRotarySlider (juce::Graphics& g,
                             int x,
                             int y,
                             int width,
                             int height,
                             float sliderPosProportional,
                             float rotaryStartAngle,
                             float rotaryEndAngle,
                             juce::Slider& slider) override
      {
         using namespace juce;

         auto bounds = Rectangle<int> { x, y, width, height };
         bounds.reduce (2, 2);
         float centre_x = x + width / 2.0f;
         float centre_y = y + height / 2.0f;

         auto radius = jmin (bounds.getWidth (), bounds.getHeight ()) / 2.0f;
         auto toAngle = rotaryStartAngle
                        + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
         auto arcRadius = radius - 5.0f * 0.5f;

         Path backgroundArc;
         backgroundArc.addCentredArc (bounds.getCentreX (),
                                      bounds.getCentreY (),
                                      arcRadius,
                                      arcRadius,
                                      0.0f,
                                      rotaryStartAngle,
                                      rotaryEndAngle,
                                      true);

         g.setColour (slider.isEnabled () ? Colour::fromRGB (31, 31, 31)
                                          : Colour::fromRGB (31, 31, 31).brighter ());
         g.strokePath (backgroundArc, PathStrokeType (6.0f));

         if (slider.isEnabled ()) {
            Path valueArc;
            valueArc.addCentredArc (bounds.getCentreX (),
                                    bounds.getCentreY (),
                                    arcRadius,
                                    arcRadius,
                                    0.0f,
                                    rotaryStartAngle,
                                    toAngle,
                                    true);

            g.setColour (Colour::fromRGB (64, 176, 207));
            g.strokePath (valueArc, PathStrokeType (6));
         }

         Path outline;
         PathStrokeType (6.0f).createStrokedPath (outline, backgroundArc);
         g.setColour (Colour::fromRGB (102, 102, 102));
         g.strokePath (outline, PathStrokeType (1.2f));
      }

   private:
   };
} // namespace cntx