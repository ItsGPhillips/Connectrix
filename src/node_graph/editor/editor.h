#pragma once

#include "../../components/box_selector.h"
#include "../node_graph.h"
#include "connector.h"
#include "left_sidebar.h"
#include "node/node.h"

namespace cntx::node_graph
{
   // Forward Declarations
   class NodeGraphEditor;
   class EditorViewport;

   namespace states
   {
      struct Normal
      {
      };

      class BoxSelection
      {
      public:
         BoxSelection (juce::Component* parent);
         BoxSelection (BoxSelection&&) = default;
         ~BoxSelection ();
         components::BoxSelector* operator-> () { return m_box_selector.get (); }
         BoxSelection& operator= (BoxSelection&& other);

      private:
         Box<components::BoxSelector> m_box_selector;
         juce::Component* m_parent;
         JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoxSelection)
      };

      struct DraggingConnector
      {
      public:
         DraggingConnector (EditorPanel&, Socket&);
         DraggingConnector (DraggingConnector&&) = default;
         DraggingConnector& operator= (DraggingConnector&& other);
         Socket& getStartSocket () { return m_source_socket; }
         Connector& getConnector () { return m_connector; }

      private:
         Socket& m_source_socket;
         // Connector must be initalised after the socket
         Connector m_connector;
         JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DraggingConnector)
      };

      // ============================================================================
      // DraggingViewport ===========================================================

      class DraggingViewport
      {
      public:
         DraggingViewport (EditorPanel&, juce::Point<f32>);
         DraggingViewport (DraggingViewport&&) = default;
         juce::CachedValue<juce::Point<f32>>* operator-> () { return m_cachedValue.get (); }
         DraggingViewport& operator= (DraggingViewport&& other);

         void updateDragPosition (const juce::MouseEvent&);
         juce::Point<f32> mouseDownPos;
         juce::Point<f32> initalWindowPos;

      private:
         Box<CachedPointValue> m_cachedValue;
         JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DraggingViewport)
      };
      // ============================================================================

      using State = std::variant<Normal, BoxSelection, DraggingConnector, DraggingViewport>;

   } // namespace states

   // EditorPanelContent ============================================================
   class EditorPanel: public juce::Component,
                      public juce::DragAndDropTarget,
                      protected juce::ValueTree::Listener
   {
   public:
      using State = states::State;
      EditorPanel (node_graph::NodeGraph&, EditorViewport&);

      void paint (juce::Graphics&) override;
      void resized () override;

      void mouseDown (const juce::MouseEvent& e) override;
      void mouseDrag (const juce::MouseEvent& e) override;
      void mouseUp (const juce::MouseEvent& e) override;
      void mouseEnter (const juce::MouseEvent&) override;
      void mouseMove (const juce::MouseEvent&) override;
      void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
      bool keyPressed (const juce::KeyPress&) override;

      juce::ValueTree& getValueTree () { return m_value_tree; }

      bool isInterestedInDragSource (const juce::DragAndDropTarget::SourceDetails&) override;
      void itemDropped (const juce::DragAndDropTarget::SourceDetails&) override;

      const State& getState () const { return m_state; }

      Connector::BoxView getConnectors () noexcept;
      void deselect_all ();
      void addSelectedItem (juce::ValueTree);
      void create_node (juce::ValueTree);
      void create_connection (juce::ValueTree);

   protected:
      void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;
      void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override;
      void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override;

   private:
      State m_state;

      EditorViewport& m_parent;
      node_graph::NodeGraph& m_node_graph;

      juce::ValueTree m_value_tree;
      juce::CachedValue<std::pair<f32, juce::Point<f32>>> m_zoom;

      juce::UndoManager m_undoManager;
      HashMap<u32, Box<Node>> m_nodes;
      Vec<Box<Connector>> m_connectors;

      Vec<juce::ValueTree> m_selected_items;

      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EditorPanel)
   };

   // ===============================================================================
   // EditorPanel ===================================================================

   class ScalableEditorContent: public juce::Component
   {
   public:
      ScalableEditorContent (node_graph::NodeGraph&, EditorViewport&);
      void resized () final override;

   private:
      juce::ValueTree m_value_tree;
      CachedPointValue m_position;
      EditorPanel m_editor_panel_content;
   };

   class EditorViewport: public juce::Viewport,
                         protected juce::ValueTree::Listener
   {
   public:
      EditorViewport (node_graph::NodeGraph&);
      void resized () override;

   protected:
      void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;

   private:
      juce::ValueTree m_value_tree;
      CachedPointValue m_position;
      juce::CachedValue<std::pair<f32, juce::Point<f32>>> m_zoom;
      EditorPanel m_content;
      // EditorPanelContent m_content;
      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EditorViewport)
   };

   // ===============================================================================
   // NodeGraphEditor ===============================================================

   class NodeGraphEditor: public juce::Component,
                          public juce::DragAndDropContainer,
                          protected juce::ChangeListener
   {
   public:
      NodeGraphEditor (node_graph::NodeGraph&);
      ~NodeGraphEditor ();

      void paint (juce::Graphics&) override;
      void resized () override;
      void changeListenerCallback (juce::ChangeBroadcaster*) override;

   private:
      EditorViewport m_editor_panel;
      TestSideBar m_sidebar;

      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NodeGraphEditor)
   };
   // ===============================================================================
} // namespace cntx::node_graph