#pragma once

#include "../../node_graph.h"
#include "node.h"

namespace cntx::node_graph
{
   class ExternalPluginNode: public Node::Base
   {
   public:
      ExternalPluginNode (Node&,
                          juce::ValueTree,
                          juce::PluginDescription);
      void resized () override;
      void showGuiWindow ();

   private:
      Box<juce::Button> m_open_editor_btn;
      juce::PluginDescription m_description;
   };
} // namespace cntx::node_graph
