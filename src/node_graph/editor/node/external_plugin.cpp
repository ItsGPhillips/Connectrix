#include "external_plugin.h"
#include "../data.h"

namespace cntx::node_graph
{

   class OpenEditorButton: public juce::Button
   {
   public:
      OpenEditorButton (ExternalPluginNode& parent) : juce::Button ("Open Editor")
      {
         setSize (24, 24);
         setTooltip ("Open Editor");
         setMouseCursor (juce::MouseCursor::PointingHandCursor);
         setPaintingIsUnclipped (true);
         onClick = [&parent] { parent.showGuiWindow (); };
      }
      void paintButton (juce::Graphics& g,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override
      {
         using namespace juce;

         Colour iconColour = Colours::grey.brighter (1.0);
         Colour bgColour = Colours::grey;

         Path p;
         p.addRectangle (getLocalBounds ());

         if (shouldDrawButtonAsDown) {
            iconColour = Colour::fromRGB (153, 211, 224);
            bgColour = bgColour.darker (3.0f);
         } else if (shouldDrawButtonAsHighlighted) {
            iconColour = iconColour.brighter ();
            bgColour = bgColour.brighter ();
         }

         g.setColour (bgColour);
         g.fillPath (p);

         auto iconBounds = getLocalBounds ().reduced (5).toFloat ();
         m_iconPath->scaleToFit (iconBounds.getX (),
                                 iconBounds.getY (),
                                 iconBounds.getWidth (),
                                 iconBounds.getHeight (),
                                 true);

         g.setColour (iconColour);

         g.fillPath (*m_iconPath);
         // HACK
         // The stroke path is to thicken up the icon a bit
         g.strokePath (*m_iconPath, PathStrokeType (0.5f));
      }

   private:
      static Rc<juce::Path> m_iconPath;
   };

   Rc<juce::Path> OpenEditorButton::m_iconPath = ([] {
      juce::Path p;
      p.loadPathFromData (data::icons::innerEnlargeIconPath,
                          sizeof (data::icons::innerEnlargeIconPath));
      return std::make_shared<juce::Path> (p);
   }) ();

   ExternalPluginNode::ExternalPluginNode (Node& parent,
                                           juce::ValueTree valueTree,
                                           juce::PluginDescription description)
      : Node::Base (parent, valueTree), m_description (description),
        m_open_editor_btn (new OpenEditorButton (*this))
   {
      setName (description.name);
      addAndMakeVisible (*m_open_editor_btn);
   }

   void ExternalPluginNode::resized ()
   {
      m_open_editor_btn->setCentrePosition (getLocalBounds ().getCentre ());
   }

   void ExternalPluginNode::showGuiWindow ()
   {
      auto display_centre = ([this] () -> juce::Point<i32> {
         auto& displays = juce::Desktop::getInstance ().getDisplays ();
         if (auto* display = displays.getDisplayForRect (getScreenBounds ())) {
            return display->userArea.getCentre ();
         } else {
            // No displays available
            jassertfalse;
            return {};
         }
      }) ();

      auto id = getParent ().getNodeId ();
      auto& node_graph = getParent ().getNodeGraph ();
      if (auto* window = node_graph.getOrCreateWindowForId (id)) {
         window->resized ();
         window->setCentrePosition (display_centre);
         window->setVisible (true);
      }
   }

} // namespace cntx::node_graph