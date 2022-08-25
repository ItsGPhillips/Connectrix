
#include "node.h"
#include "../editor.h"
#include "node_graph/node_graph.h"

namespace cntx::node_graph
{
   const i32 SHADOW_RADIUS = 10;
   const f32 SOCKET_HIT_TEST_RADUIS = 8.0f;
   const f32 SOCKET_RADUIS = 5.5f;
   const f32 SOCKET_MARGIN = 5.0f;
   const f32 BORDER_RADIUS = 3.0f;
   const f32 STROKE_THICKNESS = 1.5f;
   const f32 TITLE_HEIGHT = 20.0f;

   // Socket ========================================================================

   Socket::Socket (Node& parent, Direction direction)
      : m_parent_node (parent), m_direction (direction)
   {}

   Socket::Socket (Node& parentNode, Socket::Type kind, Direction direction)
      : type (std::move (kind)), m_parent_node (parentNode), m_direction (direction)
   {
      addMouseListener (&m_parent_node.getParentPanel (), false);
   }

   Socket::Socket (const Socket& other)
      : type (other.type), m_parent_node (other.m_parent_node), m_direction (other.m_direction)
   {
      addMouseListener (&m_parent_node.getParentPanel (), false);
   }

   Socket::~Socket () { removeMouseListener (&m_parent_node.getParentPanel ()); }

   void Socket::paint (juce::Graphics& g)
   {
      using namespace juce;
      auto set_colour_visitor = overloaded {
         [&] (const Socket::AudioChannel&) { g.setColour (juce::Colour::fromRGB (64, 176, 207)); },
         [&] (const Socket::MidiChannel&) { g.setColour (juce::Colour::fromRGB (207, 200, 64)); },
         [&] (const auto&) { g.setColour (juce::Colour::fromRGB (112, 64, 207)); },
      };

      std::visit (set_colour_visitor, type);

      // auto socketBounds = getLocalBounds ().reduced (5).toFloat ();
      auto socketBounds = Rectangle<f32> (SOCKET_RADUIS * 2.0f, SOCKET_RADUIS * 2.0f)
                             .withCentre (getLocalBounds ().getCentre ().toFloat ());

      SOCKET_PATH.scaleToFit (socketBounds.getX (),
                              socketBounds.getY (),
                              socketBounds.getWidth (),
                              socketBounds.getHeight (),
                              true);
      g.fillPath (SOCKET_PATH);

      g.setColour (Colour::fromRGB (72, 72, 72));
      g.strokePath (SOCKET_PATH, PathStrokeType (STROKE_THICKNESS));
   }

   void Socket::resized () {}

   void Socket::mouseEnter (const juce::MouseEvent&)
   {
      const auto& graph = m_parent_node.getNodeGraph ().getAudioProcessorGraph ();
      auto set_tooltip_visitor = overloaded {
         [this, &graph] (const Socket::AudioChannel& c) {
            if (const auto* node = graph.getNodeForId (c.node_and_channel.nodeID)) {
               auto name = node->getProcessor ()->getBus (isInput (), 0)->getName ();
               // auto name = node->getProcessor ()->getInputChannelName (
               //    c.node_and_channel.channelIndex);
               setTooltip (name);
            }
         },
         [this, &graph] (const Socket::MidiChannel& c) {
            if (const auto* node = graph.getNodeForId (c.node_and_channel.nodeID)) {
               auto name = node->getProcessor ()->getInputChannelName (
                  c.node_and_channel.channelIndex);
               setTooltip (name);
            }
         },
         [] (const auto&) {},
      };
      std::visit (set_tooltip_visitor, type);
   }
   // Node::Base ====================================================================

   Node::Base::Base (Node& parent, juce::ValueTree) : m_parent (parent) {}

   void Node::Base::paint (juce::Graphics& g)
   {
      // g.reduceClipRegion (getPaintableRegion ());
      onPaint (g);
   }

   void Node::Base::onPaint (juce::Graphics&) {}

   juce::Path& Node::Base::getPaintableRegion ()
   {
      auto local_bounds = getLocalBounds ().toFloat ().reduced (STROKE_THICKNESS / 2);
      auto& path = getParent ().getBodyPath ();
      path.applyTransform (path.getTransformToScaleToFit (local_bounds, false));
      return path;
   }

   juce::Path Socket::SOCKET_PATH = [] {
      juce::Path p;
      p.addEllipse (juce::Rectangle<f32> (SOCKET_RADUIS * 2.0f, SOCKET_RADUIS * 2.0f));
      return p;
   }();

   // ==============================================================================
   // Node::Shell ==================================================================

   Node::Shell::Shell (Node& parent, juce::ValueTree valuetree, Box<Node::Base> component)
      : m_parent (parent), m_content_component (std::move (component))
   {
      m_position.referTo (valuetree, node_graph::idents::position, nullptr);
   }

   Node::Shell::Shell (Node::Shell&& other)
      : m_parent (other.m_parent), m_content_component (std::move (other.m_content_component)),
        m_titlebar_path (std::move (other.m_titlebar_path)),
        m_body_path (std::move (other.m_body_path))
   {
      m_position.referTo (m_parent.m_value_tree, node_graph::idents::position, nullptr);
   }

   void Node::Shell::paint (juce::Graphics& g)
   {
      using namespace juce;

      g.setColour (Colour::fromRGB (57, 57, 57));
      g.fillPath (m_titlebar_path);

      auto titleBarBounds = m_titlebar_path.getBounds ().reduced (5.0f);
      juce::Path textPath;
      juce::GlyphArrangement glyphs;
      glyphs.addFittedText (g.getCurrentFont (),
                            m_content_component->getName (),
                            titleBarBounds.getX (),
                            titleBarBounds.getY (),
                            titleBarBounds.getWidth (),
                            titleBarBounds.getHeight (),
                            juce::Justification::centred,
                            1);
      auto font = Font ("Arial", 12.0f, Font::FontStyleFlags::bold);
      g.setFont (font);
      glyphs.createPath (textPath);
      g.setColour (Colour::fromRGB (30, 30, 30));
      g.strokePath (textPath, PathStrokeType (2.0));
      g.fillPath (textPath);
      g.setColour (Colour::fromRGB (243, 243, 243));
      g.fillPath (textPath);

      g.setColour (Colour::fromRGB (136, 136, 136));
      g.fillPath (m_body_path);

      Path p;
      auto bounds = getLocalBounds ().toFloat ().reduced (STROKE_THICKNESS);
      p.addLineSegment (Line<f32> ({ STROKE_THICKNESS * 2.0f, TITLE_HEIGHT },
                                   { bounds.getWidth (), TITLE_HEIGHT }),
                        STROKE_THICKNESS / 2.0f);
      g.setColour (Colour::fromRGB (50, 50, 50));
      g.strokePath (p, PathStrokeType (STROKE_THICKNESS));

      p.clear ();
      p.addRoundedRectangle (bounds.getX (),
                             bounds.getY (),
                             bounds.getWidth (),
                             bounds.getHeight (),
                             BORDER_RADIUS,
                             BORDER_RADIUS,
                             true,
                             true,
                             true,
                             true);

      jassert (m_parent.m_value_tree.hasProperty (node_graph::idents::selected));
      auto selected = static_cast<bool> (
         m_parent.m_value_tree.getProperty (node_graph::idents::selected));
      g.setColour (selected ? Colour::fromRGB (250, 180, 30) : Colour::fromRGB (72, 72, 72));
      g.strokePath (p, PathStrokeType (STROKE_THICKNESS));
   }

   void Node::Shell::resized ()
   {
      auto bounds = getLocalBounds ().toFloat ().reduced (STROKE_THICKNESS);
      auto top = bounds.withTrimmedBottom (bounds.getHeight () - TITLE_HEIGHT);
      m_titlebar_path.clear ();
      m_titlebar_path.addRoundedRectangle (top.getX (),
                                           top.getY (),
                                           top.getWidth (),
                                           top.getHeight (),
                                           BORDER_RADIUS,
                                           BORDER_RADIUS,
                                           true,
                                           true,
                                           false,
                                           false);

      auto bottom = bounds.withTrimmedTop (TITLE_HEIGHT);
      m_body_path.clear ();
      m_body_path.addRoundedRectangle (bottom.getX (),
                                       bottom.getY (),
                                       bottom.getWidth (),
                                       bottom.getHeight (),
                                       BORDER_RADIUS,
                                       BORDER_RADIUS,
                                       false,
                                       false,
                                       true,
                                       true);
      if (m_content_component) {
         m_content_component->setBounds (m_body_path.getBounds ().toNearestInt ());
      }
   }

   void Node::Shell::mouseDown (const juce::MouseEvent& e)
   {
      m_parent.toFront (false);

      // auto titlebar_bounds = m_titlebar_path.getBounds ();
      // auto reset_titlebar_bounds = [&, bounds = titlebar_bounds] {
      //    m_titlebar_path.scaleToFit (
      //       bounds.getX (), bounds.getY (), bounds.getX (), bounds.getY (), true);
      // };

      // auto scaled_bounds = titlebar_bounds.reduced (STROKE_THICKNESS * 2.0f);
      // m_titlebar_path.scaleToFit (scaled_bounds.getX (),
      //                             scaled_bounds.getY (),
      //                             scaled_bounds.getX (),
      //                             scaled_bounds.getY (),
      //                             true);
      if (m_titlebar_path.contains (e.position, 10.0f)) {
         m_inital_window_pos = *m_position;
         m_allowed_to_drag = true;

         m_parent.getParentPanel ().deselect_all ();
         m_parent.setSelected (true);
      }

      // reset_titlebar_bounds ();
   }

   void Node::Shell::mouseDrag (const juce::MouseEvent& e)
   {
      auto new_pos = m_inital_window_pos + e.getOffsetFromDragStart ().toFloat ();
      m_position.setValue (new_pos, nullptr);
   }

   void Node::Shell::mouseUp (const juce::MouseEvent&) { m_allowed_to_drag = false; }

   void Node::Shell::setNodeBase (Box<Node::Base> base)
   {
      // node base cant be null
      jassert (base);

      m_content_component.reset (base.release ());
      addAndMakeVisible (*m_content_component);
      m_content_component->toFront (false);
      resized ();
   }

   // ===============================================================================
   // Node ==========================================================================

   Node::Node (EditorPanel& parent,
               node_graph::NodeGraph& nodeGraph,
               juce::ValueTree valuetree,
               u32 id)
      : m_node_graph (nodeGraph), m_parentPanel (parent), m_value_tree (valuetree), m_id (id),
        m_node_shell (*this, valuetree, {})
   {
      // if (auto f = nodeGraph.getAudioProcessorGraph ().getNodeForId (getNodeId ())) {
      //    if (auto* processor = f->getProcessor ()) {
      //       for (auto* param : processor->getParameters ()) {
      //          // LOG_DEBUG ("Param = {}", param->getName (50));
      //       }
      //    }
      // }

      m_value_tree.addListener (this);

      setBufferedToImage (true);
      setOpaque (false);
      setInterceptsMouseClicks (false, true);
      addAndMakeVisible (m_node_shell);
      setupDropShadow ();
      updateSockets ();
   }

   void Node::resized ()
   {
      using namespace juce;
      auto bounds = getLocalBounds ().reduced (SHADOW_RADIUS);
      m_node_shell.setBounds (bounds.reduced (static_cast<i32> (SOCKET_HIT_TEST_RADUIS) * 2
                                                 - static_cast<i32> (STROKE_THICKNESS),
                                              static_cast<i32> (STROKE_THICKNESS)));

      bounds.removeFromTop (static_cast<i32> (TITLE_HEIGHT + SOCKET_MARGIN));

      auto input_bounds = bounds.removeFromLeft (static_cast<i32> (SOCKET_HIT_TEST_RADUIS) * 2);
      auto output_bounds = bounds.removeFromRight (static_cast<i32> (SOCKET_HIT_TEST_RADUIS) * 2);

      for (auto& socket : m_sockets) {
         if (socket.isInput ()) {
            socket.setBounds (
               input_bounds.removeFromTop (static_cast<i32> (SOCKET_HIT_TEST_RADUIS * 2)));
            input_bounds.removeFromTop (static_cast<i32> (SOCKET_MARGIN));
         } else {
            socket.setBounds (
               output_bounds.removeFromTop (static_cast<i32> (SOCKET_HIT_TEST_RADUIS * 2)));
            output_bounds.removeFromTop (static_cast<i32> (SOCKET_MARGIN));
         }
      }
   }

   EditorPanel& Node::getParentPanel () noexcept { return m_parentPanel; }

   juce::AudioProcessor* Node::getProcessor () noexcept
   {
      if (auto* node = getNodeGraph ().getAudioProcessorGraph ().getNodeForId (getNodeId ())) {
         return node->getProcessor ();
      }
      return {};
   }

   void Node::valueTreePropertyChanged (juce::ValueTree& valueTree, const juce::Identifier& property)
   {
      if (valueTree == m_value_tree) {
         if (property == node_graph::idents::position) {
            const auto& position_property = valueTree[node_graph::idents::position];
            const auto position = juce::VariantConverter<juce::Point<f32>>::fromVar (
               position_property);
            setCentrePosition (position.toInt ());
            return;
         }
         if (property == node_graph::idents::selected) {
            repaint ();
            return;
         }
      }
   }

   // creates new sockets to match the audioProcessors input and outputs
   // resizes the node to fit the sockets
   void Node::updateSockets ()
   {
      m_sockets.clear ();

      juce::AudioProcessor* proc = getProcessor ();
      if (proc == nullptr) {
         return;
      }

      const auto ins = proc->getTotalNumInputChannels ();
      const bool accepts_midi = proc->acceptsMidi ();
      const auto total_ins = accepts_midi ? ins + 1 : ins;

      const auto outs = proc->getTotalNumOutputChannels ();
      const bool outputs_midi = proc->producesMidi ();
      const auto total_outs = outputs_midi ? outs + 1 : outs;

      // reserve vector space for all the sockets
      m_sockets.reserve (static_cast<usize> (total_ins) + static_cast<usize> (total_outs));

      const auto midi_index = juce::AudioProcessorGraph::midiChannelIndex;
      const auto node_id = getNodeId ();

      // create input sockets
      for (auto i = 0; i < ins; i++) {
         m_sockets.emplace_back (
            Socket (*this, Socket::AudioChannel ({ node_id, i }), Socket::Direction::Input));
      }
      if (accepts_midi) {
         m_sockets.emplace_back (Socket (
            *this, Socket::MidiChannel ({ node_id, midi_index }), Socket::Direction::Input));
      }

      // create output sockets
      for (auto i = 0; i < outs; i++) {
         m_sockets.emplace_back (
            Socket (*this, Socket::AudioChannel ({ node_id, i }), Socket::Direction::Output));
      }
      if (outputs_midi) {
         m_sockets.emplace_back (Socket (
            *this, Socket::MidiChannel ({ node_id, midi_index }), Socket::Direction::Output));
      }

      // make the sockets children of this node
      for (auto& socket : m_sockets) {
         addAndMakeVisible (socket, 0);
      }

      // TODO (George): Make sure this is right
      const auto title_height = TITLE_HEIGHT + SHADOW_RADIUS;
      auto node_height = SOCKET_HIT_TEST_RADUIS * 2 + SOCKET_MARGIN;
      setSize (200, title_height + (node_height * (std::max (total_ins, total_outs) + 1)));
   }

   node_graph::NodeGraph::NodeId Node::getNodeId ()
   {
      auto id = node_graph::NodeGraph::NodeId (
         static_cast<i32> (m_value_tree[node_graph::idents::id]));
      return node_graph::NodeGraph::NodeId (id);
   }

   void Node::setNodeBase (Box<Node::Base> base)
   {
      m_node_shell.setNodeBase (std::move (base));
   }

   Socket* Node::getSocketWithChannelIndex (i32 channel_idx, Socket::Direction direction)
   {
      auto visitor = overloaded {
         [] (const Socket::AudioChannel& channel) { return channel.node_and_channel.channelIndex; },
         [] (const Socket::MidiChannel& channel) { return channel.node_and_channel.channelIndex; },
         [] (const auto&) {
            LOG_ERROR ("Invalid Socket Type");
            return std::numeric_limits<i32>::max ();
         },
      };

      if(m_sockets.empty()) {
         return {};
      }

      for (auto& socket : m_sockets | ranges::views::filter ([&] (Socket& socket) {
                             auto current_channel_idx = std::visit (visitor, socket.type);
                             return current_channel_idx == channel_idx;
                          })) {
         if (socket.getDirection () == direction) {
            return &socket;
         }
      }
      return nullptr;
   }

   void Node::setSelected (bool is_selected)
   {
      m_value_tree.setProperty (node_graph::idents::selected, is_selected, nullptr);
      m_parentPanel.addSelectedItem (m_value_tree);
   }

   void Node::setupDropShadow ()
   {
      using namespace juce;
      m_shadowEffect.setShadowProperties (
         DropShadow (Colours::black.withAlpha (0.5f), SHADOW_RADIUS, { 0, 0 }));
      setComponentEffect (&m_shadowEffect);
   }

   // Node ==========================================================================
} // namespace cntx::node_graph