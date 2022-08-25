#include "io_node.h"
#include "../data.h"
#include "node_graph/node_graph.h"

namespace cntx::node_graph
{
   static Rc<juce::Path> m_input_icon_path = ([] {
      juce::Path p;
      p.loadPathFromData (data::icons::stream_input_icon_path,
                          sizeof (data::icons::stream_input_icon_path));
      return std::make_shared<juce::Path> (p);
   }) ();

   static Rc<juce::Path> m_output_icon_path = ([] {
      juce::Path p;
      p.loadPathFromData (data::icons::stream_input_icon_path,
                          sizeof (data::icons::stream_input_icon_path));
      return std::make_shared<juce::Path> (p);
   }) ();

   IoNode::IoNode (Node& parent,
                   juce::ValueTree value_tree,
                   juce::PluginDescription description,
                   NodeGraph::IONodeType io_type)
      : Node::Base (parent, value_tree), m_io_type(io_type) //, m_description (description)
   {
      switch (io_type) {
         case NodeGraph::IONodeType::AudioInput: {
            setName ("Audio Input");
         } break;
         case NodeGraph::IONodeType::AudioOutput: {
            setName ("Audio Output");
         } break;
         case NodeGraph::IONodeType::MidiInput: {
            setName ("Midi Input");
         } break;
         case NodeGraph::IONodeType::MidiOutput: {
            setName ("Midi Output");
         } break;
      }
   }

   void IoNode::onPaint (juce::Graphics& g)
   {
      using namespace juce;
      auto iconBounds = getLocalBounds ().reduced (8).toFloat ();
      m_input_icon_path->scaleToFit (iconBounds.getX (),
                                     iconBounds.getY (),
                                     iconBounds.getWidth (),
                                     iconBounds.getHeight (),
                                     true);
      g.setColour (Colour::fromRGB (237, 169, 66));
      g.fillPath (*m_input_icon_path);
   }
} // namespace cntx::node_graph