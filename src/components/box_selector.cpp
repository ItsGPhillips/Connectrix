
#include "box_selector.h"

namespace cntx::components
{
   BoxSelector::BoxSelector ()
   {
      setAlwaysOnTop (true);
      setOpaque (false);
   }
   void BoxSelector::beginSelection (juce::Point<f32> pos) { m_mouseDownPos = pos; }
   void BoxSelector::resizeSelection (juce::Point<f32> currentMousePos)
   {
      juce::Rectangle<f32> r;
      if (m_mouseDownPos.getX () < currentMousePos.getX ()) {
         r.setX (m_mouseDownPos.getX ());
         r.setWidth (currentMousePos.getX () - m_mouseDownPos.getX ());
      } else {
         r.setX (currentMousePos.getX ());
         r.setWidth (m_mouseDownPos.getX () - currentMousePos.getX ());
      }
      if (m_mouseDownPos.getY () < currentMousePos.getY ()) {
         r.setY (m_mouseDownPos.getY ());
         r.setHeight (currentMousePos.getY () - m_mouseDownPos.getY ());
      } else {
         r.setY (currentMousePos.getY ());
         r.setHeight (m_mouseDownPos.getY () - currentMousePos.getY ());
      }
      setBounds (r.toNearestInt ());
      repaint ();
   }

   juce::Rectangle<f32> BoxSelector::getRect () const { return getBounds ().toFloat (); }
   void BoxSelector::reset () { setBounds ({ 0, 0 }); }
   void BoxSelector::paint (juce::Graphics& g)
   {
      using namespace juce;
      auto bounds = getLocalBounds ();
      m_path.clear ();
      m_path.addRectangle (bounds);
      g.setColour (Colours::azure.withAlpha (0.3f));
      g.fillPath (m_path);
      g.setColour (Colours::azure.withAlpha (0.7f));
      g.strokePath (m_path, PathStrokeType (2));
   }
} // namespace cntx::components