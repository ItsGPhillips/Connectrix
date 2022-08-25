#pragma once

#include "node.h"
#include "node_graph/node_graph.h"

namespace cntx::node_graph
{
   class IoNode: public Node::Base
   {
   public:
      IoNode (Node&, juce::ValueTree, juce::PluginDescription, NodeGraph::IONodeType io_type);
      void onPaint (juce::Graphics&) override;

   private:
      juce::PluginDescription desc;
      NodeGraph::IONodeType m_io_type;
   };
} // namespace cntx::node_graph
