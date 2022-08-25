#pragma once

#include "node_graph/internal_plugins/plugin_format.h"

namespace cntx
{
   class GlobalStateManager: public juce::ChangeListener,
                             public juce::DeletedAtShutdown
   {
   public:
      JUCE_DECLARE_SINGLETON (GlobalStateManager, false)
      ~GlobalStateManager () override;

      class AudioPluginManager: public juce::ChangeListener
      {
      public:
         using PluginLoaderResult = Result<Box<juce::AudioPluginInstance>, juce::String>;
         struct PluginLoaderInfo
         {
            const juce::PluginDescription description;
            const double sampleRate;
            const int blockSize;
            const std::function<void (PluginLoaderResult)> callback;
         };

         AudioPluginManager (juce::ApplicationProperties& props);
         void createPluginInstance (const PluginLoaderInfo);

         void addPluginFormat (juce::AudioPluginFormat*);
         
         juce::KnownPluginList& getKnownPluginList ();
         juce::AudioPluginFormatManager& getFormatManager ();
         juce::File& getRecentlyCrashedPluginsFile ();

         void changeListenerCallback (juce::ChangeBroadcaster* source) override;

      private:
         juce::AudioPluginFormatManager m_plugin_format_manager;
         juce::File m_recently_crashed_plugins;
         juce::KnownPluginList m_plugin_list;

         juce::CriticalSection m_format_manager_lock;
         juce::CriticalSection m_plugin_list_lock;
         juce::String error_message;
      };

      static juce::ApplicationProperties& getAppProperties ();
      static AudioPluginManager& getPluginManager ();

      void changeListenerCallback (juce::ChangeBroadcaster* source) override;

   private:
      GlobalStateManager ();

      juce::ApplicationProperties m_props;
      AudioPluginManager m_audio_plugin_manager;
   };
} // namespace cntx
