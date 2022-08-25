#include "plugin_format.h"
#include "plugin_instance.h"
#include "../node_graph.h"

namespace cntx::node_graph
{
   // class AudioIOPluginInstance: public NodeGraph::IOProcessor
   // {
   // public:
   //    using IODeviceType = NodeGraph::IOProcessor::IODeviceType;
   //    AudioIOPluginInstance (IODeviceType type)
   //       : NodeGraph::IOProcessor (type)
   //    {}
   // private:
   // };


   juce::PluginDescription InternalPluginInstance::getPluginDescription (const juce::AudioProcessor& proc)
   {
      const auto ins = proc.getTotalNumInputChannels ();
      const auto outs = proc.getTotalNumOutputChannels ();
      const auto identifier = proc.getName ();
      const auto registerAsGenerator = ins == 0;
      const auto acceptsMidi = proc.acceptsMidi ();

      juce::PluginDescription description;
      description.name = identifier;
      description.descriptiveName = identifier;
      description.pluginFormatName = InternalPluginFormat::getIdentifier ().toString ();
      description.category = (registerAsGenerator ? (acceptsMidi ? "Synth" : "Generator")
                                                   : "Effect");
      description.manufacturerName = "Connectrix";
      description.version = ProjectInfo::versionString;
      description.fileOrIdentifier = identifier;
      description.isInstrument = (acceptsMidi && registerAsGenerator);
      description.numInputChannels = ins;
      description.numOutputChannels = outs;
      description.uniqueId = description.deprecatedUid = identifier.hashCode ();

      return description;
   }


} // namespace cntx::node_graph
