#include "node_graph.h"
#include "../data_tree.h"
#include "../plugin_processor.h"
#include "../settings.h"
#include "editor/node/node.h"
#include "internal_plugins/plugin_format.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "logger.h"
#include "pch.h"
#include <cstddef>

namespace cntx::node_graph
{
   PluginWindow::PluginWindow (

      juce::AudioProcessorEditor* pluginEditorComponent)
      : juce::DocumentWindow (pluginEditorComponent->getAudioProcessor ()->getName (),
                              juce::Colours::black,
                              DocumentWindow::closeButton)
   {
      setResizable (pluginEditorComponent->isResizable (), false);
      setContentOwned (pluginEditorComponent, true);
      setUsingNativeTitleBar (true);
      setInterceptsMouseClicks (true, true);
      setMouseClickGrabsKeyboardFocus (true);
   }

   PluginWindow::~PluginWindow () { clearContentComponent (); }

   void PluginWindow::closeButtonPressed () { setVisible (false); }

   // NodeGraph ===========================================================
   namespace idents
   {
      CREATE_IDENT (source_id)
      CREATE_IDENT (source_channel)
      CREATE_IDENT (destination_id)
      CREATE_IDENT (destination_channel)
   } // namespace idents

   NodeGraph::NodeGraph (cntx::PluginProcessor* processor)
      : m_processor (processor), m_value_tree (juce::ValueTree (idents::GRAPH))
   {
      m_audio_processor_graph.addListener (this);
      addChangeListener (m_processor);
      m_value_tree.addListener (this);
      LOG_DEBUG ("NodeGraph::NodeGraph ()");
   }
   NodeGraph::~NodeGraph ()
   {
      m_audio_processor_graph.removeListener (this);
      removeChangeListener (m_processor);
      m_value_tree.removeListener (this);
   }

   void NodeGraph::addAudioPluginNode (const juce::PluginDescription description,
                                       const juce::Point<f32> pos,
                                       NodeType node_type,
                                       Option<IONodeType> io_type)
   {
      using PluginLoaderInfo = GlobalStateManager::AudioPluginManager::PluginLoaderInfo;
      using PluginLoaderResult = GlobalStateManager::AudioPluginManager::PluginLoaderResult;

      auto callback = [&, pos, description] (PluginLoaderResult result) {
         result.visit (
            [&, pos] (Ok<Box<juce::AudioPluginInstance>>& instance) {
               instance->enableAllBuses ();
               createNodeImpl (std::move (*instance), pos, node_type, io_type);
            },
            [] (Err<juce::String>& err) {
               juce::ignoreUnused (err);
               LOG_ERROR ("Plugin Loader Error: {}", *err);
            });
      };

      LOG_INFO ("{}", description.createXml ()->toString ());

      GlobalStateManager::getPluginManager ().createPluginInstance (PluginLoaderInfo {
         .description = description,
         .sampleRate = m_audio_processor_graph.getSampleRate (),
         .blockSize = m_audio_processor_graph.getBlockSize (),
         .callback = callback,
      });
   }

   void NodeGraph::removeNode (NodeId id)
   {
      if (auto* node_ptr = m_audio_processor_graph.getNodeForId (id)) {
         m_audio_processor_graph.removeNode (node_ptr);
         auto findWindow = [id] (const PtrWindowPair& p) {
            const auto& [ptr, window] = p;
            return ptr->nodeID == id;
         };
         m_plugin_windows.erase (
            std::remove_if (m_plugin_windows.begin (), m_plugin_windows.end (), findWindow),
            m_plugin_windows.end ());
      } else {
         LOG_ERROR ("Invalid ID");
      }
   }

   void NodeGraph::tryAndMakeConnection (Connection connection)
   {
      using namespace cntx::node_graph;

      std::optional<Socket::NodeAndChannel> from = {};
      std::visit (overloaded {
                     [&] (Socket::AudioChannel& socket) { from = socket.node_and_channel; },
                     [&] (Socket::MidiChannel& socket) { from = socket.node_and_channel; },
                     [] (auto&) {},
                  },
                  connection.fromSocket->type);
      std::optional<Socket::NodeAndChannel> to = {};
      std::visit (overloaded {
                     [&] (Socket::AudioChannel& socket) { to = socket.node_and_channel; },
                     [&] (Socket::MidiChannel& socket) { to = socket.node_and_channel; },
                     [] (auto&) {},
                  },
                  connection.toSocket->type);
      if (from.has_value () && to.has_value ()) {
         createAudioProcessorChannelConnection (from.value (), to.value ());
      }
   }

   juce::AudioProcessorGraph& NodeGraph::getAudioProcessorGraph ()
   {
      return m_audio_processor_graph;
   }
   const juce::AudioProcessorGraph& NodeGraph::getAudioProcessorGraph () const
   {
      return m_audio_processor_graph;
   }

   juce::PluginDescription NodeGraph::getDescriptionFromId (NodeGraph::NodeId id)
   {
      if (auto* node = getAudioProcessorGraph ().getNodeForId (id)) {
         if (auto* plugin = dynamic_cast<juce::AudioPluginInstance*> (node->getProcessor ())) {
            return plugin->getPluginDescription ();
         }
      }
      return {};
   }

   PluginWindow* NodeGraph::getOrCreateWindowForId (NodeGraph::NodeId id)
   {
      auto* ptr = getAudioProcessorGraph ().getNodeForId (id);
      for (auto& [p, window] : m_plugin_windows) {
         if (p.get () == ptr) {
            return window.get ();
         }
      }
      if (auto* processor = ptr->getProcessor ()) {
         if (auto* plugin = dynamic_cast<juce::AudioPluginInstance*> (processor)) {
            auto description = plugin->getPluginDescription ();
            if (plugin->hasEditor ()) {
               auto* editor = plugin->createEditorIfNeeded ();
               auto window = std::make_unique<PluginWindow> (editor);

               window->setCentrePosition (juce::Desktop::getMousePosition ());
               window->setAlwaysOnTop (true);

               auto* windowPtr = window.get ();
               m_plugin_windows.emplace_back (std::make_pair (std::move (ptr), std::move (window)));
               return windowPtr;
            }
         }
      }
      return nullptr;
   }

   void NodeGraph::valueTreeChildRemoved (juce::ValueTree& parent_tree,
                                          juce::ValueTree& child_tree,
                                          int /*position*/)
   {
      auto& graph = m_audio_processor_graph;

      if (parent_tree == m_value_tree) {
         if (child_tree.hasType (idents::CONNECTION)) {
            auto source_socket = child_tree.getChild (0);
            jassert (source_socket.isValid ());
            i32 source_id = static_cast<i32> (source_socket[idents::id]);
            i32 source_channel = static_cast<i32> (source_socket[idents::channel]);

            auto destination_socket = child_tree.getChild (1);
            jassert (destination_socket.isValid ());
            i32 destination_id = static_cast<i32> (destination_socket[idents::id]);
            i32 destination_channel = static_cast<i32> (destination_socket[idents::channel]);

            for (auto& connection : graph.getConnections ()) {
               if (connection.source.nodeID.uid == static_cast<u32> (source_id)
                   && connection.source.channelIndex == source_channel
                   && connection.destination.nodeID.uid == static_cast<u32> (destination_id)
                   && connection.destination.channelIndex == destination_channel) {
                  graph.removeConnection (connection);
                  return;
               }
            }
            return;
         }

         if (child_tree.hasType (idents::NODE)) {
            i32 id = child_tree[idents::id];
            auto node_id = NodeGraph::NodeId (static_cast<u32> (id));
            static Vec<juce::ValueTree> VTS_TO_REMOVE = {};
            if (auto* node = graph.getNodeForId (node_id)) {
               for (auto vt : parent_tree) {
                  jassert (vt.isValid ());
                  if (vt.getType () != idents::CONNECTION) {
                     continue;
                  }
                  auto source_socket = vt.getChild (0);
                  jassert (source_socket.isValid ());
                  i32 source_id = static_cast<i32> (source_socket[idents::id]);
                  auto destination_socket = vt.getChild (1);
                  jassert (destination_socket.isValid ());
                  i32 destination_id = static_cast<i32> (destination_socket[idents::id]);
                  if (source_id == id || destination_id == id) {
                     VTS_TO_REMOVE.emplace_back (vt);
                  }
               }
               for (auto vt : VTS_TO_REMOVE) {
                  parent_tree.removeChild (vt, nullptr);
               }
               VTS_TO_REMOVE.clear ();

               // Make sure Connections are before the Node
               removeNode (node_id);
            } else {
               // node id didnt return a node.
               jassertfalse;
            }
         }
      }
   }

   void NodeGraph::audioProcessorParameterChanged (juce::AudioProcessor*, i32, f32) {}
   void NodeGraph::audioProcessorChanged (juce::AudioProcessor* /*processor*/,
                                          const juce::AudioProcessorListener::ChangeDetails& details)
   {
      LOG_DEBUG ("Audio Processer Changed Callback");
      if (details.latencyChanged) {
         sendChangeMessage ();
      }
   }

   Box<juce::XmlElement> NodeGraph::create_xml ()
   {
      if (!m_value_tree.isValid ()) {
         return {};
      }
      jassert (m_value_tree.hasType (idents::GRAPH));

      auto root = std::make_unique<juce::XmlElement> ("GRAPH");
      auto nodes = std::make_unique<juce::XmlElement> ("NODES");
      auto connections = std::make_unique<juce::XmlElement> ("CONNECTIONS");

      for (auto vt : m_value_tree) {
         jassert (vt.isValid ());
         if (vt.hasType (idents::NODE)) {
            i32 id = vt.getProperty (idents::id);

            // clang-format off
            juce::Point<f32> pos = juce::VariantConverter<juce::Point<f32>>::fromVar (vt.getProperty (idents::position));
            const auto plugin_id = getDescriptionFromId (NodeId (static_cast<u32> (id))).createIdentifierString ();
            const auto type = static_cast<i32>(vt.getProperty((idents::type)));
            // clang-format on

            auto node = std::make_unique<juce::XmlElement> ("NODE");
            node->setAttribute (idents::id, id);
            node->setAttribute (idents::plugin_id, plugin_id);
            node->setAttribute (idents::position, juce::Base64::toBase64 (&pos, sizeof (pos)));
            node->setAttribute (idents::type, type);

            if (type == static_cast<i32> (NodeGraph::NodeType::IO)) {
               const auto io_type = static_cast<i32> (vt.getProperty ((idents::io_type)));
               node->setAttribute (idents::io_type, io_type);
            }

            nodes->addChildElement (node.release ());
         }
         if (vt.hasType (idents::CONNECTION)) {
            auto get_meta = [&] (i32 idx) -> std::pair<i32, i32> {
               auto con_vt = vt.getChild (idx);
               jassert (con_vt.isValid ());
               i32 id = con_vt.getProperty (idents::id);
               i32 channel = con_vt.getProperty (idents::channel);
               return std::make_pair (id, channel);
            };

            auto [src_id, src_channel] = get_meta (0);
            auto [dest_id, dest_channel] = get_meta (1);

            auto connection = std::make_unique<juce::XmlElement> ("CONNECTION");
            connection->setAttribute (idents::source_id, src_id);
            connection->setAttribute (idents::source_channel, src_channel);
            connection->setAttribute (idents::destination_id, dest_id);
            connection->setAttribute (idents::destination_channel, dest_channel);
            connections->addChildElement (connection.release ());
         }
      }
      root->addChildElement (nodes.release ());
      root->addChildElement (connections.release ());
      return root;
   }

   // void NodeGraph::load_from_xml (Box<juce::XmlElement> xml)
   void NodeGraph::load_from_xml (juce::XmlElement* xml)
   {
      if (auto* nodes = xml->getChildByName ("NODES")) {
         for (auto* node : nodes->getChildIterator ()) {
            auto plugin_id = node->getStringAttribute (idents::plugin_id);
            auto position64 = node->getStringAttribute (idents::position);
            auto type = node->getIntAttribute (idents::type, -1);
            jassert (plugin_id.isNotEmpty ());
            jassert (position64.isNotEmpty ());
            jassert (type != -1);

            auto stream = juce::MemoryOutputStream (sizeof (juce::Point<f32>));

            if (auto desc = GlobalStateManager::getPluginManager ()
                               .getKnownPluginList ()
                               .getTypeForIdentifierString (plugin_id)) {
               if (!juce::Base64::convertFromBase64 (stream, position64)) {
                  throw std::runtime_error ("Couldn't convert base64  tring");
               }
               const void* data = stream.getData ();
               jassert (sizeof (juce::Point<f32>) == stream.getDataSize ());
               juce::Point<f32> p;
               std::memcpy (&p, data, sizeof (juce::Point<f32>));

               LOG_DEBUG ("Adding Audio node: {}", desc->name);

               Option<IONodeType> io_type;
               if (static_cast<NodeType> (type) == NodeType::IO) {
                  auto raw_io_type = node->getIntAttribute (idents::io_type, -1);
                  jassert (raw_io_type != -1);
                  io_type.emplace (static_cast<IONodeType> (raw_io_type));
               }

               addAudioPluginNode (*desc, p, static_cast<NodeType> (type), io_type);
            } else {
               LOG_ERROR ("Unable to load {} from the plugin list", plugin_id);
            }
         }
      }
      if (auto* connections = xml->getChildByName ("CONNECTIONS")) {
         LOG_DEBUG ("Has Connections");
         for (auto* connection : connections->getChildIterator ()) {
            std::array<i32, 4> meta;
            meta[0] = connection->getIntAttribute (idents::source_id, -1);
            meta[1] = connection->getIntAttribute (idents::source_channel, -1);
            meta[2] = connection->getIntAttribute (idents::destination_id, -1);
            meta[3] = connection->getIntAttribute (idents::destination_channel, -1);
            if (!std::ranges::any_of (meta, [] (const auto& elem) { return elem == -1; })) {
               LOG_DEBUG (" - createing connection");
               createAudioProcessorChannelConnection (
                  { NodeId (static_cast<u32> (meta[0])), meta[1] },
                  { NodeId (static_cast<u32> (meta[2])), meta[3] });
            } else {
               LOG_CRITICAL ("Bad connection data");
            }
         }
      }

      juce::MessageManager::callAsync (
         [&] { LOG_DEBUG ("loaded : \n{}", create_xml ()->toString ()); });
   }

   NodeGraph::NodeId NodeGraph::getNextUID () noexcept
   {
      return NodeGraph::NodeId (++(lastUID.uid));
   }

   void NodeGraph::createNodeImpl (Box<juce::AudioPluginInstance> processor,
                                   juce::Point<f32> pos,
                                   NodeType node_type,
                                   Option<IONodeType> io_type)
   {
      jassert (processor);
      if (auto node_ptr = m_audio_processor_graph.addNode (std::move (processor), getNextUID ())) {
         node_ptr->getProcessor ()->enableAllBuses ();
         auto new_node = juce::ValueTree (idents::NODE);

         new_node.setProperty (
            idents::position, juce::VariantConverter<juce::Point<f32>>::toVar (pos), nullptr);
         new_node.setProperty (idents::id, static_cast<juce::int64> (node_ptr->nodeID.uid), nullptr);
         new_node.setProperty (idents::type, static_cast<juce::int64> (node_type), nullptr);

         if (node_type == NodeType::IO) {
            jassert (io_type.has_value ());
            new_node.setProperty (
               idents::io_type, static_cast<juce::int64> (io_type.value ()), nullptr);
         }

         new_node.setProperty (idents::selected, true, nullptr);
         juce::MessageManager::callAsync (
            [this, new_node] () mutable { m_value_tree.appendChild (new_node, nullptr); });
      } else {
         // used a duplicate nodeId
         ASSERT_FALSE ("Used a duplicate nodeId");
      }
   }

   void NodeGraph::createAudioProcessorChannelConnection (NodeAndChannel from, NodeAndChannel to)
   {
      if (m_audio_processor_graph.addConnection (AudioProccesorConnection { from, to })) {
         auto makeSocketValueTree = [] (juce::Identifier type, NodeAndChannel nac) {
            juce::ValueTree vt (type);
            vt.setProperty (
               idents::id, juce::var (static_cast<juce::int64> (nac.nodeID.uid)), nullptr);
            vt.setProperty (
               idents::channel, juce::var (static_cast<juce::int64> (nac.channelIndex)), nullptr);

            return vt;
         };
         juce::ValueTree connection (
            idents::CONNECTION,
            {
               // the connection type
               juce::NamedValueSet::NamedValue (
                  idents::type, static_cast<juce::int64> (Connection::Type::AudioMidi)),
               // is the connection selected
               juce::NamedValueSet::NamedValue (idents::selected, false),
            },
            {
               // Ordering here is important! from -> to
               makeSocketValueTree (idents::SOCKET, from),
               makeSocketValueTree (idents::SOCKET, to),
            });
         juce::MessageManager::callAsync (
            [=, this] { m_value_tree.appendChild (connection, nullptr); });
      }
   }

   // PluginLoaderThread::PluginLoaderThread () : juce::Thread ("PluginLoaderThread")
   // {
   //    m_plugin_infos.reserve (16);
   //    setPriority (5);
   //    startThread ();
   // }

   // PluginLoaderThread::~PluginLoaderThread () { stopThread (1000); }

   // void PluginLoaderThread::loadPlugin (PluginLoaderInfo info)
   // {
   //    auto cs = juce::ScopedLock (m_lock);
   //    m_plugin_infos.push_back (info);
   //    notify ();
   // }

   // void PluginLoaderThread::run ()
   // {
   //    while (!threadShouldExit ()) {
   //       wait (-1);
   //       auto guard = juce::ScopedLock (m_lock);
   //       for (auto& info : m_plugin_infos) {
   //          GlobalStateManager::getPluginManager ().createPluginInstance (info);
   //       }
   //       m_plugin_infos.clear ();
   //    }
   // }
   //==============================================================================
} // namespace cntx::node_graph