#pragma once

namespace cntx::components
{
   class BoxSelector: public juce::Component
   {
   public:
      BoxSelector ();
      void beginSelection (juce::Point<f32> pos);
      void resizeSelection (juce::Point<f32> pos);
      juce::Rectangle<f32> getRect () const;
      void reset ();

      // @internal
      void paint (juce::Graphics&) final override;

   private:
      // creating a path in the paint function requires allocting.
      // we should store the path and reuse the allocation.
      juce::Path m_path;
      juce::Point<f32> m_mouseDownPos;
      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoxSelector)
   };
} // namespace cntx::components