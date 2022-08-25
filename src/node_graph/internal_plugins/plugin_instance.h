#pragma once

namespace cntx::node_graph
{

   class InternalPluginInstance: public juce::AudioPluginInstance
   {
   public:
      explicit InternalPluginInstance (Box<juce::AudioProcessor> innerIn)
         : inner (std::move (innerIn))
      {
         jassert (inner != nullptr);

         for (auto isInput : { true, false })
            matchChannels (isInput);
         setBusesLayout (inner->getBusesLayout ());
      }

      const juce::String getName () const override { return inner->getName (); }
      juce::StringArray getAlternateDisplayNames () const override
      {
         return inner->getAlternateDisplayNames ();
      }
      double getTailLengthSeconds () const override { return inner->getTailLengthSeconds (); }
      bool acceptsMidi () const override { return inner->acceptsMidi (); }
      bool producesMidi () const override { return inner->producesMidi (); }
      juce::AudioProcessorEditor* createEditor () override { return inner->createEditor (); }
      bool hasEditor () const override { return inner->hasEditor (); }
      int getNumPrograms () override { return inner->getNumPrograms (); }
      int getCurrentProgram () override { return inner->getCurrentProgram (); }
      void setCurrentProgram (int i) override { inner->setCurrentProgram (i); }
      const juce::String getProgramName (int i) override { return inner->getProgramName (i); }
      void changeProgramName (int i, const juce::String& n) override
      {
         inner->changeProgramName (i, n);
      }
      void getStateInformation (juce::MemoryBlock& b) override { inner->getStateInformation (b); }
      void setStateInformation (const void* d, int s) override
      {
         inner->setStateInformation (d, s);
      }
      void getCurrentProgramStateInformation (juce::MemoryBlock& b) override
      {
         inner->getCurrentProgramStateInformation (b);
      }
      void setCurrentProgramStateInformation (const void* d, int s) override
      {
         inner->setCurrentProgramStateInformation (d, s);
      }
      void prepareToPlay (double sr, int bs) override
      {
         inner->setRateAndBufferSizeDetails (sr, bs);
         inner->prepareToPlay (sr, bs);
      }
      void releaseResources () override { inner->releaseResources (); }
      void memoryWarningReceived () override { inner->memoryWarningReceived (); }
      void processBlock (juce::AudioBuffer<float>& a, juce::MidiBuffer& m) override
      {
         inner->processBlock (a, m);
      }
      void processBlock (juce::AudioBuffer<double>& a, juce::MidiBuffer& m) override
      {
         inner->processBlock (a, m);
      }
      void processBlockBypassed (juce::AudioBuffer<float>& a, juce::MidiBuffer& m) override
      {
         inner->processBlockBypassed (a, m);
      }
      void processBlockBypassed (juce::AudioBuffer<double>& a, juce::MidiBuffer& m) override
      {
         inner->processBlockBypassed (a, m);
      }
      bool supportsDoublePrecisionProcessing () const override
      {
         return inner->supportsDoublePrecisionProcessing ();
      }
      bool supportsMPE () const override { return inner->supportsMPE (); }
      bool isMidiEffect () const override { return inner->isMidiEffect (); }
      void reset () override { inner->reset (); }
      void setNonRealtime (bool b) noexcept override { inner->setNonRealtime (b); }
      void refreshParameterList () override { inner->refreshParameterList (); }
      void numChannelsChanged () override { inner->numChannelsChanged (); }
      void numBusesChanged () override { inner->numBusesChanged (); }
      void processorLayoutsChanged () override { inner->processorLayoutsChanged (); }
      void setPlayHead (juce::AudioPlayHead* p) override { inner->setPlayHead (p); }
      void updateTrackProperties (const TrackProperties& p) override
      {
         inner->updateTrackProperties (p);
      }
      bool isBusesLayoutSupported (const BusesLayout& layout) const override
      {
         return inner->checkBusesLayoutSupported (layout);
      }
      bool canAddBus (bool) const override { return true; }
      bool canRemoveBus (bool) const override { return true; }

      //==============================================================================
      void fillInPluginDescription (juce::PluginDescription& description) const override
      {
         description = getPluginDescription (*inner);
      }

      static juce::PluginDescription getPluginDescription (const juce::AudioProcessor& proc);
   private:
      
      void matchChannels (bool isInput)
      {
         const auto inBuses = inner->getBusCount (isInput);

         while (getBusCount (isInput) < inBuses)
            addBus (isInput);

         while (inBuses < getBusCount (isInput))
            removeBus (isInput);
      }

      Box<AudioProcessor> inner;

      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InternalPluginInstance)
   };

} // namespace cntx::node_graph
