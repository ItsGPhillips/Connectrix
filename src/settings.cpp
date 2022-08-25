#include "settings.h"
#include "node_graph/internal_plugins/plugin_format.h"

namespace cntx
{
   JUCE_IMPLEMENT_SINGLETON (GlobalStateManager)
   GlobalStateManager::~GlobalStateManager () { clearSingletonInstance (); }

   GlobalStateManager::AudioPluginManager& GlobalStateManager::getPluginManager ()
   {
      auto* gsm = GlobalStateManager::getInstance ();
      return gsm->m_audio_plugin_manager;
   }

   juce::ApplicationProperties& GlobalStateManager::getAppProperties ()
   {
      auto* gsm = GlobalStateManager::getInstance ();
      return gsm->m_props;
   }

   void GlobalStateManager::changeListenerCallback (juce::ChangeBroadcaster* source) {}

   GlobalStateManager::GlobalStateManager () : m_audio_plugin_manager (m_props) {}

   void GlobalStateManager::AudioPluginManager::changeListenerCallback (
      juce::ChangeBroadcaster* source)
   {
      if (source == &m_plugin_list) {
         LOG_TRACE ("plugin lists changed callback");
         if (auto saved_plugin_list = m_plugin_list.createXml ()) {
            getAppProperties ().getCommonSettings (true)->setValue ("PLUGINLIST",
                                                                    saved_plugin_list.get ());
         }
      }
      getAppProperties ().saveIfNeeded ();
   }

   GlobalStateManager::AudioPluginManager::AudioPluginManager (juce::ApplicationProperties& props)
   {
      using namespace juce;

      m_plugin_format_manager.addDefaultFormats ();
      addPluginFormat (new node_graph::InternalPluginFormat ());

      m_plugin_list.addChangeListener (this);

      PropertiesFile::Options opts;
      opts.applicationName = "Connectrix";
      opts.commonToAllUsers = true;
      opts.filenameSuffix = "xml";
      opts.osxLibrarySubFolder = "Application Support";
      props.setStorageParameters (opts);

      auto desktop_directory = juce::File::getSpecialLocation (
         File::SpecialLocationType::userDesktopDirectory);

      if (auto* settings_file = props.getCommonSettings (true)) {
         if (auto savedPluginList = settings_file->getXmlValue ("PLUGINLIST")) {
            m_plugin_list.recreateFromXml (*savedPluginList);

            for (auto* format : m_plugin_format_manager.getFormats ()) {
               if (auto* internal_format = dynamic_cast<node_graph::InternalPluginFormat*> (
                      format)) {
                  internal_format->addToPluginList (m_plugin_list);
               }
            }

            m_plugin_list.sendChangeMessage ();
            return;
         }
         LOG_ERROR ("Failed to create plugin list");
      }
   }

   void GlobalStateManager::AudioPluginManager::createPluginInstance (const PluginLoaderInfo info)
   {
      Box<juce::AudioPluginInstance> instance;
      {
         auto guard = juce::ScopedLock (m_format_manager_lock);
         instance = m_plugin_format_manager.createPluginInstance (
            info.description, info.sampleRate, info.blockSize, error_message);
      }
      std::optional<PluginLoaderResult> res;
      if (instance) {
         res.emplace (Ok<Box<juce::AudioPluginInstance>> (std::move (instance)));
      } else {
         res.emplace (Err<juce::String> (error_message));
      }
      (info.callback) (std::move (res.value ()));
   }

   void GlobalStateManager::AudioPluginManager::addPluginFormat (juce::AudioPluginFormat* format)
   {
      LOG_DEBUG ("Adding Plugin Format: {}", format->getName ());
      auto guard = juce::ScopedLock (m_format_manager_lock);
      m_plugin_format_manager.addFormat (format);
   }

   juce::KnownPluginList& GlobalStateManager::AudioPluginManager::getKnownPluginList ()
   {
      return m_plugin_list;
   }

   juce::AudioPluginFormatManager& GlobalStateManager::AudioPluginManager::getFormatManager ()
   {
      return m_plugin_format_manager;
   }

   juce::File& GlobalStateManager::AudioPluginManager::getRecentlyCrashedPluginsFile ()
   {
      return m_recently_crashed_plugins;
   }
} // namespace cntx
