#pragma once

#include "../../node_graph.h"

namespace cntx::node_graph
{
   // Forward declarations
   class EditorPanel;
   class Connector;
   class Node;

   // Socket ========================================================================
   class Socket: public juce::Component,
                 public juce::SettableTooltipClient
   {
   public:
      using NodeAndChannel = juce::AudioProcessorGraph::NodeAndChannel;

      enum class Direction : u8
      {
         Input = 0,
         Output = 1,
      };

      struct AudioChannel
      {
         AudioChannel (NodeAndChannel nac) : node_and_channel (nac) {}
         NodeAndChannel node_and_channel;
      };

      struct MidiChannel
      {
         MidiChannel (NodeAndChannel nac) : node_and_channel (nac) {}
         NodeAndChannel node_and_channel;
      };

      struct ParamaterChannel
      {
      };

      using Type = std::variant<std::monostate, AudioChannel, MidiChannel, ParamaterChannel>;
      using NodeGraph = node_graph::NodeGraph;

      Socket (Node& parent, Direction direction);
      Socket (Node&, Type kind, Direction);
      Socket (const Socket&);
      Socket (Socket&&) = default;
      ~Socket () override;

      Socket& operator= (Socket&) = default;
      Socket& operator= (Socket&&) = default;

      void paint (juce::Graphics&) override;
      void resized () override;

      void mouseEnter (const juce::MouseEvent&) override;

      inline Node& getParentNode () { return m_parent_node; }
      /**
         Returns the Direction of the Socket.
         Either Input or Output
      */
      [[nodiscard]] inline const Direction getDirection () const { return m_direction; }
      [[nodiscard]] inline const bool isInput () const { return m_direction == Direction::Input; }
      [[nodiscard]] inline const bool isOutput () const { return !isInput (); }

      // TODO move to cpp file
      [[nodiscard]] inline const node_graph::NodeGraph::NodeAndChannel getNodeAndChannel () const
      {
         auto visitor = overloaded {
            [] (const Socket::AudioChannel& c) { return c.node_and_channel; },
            [] (const Socket::MidiChannel& c) { return c.node_and_channel; },
            [] (const auto&) { return node_graph::NodeGraph::NodeAndChannel ({}); },
         };
         return std::visit (visitor, type);
      }

      Type type;

   private:
      Node& m_parent_node;
      const Direction m_direction;
      static juce::Path SOCKET_PATH;
   };

   // Socket ========================================================================
   // Node ==========================================================================

   class Node: public juce::Component,
               protected juce::ValueTree::Listener
   {
   private:
      class Component;
      class Shell;

   public:
      class Base: public juce::Component
      {
      public:
         Base (Node&, juce::ValueTree);
         virtual void onPaint (juce::Graphics&);
         juce::Path& getPaintableRegion ();
         Node& getParent () { return m_parent; }

         /* internal */
         void paint (juce::Graphics&) final;

      private:
         Node& m_parent;
         JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Base)
      };

      Node (EditorPanel&, node_graph::NodeGraph&, juce::ValueTree, u32);

      void resized () final override;
      EditorPanel& getParentPanel () noexcept;
      juce::AudioProcessor* getProcessor () noexcept;
      node_graph::NodeGraph& getNodeGraph () { return m_node_graph; }
      node_graph::NodeGraph::NodeId getNodeId ();
      void setNodeBase (Box<Node::Base>);
      juce::Path& getBodyPath () { return m_node_shell.getBodyPath (); }

      [[nodiscard]] juce::ValueTree getValueTree () const { return m_value_tree; }

      Vec<Socket>& getSockets () { return m_sockets; }
      Socket* getSocketWithChannelIndex (i32 channel_id, Socket::Direction);

      Shell& getShell () { return m_node_shell; }

      void setSelected(bool is_selected);

   protected:
      void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;
      void updateSockets ();

      node_graph::NodeGraph& m_node_graph;
      EditorPanel& m_parentPanel;
      juce::ValueTree m_value_tree;

   private:
      class Shell final: public juce::Component
      {
      public:
         Shell (Shell&&);
         void paint (juce::Graphics&) final override;
         void resized () final override;
         void mouseDown (const juce::MouseEvent&) final override;
         void mouseDrag (const juce::MouseEvent&) final override;
         void mouseUp (const juce::MouseEvent&) final override;

         juce::Path& getBodyPath () { return m_body_path; }
         void setNodeBase (Box<Node::Base>);

      private:
         Shell (Node&, juce::ValueTree, Box<Base>);
         Node& m_parent;
         Box<Base> m_content_component;
         juce::Path m_titlebar_path;
         juce::Path m_body_path;

         bool m_allowed_to_drag = false;
         juce::Point<f32> m_inital_window_pos;
         CachedPointValue m_position;

         friend class Node;
         JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Shell)
      };

      u32 m_id;
      Node::Shell m_node_shell;
      Vec<Socket> m_sockets;
      juce::DropShadowEffect m_shadowEffect;

      void setupDropShadow ();

      friend class EditorPanel;
      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Node)
   };

   // Node ==========================================================================
} // namespace cntx::node_graph
