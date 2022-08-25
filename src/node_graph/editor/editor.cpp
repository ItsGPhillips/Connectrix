#include <absl/container/flat_hash_map.h>
#include <mimalloc.h>

#include <memory>

#include "../../settings.h"
#include "editor.h"

#include "logger.h"
#include "node/external_plugin.h"
#include "node/io_node.h"
#include "node_graph/internal_plugins/plugin_format.h"
#include "node_graph/node_graph.h"

namespace cntx::node_graph
{
   // states ========================================================================
   namespace states
   {
      // BoxSelection ===============================================================
      BoxSelection::BoxSelection (juce::Component* parent)
         : m_box_selector (new components::BoxSelector ()), m_parent (parent)
      {
         m_box_selector->setVisible (true);
         m_parent->addChildComponent (*m_box_selector);
         m_box_selector->setAlwaysOnTop (true);
      }
      BoxSelection::~BoxSelection () { m_parent->removeChildComponent (m_box_selector.get ()); }
      BoxSelection& BoxSelection::operator= (BoxSelection&& other)
      {
         m_box_selector = std::move (other.m_box_selector);
         m_parent = other.m_parent;
         return *this;
      }

      // ============================================================================
      // DraggingConnector ==========================================================
      DraggingConnector::DraggingConnector (EditorPanel& parent, Socket& source_socket)
         : m_source_socket (source_socket), m_connector (parent, &m_source_socket, nullptr)
      {
         m_connector.setAlwaysOnTop (true);
      }

      DraggingConnector& DraggingConnector::operator= (DraggingConnector&& other)
      {
         *this = std::move (other);
         return *this;
      }

      // ============================================================================
      // DraggingViewport ===========================================================
      DraggingViewport::DraggingViewport (EditorPanel& parent, juce::Point<f32> startPos)
         : mouseDownPos (startPos),
           m_cachedValue (
              new CachedPointValue (parent.getValueTree (), node_graph::idents::position, nullptr))
      {
         initalWindowPos = m_cachedValue->get ();
      }

      DraggingViewport& DraggingViewport::operator= (DraggingViewport&& other)
      {
         mouseDownPos = other.mouseDownPos;
         m_cachedValue = std::move (other.m_cachedValue);
         return *this;
      }

      void DraggingViewport::updateDragPosition (const juce::MouseEvent& e)
      {
         auto newPoint = initalWindowPos - e.getOffsetFromDragStart ().toFloat ();
         m_cachedValue->setValue (newPoint, nullptr);
      }
      // ============================================================================
   } // namespace states

   // ===============================================================================
   // EditorPanelContent ============================================================

   const juce::Rectangle<i32> DEFAULT_VEIWPORT_BOUNDS = {
      {      0,       0},
      {100'000, 100'000}
   };

   EditorPanel::EditorPanel (node_graph::NodeGraph& nodeGraph, EditorViewport& parent)
      : m_state (states::Normal ()), m_parent (parent), m_node_graph (nodeGraph),
        m_value_tree (nodeGraph.getValueTree ())
   {
      m_zoom.referTo (m_value_tree, node_graph::idents::zoom, nullptr);
      m_value_tree.addListener (this);

      setWantsKeyboardFocus (true);
      setMouseClickGrabsKeyboardFocus (true);
      // Verrrry big viewport
      setSize (DEFAULT_VEIWPORT_BOUNDS.getWidth (), DEFAULT_VEIWPORT_BOUNDS.getHeight ());

      if (m_value_tree.isValid ()) {
         LOG_DEBUG ("EditorPanel Constructor -> \n{}\n", m_value_tree.toXmlString ());

         jassert (m_value_tree.hasType (node_graph::idents::GRAPH));
         for (auto child_vt : m_value_tree) {
            if (child_vt.hasType (node_graph::idents::NODE)) {
               create_node (child_vt);
            }
         }
         for (auto child_vt : m_value_tree) {
            if (child_vt.hasType (node_graph::idents::CONNECTION)) {
               create_connection (child_vt);
            }
         }
      }
   }

   void EditorPanel::paint (juce::Graphics& g)
   {
      using namespace juce;
      const i32 STEP_SIZE = 20;
      const f32 LINE_THICKNESS = 0.2f;

      const auto va = m_parent.getViewArea ();

      g.setColour (Colour::fromRGB (19, 19, 19));
      g.fillRect (va);

      bool shouldDrawLines = true;

      if (shouldDrawLines) {
         auto lineColour = Colour::fromRGB (59, 61, 64);
         g.setColour (lineColour);

         i32 horizontalDragOffset = va.getX () % STEP_SIZE;
         for (i32 step = 0; step < va.getWidth () + STEP_SIZE; step += STEP_SIZE) {
            g.fillRect (Rectangle<f32> (
               static_cast<f32> (va.getX () + step - horizontalDragOffset - LINE_THICKNESS / 2.0f),
               static_cast<f32> (va.getY ()),
               LINE_THICKNESS,
               static_cast<f32> (va.getHeight ())));
         }

         i32 verticalDragOffset = va.getY () % STEP_SIZE;
         for (i32 step = 0; step < va.getHeight () + STEP_SIZE; step += STEP_SIZE) {
            g.fillRect (Rectangle<f32> (
               static_cast<f32> (va.getX ()),
               static_cast<f32> (va.getY () + step - verticalDragOffset - LINE_THICKNESS / 2.0f),
               static_cast<f32> (va.getWidth ()),
               LINE_THICKNESS));
         }
      }
   }

   void EditorPanel::resized () {}

   void EditorPanel::mouseDown (const juce::MouseEvent& e)
   {
      auto normal_state_visitor = [&] (states::Normal&) mutable {
         if (e.eventComponent == this) {
            if (e.mods.isPopupMenu ()) {
               auto menu = std::make_unique<juce::PopupMenu> ();
               menu->addItem (juce::PopupMenu::Item ("Add Audio Input Node").setID (1));
               menu->addItem (juce::PopupMenu::Item ("Add Audio Output Node").setID (2));
               menu->addItem (juce::PopupMenu::Item ("Add Midi Input Node").setID (3));
               menu->addItem (juce::PopupMenu::Item ("Add Midi Output Node").setID (4));

               using IOP = node_graph::NodeGraph::IOProcessor;
               using NodeType = node_graph::NodeGraph::NodeType;
               using IONodeType = node_graph::NodeGraph::IONodeType;
               menu->showMenuAsync ({}, [this, event = e] (int result) {
                  switch (result) {
                     case 1: {
                        const auto desc =
                           IOP (IOP::IODeviceType::audioInputNode).getPluginDescription ();
                        m_node_graph.addAudioPluginNode (
                           desc, event.mouseDownPosition, NodeType::IO, IONodeType::AudioInput);
                        break;
                     }
                     case 2: {
                        const auto desc =
                           IOP (IOP::IODeviceType::audioOutputNode).getPluginDescription ();
                        m_node_graph.addAudioPluginNode (
                           desc, event.mouseDownPosition, NodeType::IO, IONodeType::AudioOutput);
                        break;
                     }
                     case 3: {
                        const auto desc =
                           IOP (IOP::IODeviceType::midiInputNode).getPluginDescription ();
                        m_node_graph.addAudioPluginNode (
                           desc, event.mouseDownPosition, NodeType::IO, IONodeType::MidiInput);
                        break;
                     }
                     case 4: {
                        const auto desc =
                           IOP (IOP::IODeviceType::midiOutputNode).getPluginDescription ();
                        m_node_graph.addAudioPluginNode (
                           desc, event.mouseDownPosition, NodeType::IO, IONodeType::MidiOutput);
                        break;
                     }
                     default: LOG_DEBUG ("Error");
                  };
               });
               return;
            }

            if (e.mods.isMiddleButtonDown ()
                || (e.mods.isAltDown () && e.mods.isLeftButtonDown ())) {
               m_state = states::DraggingViewport (*this, e.position);
               return;
            }

            if (e.mods.isLeftButtonDown ()) {
               deselect_all ();
               auto box_selection = states::BoxSelection (this);
               box_selection->beginSelection (e.getPosition ().toFloat ());
               m_state = std::move (box_selection);
               grabKeyboardFocus ();
               return;
            }
            return;
         }
         if (auto* socket = dynamic_cast<Socket*> (e.eventComponent)) {
            if (e.mods.isLeftButtonDown ()) {
               m_state = states::DraggingConnector (*this, *socket);
               return;
            }
            return;
         }
      };
      // ============================================================================
      std::visit (
         overloaded {
            normal_state_visitor,
            [] (auto&) { UNREACHABLE },
         },
         m_state);
   }

   void EditorPanel::mouseDrag (const juce::MouseEvent& e)
   {
      // ============================================================================
      const auto normal_state_visitor = [] (states::Normal&) { UNREACHABLE };
      // ============================================================================
      const auto box_selection_state_visitor = [&] (states::BoxSelection& state) {
         using namespace node_graph;
         deselect_all ();

         state->resizeSelection (e.getPosition ().toFloat ());
         auto bounds = state->getBounds ();

         if (e.mods.isCommandDown ()) {
            // If Cntl | Command is held down switch to select connections.
            for (auto& connector : m_connectors) {
               if (bounds.intersects (connector->getBounds ())
                   && connector->overlaps (bounds.toFloat ())) {
                  addSelectedItem (connector->getValueTree ());
                  connector->setSelected (true);
                  connector->repaint ();
               }
            }
         } else {
            for (const juce::ValueTree& const_vt : m_value_tree) {
               auto vt = const_vt;
               if (vt.getType () == idents::NODE) {
                  auto id = static_cast<i32> (vt.getProperty (idents::id));
                  auto& node = m_nodes.at (static_cast<u32> (id));
                  if (bounds.intersects (
                         getLocalArea (&*node, node->getShell ().getBounds ().toNearestInt ()))) {
                     node->setSelected (true);
                     addSelectedItem (vt);
                  }
               }
            }
         }
      };
      // ============================================================================
      const auto dragging_connector_state_visitor = [&] (states::DraggingConnector& state) {
         using namespace ranges;
         auto& connector = state.getConnector ();
         // event e is relative to the start socket because this is where the drag started
         // the position needs to be conveted to be relative EditorPanelContent.
         auto local_point = getLocalPoint (&state.getStartSocket (), e.getPosition ());

         auto contains_point = [&] (const auto& node) {
            auto node_local_point = node->getLocalPoint (&state.getStartSocket (), e.getPosition ());
            return node->contains (node_local_point);
         };
         // find a node that contains the event position
         for (auto& node : m_nodes | views::values | views::filter (contains_point)) {
            jassert (node != nullptr);
            // find a socket within the node that contains the event position
            for (auto& socket : node->getSockets ()) {
               if (connector.getSourceSocket () == &socket) {
                  continue;
               }
               auto local_bounds = getLocalArea (node.get (), socket.getBounds ());
               if (local_bounds.contains (local_point)) {
                  // setting the desitination position here instead of passing true to
                  // setDestinationSocket and letting it set the postions allows us to reuse the
                  // local_bounds already calculated
                  connector.setDestinationSocket (&socket, false);
                  connector.setDestinationPosition (local_bounds.getCentre ().toFloat ());
                  return;
               }
            }
         }
         // if we got here set the destination socket to nullptr and set the connector destination
         // position to the drag event position
         connector.setDestinationSocket (nullptr, false);
         connector.setDestinationPosition (local_point.toFloat ());
      };
      // ============================================================================
      const auto dragging_viewport_state_visitor = [&] (states::DraggingViewport& state) {
         state.updateDragPosition (e);
      };
      // ============================================================================
      std::visit (
         overloaded {
            normal_state_visitor,
            box_selection_state_visitor,
            dragging_connector_state_visitor,
            dragging_viewport_state_visitor,
         },
         m_state);
   }

   void EditorPanel::mouseUp (const juce::MouseEvent& e)
   {
      juce::ignoreUnused (e);
      // ============================================================================
      const auto normalStateVisitor = [] (states::Normal&) {};
      // ============================================================================
      auto boxSelectionStateVisitor = [this] (states::BoxSelection&) {
         m_state = states::Normal ();
      };
      // ============================================================================
      const auto draggingConnectorStateVisitor = [&] (states::DraggingConnector& state) {
         auto& connector = state.getConnector ();
         if (auto* socket = connector.getDestinationSocket ()) {
            using Connection = node_graph::NodeGraph::Connection;
            m_node_graph.tryAndMakeConnection (Connection {
               .kind = Connection::Type::AudioMidi,
               .fromSocket = connector.getSourceSocket (),
               .toSocket = socket,
            });
         }
         m_state = states::Normal ();
      };
      // ============================================================================
      const auto draggingViewportStateVisitor = [&] (states::DraggingViewport&) {
         m_state = states::Normal ();
      };
      // ============================================================================
      std::visit (
         overloaded {
            normalStateVisitor,
            boxSelectionStateVisitor,
            draggingConnectorStateVisitor,
            draggingViewportStateVisitor,
         },
         m_state);
   }

   void EditorPanel::mouseEnter (const juce::MouseEvent& e) {}
   void EditorPanel::mouseMove (const juce::MouseEvent& e) {}

   void EditorPanel::mouseWheelMove (const juce::MouseEvent& e,
                                     const juce::MouseWheelDetails& details)
   {
      // ============================================================================
      const auto normalStateVisitor = [&] (states::Normal&) {
         auto f = details.deltaY > 0 ? 0.0001 : -0.0001;
         auto pos = m_parent.getLocalPoint (this, e.getPosition ()).toFloat ();
         m_zoom.setValue ({ m_zoom.get ().first + f, pos }, nullptr);
      };
      // ============================================================================
      std::visit (
         overloaded {
            normalStateVisitor,
            [] (auto&) {},
         },
         m_state);
   }

   bool EditorPanel::keyPressed (const juce::KeyPress& kp)
   {
      if (kp.isKeyCode (juce::KeyPress::deleteKey)) {
         for (auto& value_tree : m_selected_items) {
            m_value_tree.removeChild (value_tree, nullptr);
         }
         return true;
      }
      if (kp.isKeyCode (juce::KeyPress::spaceKey)) {
         // m_node_graph.create_xml ();
         LOG_DEBUG ("Latency: {}", m_node_graph.getAudioProcessorGraph ().getLatencySamples ());
         return true;
      }
      return false;
   }

   Connector::BoxView EditorPanel::getConnectors () noexcept
   {
      return m_connectors | ranges::views::all;
   }

   void EditorPanel::deselect_all ()
   {
      for (auto& value_tree : m_selected_items) {
         jassert (value_tree.hasProperty (node_graph::idents::selected));
         value_tree.setProperty (node_graph::idents::selected, false, nullptr);
      }
      m_selected_items.clear ();
   }

   void EditorPanel::addSelectedItem (juce::ValueTree vt) { m_selected_items.emplace_back (vt); }

   void EditorPanel::valueTreePropertyChanged (juce::ValueTree& valueTree,
                                               const juce::Identifier& property)
   {
      if (valueTree.hasType (node_graph::idents::NODE)) {
         if (property == node_graph::idents::position) {
            const auto& position_property = valueTree[node_graph::idents::position];
            const auto position = juce::VariantConverter<juce::Point<f32>>::fromVar (
               position_property);
            const auto id = static_cast<i32> (valueTree[node_graph::idents::id]);

            auto& node = m_nodes[id];
            node->setCentrePosition (position.toInt ());
         }
      }
   }

   void EditorPanel::valueTreeChildAdded (juce::ValueTree& parent_tree, juce::ValueTree& child_tree)
   {
      using namespace node_graph;

      if (parent_tree == m_value_tree) {
         if (child_tree.hasType (idents::NODE)) {
            create_node (child_tree);
         } else if (child_tree.hasType (idents::CONNECTION)) {
            create_connection (child_tree);
         }
      }
   }

   void EditorPanel::valueTreeChildRemoved (juce::ValueTree& parent_tree,
                                            juce::ValueTree& child_tree,
                                            int /*indexFromWhichChildWasRemoved*/)
   {
      using namespace node_graph;
      if (parent_tree == m_value_tree) {
         if (child_tree.hasType (idents::NODE)) {
            i32 idx = child_tree[idents::id];
            jassert (m_nodes.contains (idx));
            removeChildComponent (m_nodes[idx].get ());
            m_nodes.erase (idx);
            return;
         }
         if (child_tree.hasType (idents::CONNECTION)) {
            std::erase_if (m_connectors, [child_tree] (const auto& connector) {
               return connector->getValueTree () == child_tree;
            });
         }
      }
   }

   bool EditorPanel::isInterestedInDragSource (const juce::DragAndDropTarget::SourceDetails&)
   {
      // TODO: some items might not be allowed to drop on this target.. impliment a way to
      // detect if items should be dropped here.
      return true;
   }
   void EditorPanel::itemDropped (const juce::DragAndDropTarget::SourceDetails& srcDetails)
   {
      if (srcDetails.description.isString ()) {
         if (auto description = GlobalStateManager::getPluginManager ()
                                   .getKnownPluginList ()
                                   .getTypeForIdentifierString (srcDetails.description)) {
            auto type = NodeGraph::NodeType::ExternalPlugin;
            if (description->pluginFormatName
                == InternalPluginFormat::getIdentifier ().getCharPointer ()) {
               type = NodeGraph::NodeType::IO;
            }
            m_node_graph.addAudioPluginNode (*description, srcDetails.localPosition.toFloat (), type);
         } else {
            LOG_WARN ("Plugin identifier string not found");
         }
         return;
      }

      if (srcDetails.description.isInt ()) {
         // TODO: temporary, replace with description from drag source details ptr;
         int index = srcDetails.description;
         auto description =
            GlobalStateManager::getPluginManager ().getKnownPluginList ().getTypes ()[index];

         auto type = NodeGraph::NodeType::ExternalPlugin;
         if (description.pluginFormatName
             == InternalPluginFormat::getIdentifier ().getCharPointer ()) {
            type = NodeGraph::NodeType::IO;
         }

         m_node_graph.addAudioPluginNode (description, srcDetails.localPosition.toFloat (), type);
      }
   }

   void EditorPanel::create_node (juce::ValueTree value_tree)
   {
      using namespace node_graph;
      using NodeType = NodeGraph::NodeType;

      deselect_all ();
      jassert (value_tree.hasProperty (idents::selected));
      jassert (static_cast<bool> (value_tree.getProperty (idents::selected)));
      m_selected_items.emplace_back (value_tree);

      const auto id = static_cast<i32> (value_tree[idents::id]);
      // Node with this Id already exists.
      jassert (!m_nodes.contains (id));

      auto node = std::make_unique<Node> (*this, m_node_graph, value_tree, id);

      auto node_id = NodeGraph::NodeId (static_cast<i32> (value_tree[idents::id]));
      auto desc = m_node_graph.getDescriptionFromId (node_id);

      Box<Node::Base> node_base;
      switch (static_cast<i32> (value_tree[idents::type])) {
         case static_cast<i32> (NodeType::ExternalPlugin): {
            node_base = std::make_unique<ExternalPluginNode> (*node, value_tree, desc);
            break;
         }
         case static_cast<i32> (NodeType::IO): {
            auto io_type = static_cast<i32> (value_tree[idents::io_type]);
            node_base = std::make_unique<IoNode> (
               *node, value_tree, desc, static_cast<NodeGraph::IONodeType> (io_type));
            break;
         }
      }

      node->setNodeBase (std::move (node_base));
      addAndMakeVisible (*node);

      // temporary CachedPointValue to make setting the position easier and type safe
      auto pos = CachedPointValue (value_tree, idents::position, nullptr);
      node->setCentrePosition (pos->toInt ());

      m_nodes.emplace (id, std::move (node));
      value_tree.addListener (this);
   }

   void EditorPanel::create_connection (juce::ValueTree value_tree)
   {
      using namespace node_graph;

      std::array<std::pair<i32, i32>, 2> sockets_meta;
      // Connection Trees should only have 2 children.
      // - 1 to socket
      // - 1 from socket
      jassert (value_tree.getNumChildren () == sockets_meta.size ());

      for (auto idx = 0; idx < sockets_meta.size (); idx++) {
         const auto socket_tree = value_tree.getChild (idx);
         // BUG: invalid child type
         jassert (socket_tree.isValid () && socket_tree.getType () == idents::SOCKET);

         const auto node_id = static_cast<i32> (socket_tree[idents::id]);
         const auto channel_id = static_cast<i32> (socket_tree[idents::channel]);
         sockets_meta[idx] = std::make_pair (node_id, channel_id);
      }

      std::array<Socket*, 2> connected_sockets { nullptr };
      {
         auto& [node_id, channel_id] = sockets_meta[0];

         if (m_nodes.contains (static_cast<u32> (node_id))) {
            auto& node = m_nodes[static_cast<u32> (node_id)];
            connected_sockets[0] = node->getSocketWithChannelIndex (channel_id,
                                                                    Socket::Direction::Output);
         };
      }
      {
         auto& [node_id, channel_id] = sockets_meta[1];
         if (m_nodes.contains (static_cast<u32> (node_id))) {
            auto& node = m_nodes[static_cast<u32> (node_id)];
            connected_sockets[1] = node->getSocketWithChannelIndex (channel_id,
                                                                    Socket::Direction::Input);
         }
      }

      auto& [from_socket, to_socket] = connected_sockets;
      if (from_socket != nullptr && to_socket != nullptr) {
         auto connection = std::make_unique<Connector> (*this, from_socket, to_socket, value_tree);
         m_connectors.emplace_back (std::move (connection));
      }
   }
   // ===============================================================================
   // EditorPanel ===================================================================

   EditorViewport::EditorViewport (node_graph::NodeGraph& nodeGraph)
      : m_content (EditorPanel (nodeGraph, *this)), m_value_tree (nodeGraph.getValueTree ())
   {
      m_value_tree.addListener (this);
      m_position.referTo (m_value_tree, node_graph::idents::position, nullptr);

      juce::MessageManager::callAsync ([this] {
         if (m_position.isUsingDefault ()) {
            m_position.setValue ({ static_cast<f32> (DEFAULT_VEIWPORT_BOUNDS.getWidth ()) / 2.0f,
                                   static_cast<f32> (DEFAULT_VEIWPORT_BOUNDS.getHeight ()) / 2.0f },
                                 nullptr);
         }
         resized ();
      });

      setScrollOnDragEnabled (false);
      setScrollBarsShown (false, false, true, true);
      setViewedComponent (&m_content, false);
   }

   void EditorViewport::resized () { this->juce::Viewport::resized (); }
   void EditorViewport::valueTreePropertyChanged (juce::ValueTree& valueTree,
                                                  const juce::Identifier& property)
   {
      if (valueTree.hasType (node_graph::idents::GRAPH)) {
         if (property == m_position.getPropertyID ()) {
            setViewPosition (m_position->toInt ());
            return;
         }
         if (property == m_zoom.getPropertyID ()) {
            // TODO: figure out how to scale the viewport
            /*    auto bounds = m_content.getBounds ();
                auto [zoom, pos] = m_zoom.get ();
                m_content.setTransform (juce::AffineTransform::scale (zoom, zoom, pos.x, pos.y));*/
            return;
         }
      }
   }

   // ===============================================================================
   // ScalableNodeGraphEditor =======================================================

   ScalableEditorContent::ScalableEditorContent (node_graph::NodeGraph& node_graph,
                                                 EditorViewport& panel)
      : m_editor_panel_content (node_graph, panel)
   {
      addAndMakeVisible (m_editor_panel_content);
   }
   void ScalableEditorContent::resized () { m_editor_panel_content.setBounds (getBounds ()); }

   // ===============================================================================
   // NodeGraphEditor ===============================================================

   NodeGraphEditor::NodeGraphEditor (node_graph::NodeGraph& nodeGraph) : m_editor_panel (nodeGraph)
   {
      m_sidebar.getContentPanel ().addChangeListener (this);

      // we dont make the content panel immediately visible.
      addChildComponent (m_sidebar.getContentPanel ());
      addAndMakeVisible (m_sidebar.getButtonBar ());
      addAndMakeVisible (m_editor_panel);
      juce::MessageManager::callAsync ([this] {
         resized ();
         repaint ();
      });
   }

   NodeGraphEditor::~NodeGraphEditor ()
   {
      m_sidebar.getContentPanel ().removeChangeListener (this);
   }

   void NodeGraphEditor::paint (juce::Graphics& g) {}

   void NodeGraphEditor::resized ()
   {
      auto bounds = getLocalBounds ();
      auto sideButtonBarBounds = bounds.removeFromLeft (40);
      m_sidebar.getButtonBar ().setBounds (sideButtonBarBounds);

      if (m_sidebar.getContentPanel ().isVisible ()) {
         auto boundsCpy = bounds;
         m_sidebar.getContentPanel ().setBounds (boundsCpy.removeFromLeft (222));
      }
      m_editor_panel.setBounds (bounds);
   }

   void NodeGraphEditor::changeListenerCallback (juce::ChangeBroadcaster* source)
   {
      if (source == &m_sidebar.getContentPanel ()) {
         if (m_sidebar.getContentPanel ().getActiveContent () != nullptr) {
            m_sidebar.getContentPanel ().setVisible (true);
            m_sidebar.getContentPanel ().toFront (true);
         } else {
            m_sidebar.getContentPanel ().setVisible (false);
         }
         resized ();
      }
   }
} // namespace cntx::node_graph