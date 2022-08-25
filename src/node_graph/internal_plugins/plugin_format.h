#pragma once

#include "node_graph/internal_plugins/plugin_instance.h"

namespace cntx::node_graph
{
   namespace builtin_plugin_idents
   {
      static const juce::Identifier AUDIO_IN = "Audio In";
      static const juce::Identifier AUDIO_OUT = "Audio Out";
      static const juce::Identifier MIDI_IN = "Midi In";
      static const juce::Identifier MIDI_OUT = "Midi In";
   } // namespace builtin_plugin_idents

   class InternalPluginFormat: public juce::AudioPluginFormat
   {
   public:
      //==============================================================================
      InternalPluginFormat ();

      [[nodiscard]] juce::String getName () const override { return IDENTIFIER.toString (); }

#pragma region overrides
      //==============================================================================
      [[nodiscard]] bool fileMightContainThisPluginType (const juce::String&) override;
      [[nodiscard]] juce::FileSearchPath getDefaultLocationsToSearch () override;
      [[nodiscard]] bool canScanForPlugins () const override;
      [[nodiscard]] bool isTrivialToScan () const override;
      void findAllTypesForFile (juce::OwnedArray<juce::PluginDescription>&,
                                const juce::String&) override;
      [[nodiscard]] bool doesPluginStillExist (const juce::PluginDescription&) override;
      [[nodiscard]] juce::String getNameOfPluginFromIdentifier (const juce::String&) override;
      [[nodiscard]] juce::StringArray searchPathsForPlugins (const juce::FileSearchPath&,
                                                             bool,
                                                             bool) override;
      [[nodiscard]] bool pluginNeedsRescanning (const juce::PluginDescription&) override;

#pragma endregion overrides

      //==============================================================================

      [[nodiscard]] static const juce::Identifier& getIdentifier () { return IDENTIFIER; }

      void addToPluginList (juce::KnownPluginList& kpl)
      {
         for (const auto& desc : m_factory.getDescriptions ()) {
            kpl.addType (desc);
         }
      }

   private:
      class Factory
      {
      public:
         // using Constructor = std::function<Box<juce::AudioPluginInstance> ()>;
         using Constructor = std::function<Box<juce::AudioPluginInstance>()>;
         explicit Factory (const std::initializer_list<Constructor>& constructors);
         [[nodiscard]] Box<juce::AudioPluginInstance> createInstance (i32) const;

         [[nodiscard]] const Vec<juce::PluginDescription>& getDescriptions () const
         {
            return m_descriptions;
         }

      private:
         // HashMap<i32, Constructor> m_constructors;
         Vec<Constructor> m_constructors;
         Vec<juce::PluginDescription> m_descriptions;
      };

      //==============================================================================
      void createPluginInstance (const juce::PluginDescription&,
                                 double,
                                 int,
                                 PluginCreationCallback) override;

      Box<juce::AudioPluginInstance> createInstance (i32 name);

      [[nodiscard]] bool requiresUnblockedMessageThreadDuringCreation (
         const juce::PluginDescription&) const override
      {
         return false;
      }

      static inline const juce::Identifier IDENTIFIER = { "Internal" };
      Factory m_factory;
   };
} // namespace cntx::node_graph
