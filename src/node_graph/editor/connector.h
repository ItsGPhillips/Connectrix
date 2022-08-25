#pragma once
#include "../node_graph.h"

namespace cntx::node_graph
{
   // forward decls
   class EditorPanel;

   // Connector =====================================================================
   class Connector: public juce::Component,
                    public juce::ComponentListener,
                    public juce::ValueTree::Listener
   {
   public:
      using View = ranges::ref_view<Vec<Connector>>;
      using BoxView = ranges::ref_view<Vec<Box<Connector>>>;

      Connector (EditorPanel&, Socket*, Socket* = nullptr, juce::ValueTree = {});
      Connector (Connector&&);
      virtual ~Connector () override;
      Connector& operator= (Connector&&);

      void paint (juce::Graphics&) override;
      void resized () override;
      void mouseDown (const juce::MouseEvent&) override;
      bool hitTest (i32 x, i32 y) override;

      void setDestinationPosition (juce::Point<f32>);
      void setSourcePosition (juce::Point<f32>);
      [[nodiscard]] inline juce::Point<f32> getStartPosition () const { return m_start_pos; }
      [[nodiscard]] inline juce::Point<f32> getEndPosition () const { return m_end_pos; }

      void setSourceSocket (Socket* socket);
      void setDestinationSocket (Socket* socket, bool update_connector_bounds = true);
      inline Socket* getSourceSocket () { return m_from_socket; }
      inline Socket* getDestinationSocket () { return m_to_socket; }

      void componentMovedOrResized (juce::Component&, bool, bool) override;
      void componentBroughtToFront (juce::Component&) override;

      void setSelected (bool is_selected);
      juce::ValueTree getValueTree () { return m_value_tree; }

      void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;

      bool overlaps(juce::Rectangle<f32>);

   private:
      void recalculateBounds ();
      EditorPanel& m_parent;
      juce::ValueTree m_value_tree;

      Socket* m_from_socket;
      Socket* m_to_socket;

      juce::Point<f32> m_start_pos, m_end_pos;

      juce::Path m_main_path;
      juce::Path m_outline_path;
      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Connector)
   };
   // ===============================================================================
} // namespace cntx::node_graph_editor