#include "connector.h"
#include "editor.h"
#include "node_graph/node_graph.h"

namespace cntx::node_graph
{
   const f32 CONNECTOR_STROKE_THICKNESS = 1.6f;

   Connector::Connector (EditorPanel& parent,
                         Socket* source_socket,
                         Socket* destination_socket,
                         juce::ValueTree value_tree)
      : m_parent (parent), m_value_tree (value_tree), m_from_socket (source_socket),
        m_to_socket (destination_socket)
   {
      m_value_tree.addListener (this);

      jassert (source_socket != nullptr);
      m_start_pos = parent
                       .getLocalPoint (source_socket, source_socket->getLocalBounds ().getCentre ())
                       .toFloat ();
      m_from_socket->getParentNode ().addComponentListener (this);

      if (m_to_socket != nullptr) {
         m_end_pos = parent
                        .getLocalPoint (destination_socket,
                                        destination_socket->getLocalBounds ().getCentre ())
                        .toFloat ();
         m_to_socket->getParentNode ().addComponentListener (this);
      } else {
         m_end_pos = m_start_pos;
      }

      setOpaque (false);
      // setBufferedToImage (true);
      m_parent.addAndMakeVisible (this, 0);
      recalculateBounds ();

      for (auto& connector : m_parent.getConnectors ()) {
         jassert (connector != nullptr);
         connector->toBehind (this);
      }
   }

   Connector::Connector (Connector&& other)
      : m_parent (other.m_parent), m_value_tree (other.m_value_tree),
        m_from_socket (other.m_from_socket), m_to_socket (other.m_to_socket),
        m_start_pos (other.m_start_pos), m_end_pos (other.m_end_pos)
   {
      m_value_tree.addListener (this);
      setOpaque (false);
      // setBufferedToImage (true);
      m_parent.addAndMakeVisible (this, 0);
      recalculateBounds ();

      for (auto& connector : m_parent.getConnectors ()) {
         jassert (connector != nullptr);
         connector->toBehind (this);
      }
   }

   Connector::~Connector ()
   {
      if (m_from_socket) {
         m_from_socket->getParentNode ().removeComponentListener (this);
      }
      if (m_to_socket) {
         m_to_socket->getParentNode ().removeComponentListener (this);
      }
      m_value_tree.removeListener (this);
   }

   Connector& Connector::operator= (Connector&&) { return *this; }

   void Connector::paint (juce::Graphics& g)
   {
      using namespace juce;
      Path scaled_path = m_outline_path;
      auto local_bounds = getLocalBounds ().reduced (CONNECTOR_STROKE_THICKNESS * 2.0f,
                                                     CONNECTOR_STROKE_THICKNESS * 2.0f);

      scaled_path.scaleToFit (local_bounds.getTopLeft ().x,
                              local_bounds.getTopLeft ().y,
                              local_bounds.getWidth (),
                              local_bounds.getHeight (),
                              true);
      if (m_value_tree.isValid ()) {
         jassert (m_value_tree.hasProperty (node_graph::idents::selected));
         auto selected = static_cast<bool> (m_value_tree.getProperty (node_graph::idents::selected));
         g.setColour (selected ? Colour::fromRGB (250, 180, 30) : Colours::cyan);
      } else {
         g.setColour (Colours::cyan);
      }
      g.fillPath (scaled_path);

      g.setColour (Colours::black);
      g.strokePath (scaled_path, PathStrokeType (CONNECTOR_STROKE_THICKNESS / 2));
   }

   void Connector::resized () {}

   void Connector::mouseDown (const juce::MouseEvent&) { 
      m_parent.deselect_all ();
      setSelected(true);
   }

   bool Connector::hitTest (i32 x, i32 y)
   {
      return m_outline_path.contains (
         m_parent.getLocalPoint (this, juce::Point<i32> { x, y }).toFloat ());
   }

   void Connector::setDestinationPosition (juce::Point<f32> end_pos)
   {
      if (m_end_pos == end_pos) {
         return;
      }
      m_end_pos = end_pos;
      recalculateBounds ();
   }

   void Connector::setSourcePosition (juce::Point<f32> start_pos)
   {
      if (m_start_pos == start_pos) {
         return;
      }
      m_start_pos = start_pos;
      recalculateBounds ();
   }

   void Connector::setSourceSocket (Socket* socket)
   {
      if (m_from_socket != nullptr) {
         m_from_socket->removeComponentListener (this);
      }
      m_from_socket = socket;
      if (m_from_socket == nullptr) {
         return;
      }
      m_from_socket->getParentNode ().addComponentListener (this);
      auto new_pos = m_parent.getLocalPoint (m_from_socket,
                                             m_from_socket->getLocalBounds ().getCentre ());
      setSourcePosition (new_pos.toFloat ());
   }

   void Connector::setDestinationSocket (Socket* socket, bool update_connector_bounds)
   {
      if (m_to_socket != nullptr) {
         m_to_socket->removeComponentListener (this);
      }
      m_to_socket = socket;
      if (m_to_socket == nullptr) {
         return;
      }
      m_to_socket->getParentNode ().addComponentListener (this);
      if (update_connector_bounds) {
         auto new_pos = m_parent.getLocalPoint (m_to_socket,
                                                m_to_socket->getLocalBounds ().getCentre ());
         setDestinationPosition (new_pos.toFloat ());
      }
   }

   void Connector::componentMovedOrResized (juce::Component& component,
                                            bool was_moved,
                                            bool /*wasResized*/)
   {
      jassert (m_to_socket != nullptr && m_from_socket != nullptr);
      if (!was_moved) {
         return;
      }
      if (&component == &m_from_socket->getParentNode ()) {
         auto new_pos = m_parent.getLocalPoint (m_from_socket,
                                                m_from_socket->getLocalBounds ().getCentre ());
         setSourcePosition (new_pos.toFloat ());
      } else if (&component == &m_to_socket->getParentNode ()) {
         auto new_pos = m_parent.getLocalPoint (m_to_socket,
                                                m_to_socket->getLocalBounds ().getCentre ());
         setDestinationPosition (new_pos.toFloat ());
      }
   }

   void Connector::componentBroughtToFront (juce::Component& component)
   {
      // if (&component == &m_from_socket->getParentNode ()) {
      //    toBehind (&component);
      //    return;
      // }
      // if (&component == &m_to_socket->getParentNode ()) {
      //    toFront (false);
      //    return;
      // }
   }

   void Connector::setSelected (bool is_selected)
   {
      if (m_value_tree.isValid ()) {
         m_value_tree.setProperty (node_graph::idents::selected, is_selected, nullptr);
      }
      m_parent.addSelectedItem(m_value_tree);
   }

   void Connector::valueTreePropertyChanged (juce::ValueTree& valueTree,
                                             const juce::Identifier& property)
   {
      if (valueTree == m_value_tree) {
         if (property == node_graph::idents::selected) {
            repaint ();
         }
      }
   }

   bool Connector::overlaps (juce::Rectangle<f32> rect)
   {
      juce::PathFlatteningIterator iter (m_outline_path);
      while (iter.next ()) {
         if (rect.intersects (juce::Line<float> (iter.x1, iter.y1, iter.x2, iter.y2))) {
            return true;
         }
      }
      return false;
   }

   void Connector::recalculateBounds ()
   {
#if 0
      auto d = std::min (std::max (std::abs (m_end_pos.x - m_start_pos.x), 50.0f), 100.0f);
      d *= std::min (std::abs (m_end_pos.y - m_start_pos.y) * 0.01f, 1.0f);

      m_main_path.clear ();
      m_main_path.startNewSubPath (m_start_pos);
      m_main_path.cubicTo (
         m_start_pos.translated (d, 0.0f), m_end_pos.translated (-d, 0.0f), m_end_pos);

      auto pst = juce::PathStrokeType (CONNECTOR_STROKE_THICKNESS * 2);
      m_outline_path.clear ();
      pst.createStrokedPath (m_outline_path, m_main_path);
      auto bounds = m_outline_path.getBounds ().expanded (CONNECTOR_STROKE_THICKNESS * 2.0f,
                                                          CONNECTOR_STROKE_THICKNESS * 2.0f);
      setBounds (bounds.toNearestInt ());
      repaint ();
#else
      m_main_path.clear ();
      m_main_path.startNewSubPath (m_start_pos);
      m_main_path.lineTo (m_end_pos);
      auto pst = juce::PathStrokeType (CONNECTOR_STROKE_THICKNESS * 2.2);
      m_outline_path.clear ();
      pst.createStrokedPath (m_outline_path, m_main_path);
      auto bounds = m_outline_path.getBounds ().expanded (CONNECTOR_STROKE_THICKNESS * 2.0f,
                                                          CONNECTOR_STROKE_THICKNESS * 2.0f);
      setBounds (bounds.toNearestInt ());
      repaint ();
#endif
   }
} // namespace cntx::node_graph_editor