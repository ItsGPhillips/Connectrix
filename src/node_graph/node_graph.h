#pragma once
#include "../settings.h"

namespace cntx
{
   class PluginProcessor;
}

namespace cntx::node_graph
{
   // forward decl;
   class Socket;
} // namespace cntx::node_graph

namespace cntx::node_graph
{
   namespace idents
   {
      namespace impl
      {
         CREATE_IDENT (NODES)
         CREATE_IDENT (CONNECTIONS)
      } // namespace impl

      CREATE_IDENT (GRAPH)
      CREATE_IDENT (NODE)
      CREATE_IDENT (CONNECTION)
      CREATE_IDENT (SOCKET)

      CREATE_IDENT (position)
      CREATE_IDENT (zoom)
      CREATE_IDENT (id)
      CREATE_IDENT (plugin_id)
      CREATE_IDENT (type)
      CREATE_IDENT (io_type)
      CREATE_IDENT (channel)
      CREATE_IDENT (selected)
   } // namespace idents

   // Plugin Window =================================================================
   class PluginWindow: public juce::DocumentWindow
   {
   public:
      PluginWindow (juce::AudioProcessorEditor*);
      virtual ~PluginWindow () override;
      void closeButtonPressed () override;

   private:
      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginWindow)
   };
   // ===============================================================================

   // class PluginLoaderThread: public juce::Thread
   // {
   // public:
   //    using PluginLoaderInfo = GlobalStateManager::AudioPluginManager::PluginLoaderInfo;

   //    PluginLoaderThread ();
   //    ~PluginLoaderThread () override;

   //    void loadPlugin (PluginLoaderInfo);

   //    void run () override;

   // private:
   //    juce::CriticalSection m_lock;
   //    Vec<PluginLoaderInfo> m_plugin_infos;
   // };

   class NodeGraph: public juce::ValueTree::Listener,
                    public juce::ChangeBroadcaster,
                    public juce::AudioProcessorListener
   {
   public:
      using IOProcessor = juce::AudioProcessorGraph::AudioGraphIOProcessor;
      using Node = juce::AudioProcessorGraph::Node;
      using NodeId = juce::AudioProcessorGraph::NodeID;
      using AudioProccesorConnection = juce::AudioProcessorGraph::Connection;
      using NodeAndChannel = juce::AudioProcessorGraph::NodeAndChannel;

      enum class NodeType : i32
      {
         ExternalPlugin,
         IO,
      };

      enum class IONodeType : i32
      {
         AudioInput,
         AudioOutput,
         MidiInput,
         MidiOutput,
      };

      struct Connection
      {
         enum class Type : i32
         {
            AudioMidi,
            Parameter,
         };

         Type kind;
         node_graph::Socket* fromSocket;
         node_graph::Socket* toSocket;
      };

      NodeGraph (cntx::PluginProcessor* processor);
      ~NodeGraph () override;

      void addAudioPluginNode (const juce::PluginDescription,
                               const juce::Point<f32>,
                               NodeType,
                               Option<IONodeType> io_type = Option<IONodeType> ());

      void removeNode (NodeId id);
      void tryAndMakeConnection (Connection);

      juce::AudioProcessorGraph& getAudioProcessorGraph ();
      const juce::AudioProcessorGraph& getAudioProcessorGraph () const;

      juce::PluginDescription getDescriptionFromId (NodeId id);
      PluginWindow* getOrCreateWindowForId (NodeId);

      juce::ValueTree getValueTree () { return m_value_tree; }
      void print_value_tree () { LOG_INFO ("\n {}", m_value_tree.toXmlString ()); }

      void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override;

      void audioProcessorParameterChanged (juce::AudioProcessor*, i32, f32) override;
      void audioProcessorChanged (juce::AudioProcessor*,
                                  const juce::AudioProcessorListener::ChangeDetails&) override;
      Box<juce::XmlElement> create_xml ();
      // void load_from_xml (Box<juce::XmlElement> xml);
      void load_from_xml (juce::XmlElement* xml);

   private:
      PluginProcessor* m_processor;
      NodeId lastUID;
      NodeId getNextUID () noexcept;
      juce::AudioProcessorGraph m_audio_processor_graph;

      juce::ValueTree m_value_tree;

      // TODO: put this in a better place
      using PtrWindowPair = std::pair<Node::Ptr, Box<PluginWindow>>;
      std::vector<PtrWindowPair> m_plugin_windows;

      // PluginLoaderThread m_plugin_loader_thread;

      void createNodeImpl (Box<juce::AudioPluginInstance>,
                           juce::Point<f32>,
                           NodeType,
                           Option<IONodeType> io_type);

      void createAudioProcessorChannelConnection (NodeAndChannel, NodeAndChannel);

      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NodeGraph)
   };
} // namespace cntx::node_graph