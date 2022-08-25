#include "plugin_editor.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "logger.h"
#include "look_and_feel.h"
#include "node_graph/node_graph.h"
#include "plugin_processor.h"
#include "settings.h"
#include <memory>

namespace cntx
{
   DemoLabel::DemoLabel () : m_label_text ("Connectrix by George Phillips | Version 0.0.1 DEMO")
   {
      setOpaque (true);
      setBufferedToImage (true);
   }

   void DemoLabel::paint (juce::Graphics& g)
   {
      using namespace juce;
      auto bounds = getLocalBounds ();

#if JUCE_DEBUG
      auto debug_label_bounds = bounds.removeFromLeft (90);
      g.setColour (juce::Colours::red);
      g.fillRect (debug_label_bounds);
      g.setColour (Colour::fromRGB (46, 46, 45));
      g.drawText ("DEBUG", debug_label_bounds, juce::Justification::centred);
#endif

      g.setGradientFill (ColourGradient (Colour::fromRGB (194, 192, 190),
                                         bounds.getTopLeft ().toFloat (),
                                         Colour::fromRGB (194, 192, 190).darker (0.1f),
                                         bounds.getBottomRight ().toFloat (),
                                         true));

      g.fillRect (bounds);
      g.setColour (Colour::fromRGB (46, 46, 45));
      g.drawText (m_label_text, getLocalBounds ().reduced (2), Justification::centred, false);
      g.drawHorizontalLine (0, 0, static_cast<f32> (getWidth ()));
   }

   class BypassButton: public juce::ToggleButton,
                       public juce::AudioProcessorParameter::Listener
   {
   public:
      BypassButton (PluginProcessor& processor) : m_processor (processor)
      {
         setButtonText ("Bypass");
         if (auto* bypass_param = m_processor.getBypassParameter ()) {
            bypass_param->addListener (this);
            setToggleState (static_cast<bool> (bypass_param->getValue ()),
                            juce::dontSendNotification);
         }
         onClick = [&] {
            if (auto* bypass_param = m_processor.getBypassParameter ()) {
               bypass_param->setValueNotifyingHost (static_cast<float> (getToggleState ()));
            }
         };
      }

      ~BypassButton () override
      {
         if (auto* bypass_param = m_processor.getBypassParameter ()) {
            bypass_param->removeListener (this);
         }
      }

      void parameterValueChanged (int /* parameter_index */, float /* new_value */) override
      {
         auto callback = [self = SafePointer<BypassButton> (this)] {
            if (self == nullptr) {
               LOG_WARN ("SafePointer<BypassButton> was nullptr: {}: {}", __FILE__, __LINE__);
               return;
            }
            if (auto* bypass_param = static_cast<juce::AudioParameterBool*> (
                   self->m_processor.getBypassParameter ())) {
               self->setToggleState (static_cast<bool> (bypass_param->get ()),
                                     juce::dontSendNotification);
               for (auto* node :
                    self->m_processor.getNodeGraph ().getAudioProcessorGraph ().getNodes ()) {
                  node->setBypassed (self->getToggleState ());
               }
            }
         };
         juce::MessageManager::callAsync (std::move (callback));
      }
      void parameterGestureChanged (int /* parameter_index */,
                                    bool /* gesture_is_starting */) override
      {}

   private:
      PluginProcessor& m_processor;
      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BypassButton)
   };

   TopMenubar::TopMenubar (PluginProcessor& processor) : m_processor (processor)
   {
      if (auto svg = juce::XmlDocument::parse (BinaryData::CONNECTRIXlogo_svg)) {
         auto drawable = juce::Drawable::createFromSVG (*svg);
         m_logo = drawable->getOutlineAsPath ();
      }

      m_bypass_button = std::make_unique<BypassButton> (m_processor);
      addAndMakeVisible (m_bypass_button.get ());

      auto& apvts = m_processor.getAPVTS ();
      for (auto parameter_id : m_processor.getParameterIds ()) {
         using namespace juce;
         if (apvts.getParameter (parameter_id.getParamID ()) == m_processor.getBypassParameter ()) {
            continue;
         }
         auto knob = std::make_unique<Slider> (Slider::SliderStyle::RotaryHorizontalVerticalDrag,
                                               Slider::TextEntryBoxPosition::NoTextBox);
         knob->setRange (0.0f, 127.0f, 1.0f);
         knob->setValue (0.0f);
         addAndMakeVisible (*knob);
         auto attachment = std::make_unique<APVTS::SliderAttachment> (
            m_processor.getAPVTS (), parameter_id.getParamID (), *knob);
         m_macro_knobs.emplace_back (std::make_pair (std::move (attachment), std::move (knob)));
      }
   }

   TopMenubar::~TopMenubar ()
   {
      jassert (juce::MessageManager::getInstance ()->isThisTheMessageThread ());

      if (auto* bypass_parameter = m_processor.getBypassParameter ()) {
         bypass_parameter->removeListener (this);
      }
      // attachments need to be deleted before the sliders.
      for (auto& [attatchment, slider] : m_macro_knobs) {
         attatchment.reset ();
         slider.reset ();
      }
   }

   void TopMenubar::paint (juce::Graphics& g)
   {
      using namespace juce;
      auto bounds = getLocalBounds ();
      g.setGradientFill (ColourGradient (Colour::fromRGB (194, 192, 190),
                                         bounds.getTopLeft ().toFloat (),
                                         Colour::fromRGB (194, 192, 190).darker (0.1f),
                                         bounds.getBottomRight ().toFloat (),
                                         true));
      g.fillAll ();
      g.setColour (Colour::fromRGB (46, 46, 45));
      g.fillRect (bounds.removeFromBottom (2));

      bounds.removeFromLeft (50);
      m_logo.applyTransform (
         m_logo.getTransformToScaleToFit (bounds.removeFromLeft (150).toFloat (), true));
      g.setColour (Colour::fromRGB (19, 19, 19));
      g.fillPath (m_logo);
   }

   void TopMenubar::resized ()
   {
      auto bounds = getBounds ();
      bounds.removeFromBottom (2);
      bounds.removeFromRight (50);
      for (auto& [a, knob] : m_macro_knobs) {
         juce::ignoreUnused (a);
         auto knob_bounds = bounds.removeFromRight (bounds.getHeight ());
         knob->setBounds (knob_bounds.reduced (1));
         bounds.removeFromRight (knob_bounds.getWidth () / 2);
      }
      if (m_bypass_button) {
         m_bypass_button->setBounds (bounds.removeFromRight (90));
         bounds.removeFromRight (10);
      }
   }

   void TopMenubar::parameterValueChanged (int parameter_index, float new_value)
   {
      auto get_parameter = [&] () -> juce::AudioProcessorParameter* {
         for (auto* parameter : m_processor.getParameters ()) {
            if (parameter->getParameterIndex () == parameter_index) {
               return parameter;
            }
         }
         return nullptr;
      };

      auto* parameter = get_parameter ();
      if (parameter == nullptr) {
         return;
      }

      auto* bypass_parameter = m_processor.getBypassParameter ();
      if (bypass_parameter && bypass_parameter == parameter) {
         LOG_DEBUG ("Bypass Param Clicked: {}", new_value);
         m_bypass_button->setToggleState (static_cast<bool> (new_value), juce::dontSendNotification);
      }
   }

   //================================================================================
   ProcessorEditor::ProcessorEditor (PluginProcessor& p)
      : AudioProcessorEditor (&p), m_processer (p), m_nodeGraphEditor (p.getNodeGraph ()),
        m_topbar (p), m_tooltip_window (juce::TooltipWindow (this))
   {
      m_ctx.setComponentPaintingEnabled (true);
      m_ctx.setMultisamplingEnabled (true);
      m_ctx.attachTo (*this);

      addAndMakeVisible (&m_nodeGraphEditor);
      addAndMakeVisible (&m_demo_label);
      addAndMakeVisible (&m_topbar);

      LookAndFeel::setDefaultLookAndFeel (&m_laf);

      juce::MessageManager::callAsync ([this] {
         resized ();
         repaint ();
      });

      setSize (1000, 500);

      // m_paramater_window = std::make_unique<juce::DocumentWindow> (
      //    "Plugin Paramaters",
      //    m_laf.findColour (juce::ResizableWindow::ColourIds::backgroundColourId),
      //    juce::DocumentWindow::TitleBarButtons::closeButton);
      // m_paramater_window->setContentOwned (new juce::GenericAudioProcessorEditor (p), true);
      // m_paramater_window->setAlwaysOnTop (true);
      // m_paramater_window->setVisible (true);
   }

   ProcessorEditor::~ProcessorEditor ()
   {
      // LookAndFeel::setDefaultLookAndFeel (nullptr);
   }

   //================================================================================
   void ProcessorEditor::paint (juce::Graphics& g)
   {
      // (Our component is opaque, so we must completely fill the background with a solid colour)
      using namespace juce;
      g.fillAll (Colour::fromRGB (19, 19, 19));
   }

   void ProcessorEditor::resized ()
   {
      auto bounds = getBounds ();
      m_topbar.setBounds (bounds.removeFromTop (38));
      m_demo_label.setBounds (bounds.removeFromBottom (20));
      m_nodeGraphEditor.setBounds (bounds);
   }
} // namespace cntx