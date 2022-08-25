#include <memory>
#include <mimalloc-new-delete.h>
#include <mimalloc.h>

#include "logger.h"
#include "plugin_editor.h"
#include "plugin_processor.h"
#include "range/v3/numeric/iota.hpp"
#include "range/v3/view/take.hpp"
#include "settings.h"

namespace cntx
{

   //==============================================================================
   PluginProcessor::PluginProcessor ()
      : AudioProcessor (BusesProperties ()
                           .withInput ("Input", juce::AudioChannelSet::stereo (), true)
                           .withInput ("Sidechain", juce::AudioChannelSet::stereo (), true)
                           .withOutput ("Output", juce::AudioChannelSet::stereo (), true)),
        m_graph (this), m_apvts (*this, nullptr, "PARAMATERS", create_paramaters ())
   {
      LOG_INFO ("MIMALLOC v{}", mi_version ());
      setLatencySamples (m_graph.getAudioProcessorGraph ().getLatencySamples ());
   }

   PluginProcessor::~PluginProcessor () {}

   //==============================================================================
   const juce::String PluginProcessor::getName () const { return JucePlugin_Name; }

   bool PluginProcessor::acceptsMidi () const { return true; }
   bool PluginProcessor::producesMidi () const { return true; }
   bool PluginProcessor::isMidiEffect () const { return true; }
   double PluginProcessor::getTailLengthSeconds () const { return 0.0; }
   int PluginProcessor::getNumPrograms ()
   {
      return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
   }
   int PluginProcessor::getCurrentProgram () { return 0; }
   void PluginProcessor::setCurrentProgram (int /* index */) {}
   const juce::String PluginProcessor::getProgramName (int /* index */) { return {}; }
   void PluginProcessor::changeProgramName (int /* index */, const juce::String& /* newName */) {}

   juce::AudioProcessorParameter* PluginProcessor::getBypassParameter () const
   {
      return m_apvts.getParameter ("BYPASS");
   }

   //==============================================================================
   void PluginProcessor::prepareToPlay (double sample_rate, int samples_per_block)
   {
      m_graph.getAudioProcessorGraph ().setPlayConfigDetails (getMainBusNumInputChannels (),
                                                              getMainBusNumOutputChannels (),
                                                              sample_rate,
                                                              samples_per_block);
      m_graph.getAudioProcessorGraph ().prepareToPlay (sample_rate, samples_per_block);
   }

   void PluginProcessor::releaseResources ()
   {
      m_graph.getAudioProcessorGraph ().releaseResources ();
   }

   bool PluginProcessor::isBusesLayoutSupported (const BusesLayout&) const { return true; }

   void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& midi_messages)
   {
      juce::ScopedNoDenormals no_demormals;
      m_graph.getAudioProcessorGraph ().setPlayHead (getPlayHead ());
      m_graph.getAudioProcessorGraph ().processBlock (buffer, midi_messages);
   }

   void PluginProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& midi_messages)
   {
      juce::ScopedNoDenormals no_demormals;
      m_graph.getAudioProcessorGraph ().setPlayHead (getPlayHead ());
      m_graph.getAudioProcessorGraph ().processBlock (buffer, midi_messages);
   }

   bool PluginProcessor::hasEditor () const { return true; }

   juce::AudioProcessorEditor* PluginProcessor::createEditor ()
   {
      using namespace juce;
      auto* editor = new ProcessorEditor (*this);
      editor->setResizeLimits (800, 300, 1920, 1080);
      editor->setResizable (true, true);
      return editor;
   }

   //================================================================================

   void PluginProcessor::refreshParameterList () {}

   //================================================================================
   void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
   {
      // You should use this method to store your parameters in the memory block.
      // You could do that either as raw data, or use the XML or ValueTree classes
      // as intermediaries to make it easy to save and load complex data.
      if (auto xml = getNodeGraph ().create_xml ()) {
         LOG_DEBUG ("Created XML : getStateInformation\n{}", xml->toString ());
         copyXmlToBinary (*xml, destData);
      }
   }

   void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
   {
      // You should use this method to restore your parameters from this memory block,
      // whose contents will have been created by the getStateInformation() call.

      if (auto boxed_xml = getXmlFromBinary (data, sizeInBytes)) {
         auto xml = Rc<juce::XmlElement>(boxed_xml.release());
         juce::MessageManager::callAsync ([&, xml = xml] {
            LOG_DEBUG ("Created XML : setStateInformation\n{}", xml->toString ());
            getNodeGraph ().load_from_xml (xml.get());
         });
      }
   }

   void PluginProcessor::changeListenerCallback (juce::ChangeBroadcaster* source)
   {
      if (source == &m_graph) {
         auto latency = m_graph.getAudioProcessorGraph ().getLatencySamples ();
         if (latency != getLatencySamples ()) {
            setLatencySamples (latency);
            LOG_DEBUG ("Processor Latency Updated: {} ms", latency);
         }
      }
   }

   juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::create_paramaters ()
   {
      m_param_ids.emplace_back ("MACRO_1");
      m_param_ids.emplace_back ("MACRO_2");
      m_param_ids.emplace_back ("MACRO_3");
      m_param_ids.emplace_back ("MACRO_4");
      m_param_ids.emplace_back ("MACRO_5");
      m_param_ids.emplace_back ("MACRO_6");

      juce::AudioProcessorValueTreeState::ParameterLayout layout;

      layout.add (std::make_unique<juce::AudioParameterBool> (
         juce::ParameterID ("BYPASS"), "bypass", false));

      for (auto& parameter_id : m_param_ids) {
         auto parameter_name = parameter_id.getParamID ().replaceCharacter ('_', ' ').toLowerCase ();
         layout.add (
            std::make_unique<juce::AudioParameterInt> (parameter_id, parameter_name, 0, 127, 0));
      }
      return layout;
   }
} // namespace cntx

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter () { return new cntx::PluginProcessor (); }