#pragma once

#include "node_graph/node_graph.h"

namespace cntx
{
   class BidirectionalMessageQueue
   {
   };

   //==============================================================================
   class PluginProcessor: public juce::AudioProcessor,
                          public juce::ChangeListener
   {
   public:
      //==============================================================================
      PluginProcessor ();
      ~PluginProcessor () override;

      //==============================================================================
      void prepareToPlay (double sampleRate, int samplesPerBlock) override;
      void releaseResources () override;

      bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

      void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
      using AudioProcessor::processBlock;
      void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
      using AudioProcessor::processBlockBypassed;

      //==============================================================================
      juce::AudioProcessorEditor* createEditor () override;
      bool hasEditor () const override;

      //==============================================================================
      const juce::String getName () const override;

      bool acceptsMidi () const override;
      bool producesMidi () const override;
      bool isMidiEffect () const override;
      double getTailLengthSeconds () const override;

      //==============================================================================
      int getNumPrograms () override;
      int getCurrentProgram () override;
      void setCurrentProgram (int index) override;
      const juce::String getProgramName (int index) override;
      void changeProgramName (int index, const juce::String& newName) override;

      //=============================================================================
      juce::AudioProcessorParameter* getBypassParameter () const override;
      void refreshParameterList () override;

      //=============================================================================
      void getStateInformation (juce::MemoryBlock& destData) override;
      void setStateInformation (const void* data, int sizeInBytes) override;

      //=============================================================================
      node_graph::NodeGraph& getNodeGraph () noexcept { return m_graph; };

      void changeListenerCallback (juce::ChangeBroadcaster*) override;

      juce::AudioProcessorValueTreeState& getAPVTS () { return m_apvts; }
      Vec<juce::ParameterID>& getParameterIds () { return m_param_ids; }

   private:
      //==============================================================================
      node_graph::NodeGraph m_graph;

      Vec<juce::ParameterID> m_param_ids;

      juce::AudioProcessorValueTreeState m_apvts;
      juce::AudioProcessorValueTreeState::ParameterLayout create_paramaters ();

      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
   };
} // namespace cntx