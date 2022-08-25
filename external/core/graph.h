#pragma once

#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/property_map/property_map.hpp>
#include <utility>

namespace boost
{
   enum vertex_properties_t
   {
      vertex_properties
   };
   enum edge_properties_t
   {
      edge_properties
   };

   BOOST_INSTALL_PROPERTY (vertex, properties);
   BOOST_INSTALL_PROPERTY (edge, properties);
} // namespace boost

namespace core::graph
{
   struct NullProps
   {
   };

   template <typename VProps, typename EProps = NullProps>
   class Graph
   {
   public:
      /* an adjacency_list like we need it */
      using GraphContainer =
         ::boost::adjacency_list<::boost::setS,           // disallow parallel edges
                                 ::boost::vecS,           // vertex container
                                 ::boost::bidirectionalS, // directed graph
                                 ::boost::property<::boost::vertex_properties_t, VProps>,
                                 ::boost::property<::boost::edge_properties_t, EProps>>;

      /* a bunch of graph-specific typedefs */
      using Vertex = typename ::boost::graph_traits<GraphContainer>::vertex_descriptor;
      using Edge = typename ::boost::graph_traits<GraphContainer>::edge_descriptor;
      using EdgePair = std::pair<Edge, Edge>;

      using VertexIter = typename ::boost::graph_traits<GraphContainer>::vertex_iterator;
      using EdgeIter = typename ::boost::graph_traits<GraphContainer>::edge_iterator;
      using AdjacencyIter = typename ::boost::graph_traits<GraphContainer>::adjacency_iterator;
      using OutEdgeIter = typename ::boost::graph_traits<GraphContainer>::out_edge_iterator;

      using Degree_t = typename ::boost::graph_traits<GraphContainer>::degree_size_type;

      using AdjacenctVertexRange = std::pair<AdjacencyIter, AdjacencyIter>;
      using out_edge_range_t = std::pair<OutEdgeIter, OutEdgeIter>;
      using vertex_range_t = std::pair<VertexIter, VertexIter>;
      using edge_range_t = std::pair<EdgeIter, EdgeIter>;

      /* constructors etc. */
      Graph () {}

      Graph (const Graph& g) : graph (g.graph) {}
      Graph (Graph&& g) : graph (std::move (g.graph)) {}

      virtual ~Graph () {}

      Graph& operator= (const Graph& rhs)
      {
         graph = rhs.graph;
         return *this;
      }

      Graph& operator= (Graph&& rhs)
      {
         graph = std::move (rhs.graph);
         return *this;
      }

      /* structure modification methods */
      void Clear () { graph.clear (); }

      Vertex AddVertex (const VProps& prop)
      {
         Vertex v = ::boost::add_vertex (graph);
         properties (v) = prop;
         return v;
      }

      void RemoveVertex (const Vertex& v)
      {
         ::boost::clear_vertex (v, graph);
         ::boost::remove_vertex (v, graph);
      }

      Edge AddEdge (const Vertex& v1, const Vertex& v2, const EProps& props)
      {
         /* TODO: maybe one wants to check if this edge could be inserted */
         Edge edge = ::boost::add_edge (v1, v2, graph).first;
         properties (edge) = props;
         return edge;
      }

      /* property access */
      VProps& properties (const Vertex& v)
      {
         typename ::boost::property_map<GraphContainer, ::boost::vertex_properties_t>::type param =
            ::boost::get (::boost::vertex_properties, graph);
         return param[v];
      }

      const VProps& properties (const Vertex& v) const
      {
         typename boost::property_map<GraphContainer, ::boost::vertex_properties_t>::const_type
            param = ::boost::get (::boost::vertex_properties, graph);
         return param[v];
      }

      EProps& properties (const Edge& v)
      {
         typename boost::property_map<GraphContainer, ::boost::edge_properties_t>::type param =
            ::boost::get (::boost::edge_properties, graph);
         return param[v];
      }

      const EProps& properties (const Edge& v) const
      {
         typename ::boost::property_map<GraphContainer, ::boost::edge_properties_t>::const_type
            param = ::boost::get (::boost::edge_properties, graph);
         return param[v];
      }

      /* selectors and properties */
      const GraphContainer& getGraph () const { return graph; }

      vertex_range_t getVertices () const { return ::boost::vertices (graph); }

      AdjacenctVertexRange getAdjacentVertices (const Vertex& v) const
      {
         return ::boost::adjacent_vertices (v, graph);
      }

      int getVertexCount () const { return ::boost::num_vertices (graph); }

      int getVertexDegree (const Vertex& v) const { return ::boost::out_degree (v, graph); }

      /* operators */

   protected:
      GraphContainer graph;
   };
} // namespace core::graph