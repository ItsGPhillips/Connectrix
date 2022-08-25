#include "plugin_format.h"
#include "../node_graph.h"
#include "node_graph/internal_plugins/plugin_instance.h"
#include "range/v3/action/take.hpp"

namespace cntx::node_graph
{

   //==============================================================================

   InternalPluginFormat::InternalPluginFormat ()
      : m_factory {
           [] {
              return std::make_unique<NodeGraph::IOProcessor> (
                 NodeGraph::IOProcessor::IODeviceType::audioInputNode);
           },
           [] {
              return std::make_unique<NodeGraph::IOProcessor> (
                 NodeGraph::IOProcessor::IODeviceType::audioOutputNode);
           },
           [] {
              return std::make_unique<NodeGraph::IOProcessor> (
                 NodeGraph::IOProcessor::IODeviceType::midiInputNode);
           },
           [] {
              return std::make_unique<NodeGraph::IOProcessor> (
                 NodeGraph::IOProcessor::IODeviceType::midiOutputNode);
           },
        }
   {}

   void InternalPluginFormat::createPluginInstance (
      const juce::PluginDescription& desc,
      double /*initialSampleRate*/,
      int /*initialBufferSize*/,
      juce::AudioPluginFormat::PluginCreationCallback callback)
   {
      if (auto p = createInstance (desc.uniqueId)) {
         callback (std::move (p), {});
      } else {
         callback (nullptr, NEEDS_TRANS ("Invalid internal plugin Id"));
      }
   }

   bool InternalPluginFormat::fileMightContainThisPluginType (const juce::String&) { return true; }
   juce::FileSearchPath InternalPluginFormat::getDefaultLocationsToSearch () { return {}; }
   bool InternalPluginFormat::canScanForPlugins () const { return false; }
   bool InternalPluginFormat::isTrivialToScan () const { return true; }
   void InternalPluginFormat::findAllTypesForFile (juce::OwnedArray<juce::PluginDescription>&,
                                                   const juce::String&) {};
   bool InternalPluginFormat::doesPluginStillExist (const juce::PluginDescription&) { return true; }
   juce::String InternalPluginFormat::getNameOfPluginFromIdentifier (const juce::String& f)
   {
      return f;
   }
   juce::StringArray InternalPluginFormat::searchPathsForPlugins (const juce::FileSearchPath&,
                                                                  bool,
                                                                  bool)
   {
      return {};
   }

   bool InternalPluginFormat::pluginNeedsRescanning (const juce::PluginDescription&)
   {
      return false;
   }

   Box<juce::AudioPluginInstance> InternalPluginFormat::createInstance (const i32 id)
   {
      return m_factory.createInstance (id);
   }

   //==============================================================================

   InternalPluginFormat::Factory::Factory (const std::initializer_list<Constructor>& constructors)
      : m_constructors (constructors), m_descriptions ([&] {
           Vec<juce::PluginDescription> result;
           for (const auto& constructor : constructors) {
              auto desc =  constructor ()->getPluginDescription ();
              result.push_back (desc);
              LOG_DEBUG("Added Plugin-Id: {}", static_cast<u32>(desc.uniqueId));
           }
           return result;
        }())
   {}

   Box<juce::AudioPluginInstance> InternalPluginFormat::Factory::createInstance (const i32 id) const
   {
      // clang-format off
      auto view = m_descriptions
                  | ranges::views::enumerate
                  | ranges::views::filter ([&] (const auto& rng) { return id == rng.second.uniqueId; });

      // clang-format on

      if (!view.empty ()) {
         auto&& [idx, _] = view.front ();
         LOG_INFO("Called Contructor");
         return m_constructors[idx]();
      }

      LOG_ERROR ("Tried to create plugin instance with id={} but no contructor existed for id={}",
                 id);
      return {};
   }

} // namespace cntx::node_graph