#pragma once

#include "look_and_feel.h"
#include "node_graph/editor/editor.h"
#include "plugin_processor.h"

namespace cntx
{
   //==============================================================================
   class DemoLabel: public juce::Component
   {
   public:
      DemoLabel ();
      void paint (juce::Graphics&) override;

   private:
      juce::String m_label_text;

      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoLabel)
   };

   class TopMenubar: public juce::Component,
                     public juce::AudioProcessorParameter::Listener
   {
   public:
      TopMenubar (PluginProcessor& processor);
      ~TopMenubar () override;
      void paint (juce::Graphics&) override;
      void resized () override;

      void parameterValueChanged (int, float) override;
      void parameterGestureChanged (int /* parameterIndex */, bool /* gestureIsStarting */) override
      {}

   private:
      using APVTS = juce::AudioProcessorValueTreeState;
      PluginProcessor& m_processor;
      juce::Path m_logo;
      Box<juce::ToggleButton> m_bypass_button;

      Vec<std::pair<Box<APVTS::SliderAttachment>, Box<juce::Slider>>> m_macro_knobs;

      std::atomic<bool> m_is_bypassed = false;

      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TopMenubar)
   };

   class ProcessorEditor: public juce::AudioProcessorEditor
   {
   public:
      explicit ProcessorEditor (PluginProcessor&);
      ~ProcessorEditor () override;

      //==============================================================================
      void paint (juce::Graphics&) override;
      void resized () override;

   private:
      [[maybe_unused]] PluginProcessor& m_processer;
      LookAndFeel m_laf;

      node_graph::NodeGraphEditor m_nodeGraphEditor;
      TopMenubar m_topbar;
      DemoLabel m_demo_label;

      juce::OpenGLContext m_ctx;
      juce::TooltipWindow m_tooltip_window;

      Box<juce::DocumentWindow> m_paramater_window;

      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorEditor)
   };
} // namespace cntx