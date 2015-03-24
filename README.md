Visual Graph Editor 2
---------------------
Lightweight visual editor for graphs, has support for many languages.
Designed for quick and easy creation, editing and saving of graphs and
analysis of problems connected with graphs. Provides the ability to edit
a graph in any of the three main planes (top, front, side) and view it
in three-dimensional mode. For the analysis presented methods for
extending the functionality of the program through the JavaScript-like
language and RPC (data transfer via XML over HTTP protocol). Created
with Qt Creator 2.0.1 and Qt 4.7.1.

![VGE2 Screenshot](http://imageshack.com/a/img538/254/uatTTf.png)

# Introduction #

Visual Graph Editor 2 has support for scripting with QtScript (Based on the ECMAScript), to make your graph analyzing tasks a bit easier. With help of scripts you can:
  * Dynamically modify graph with scripts and see result immediately;
  * Try to make some tasks from graph theory for easier getting the point of complex algorithms that are connected with graph theory.

# Details #

Scripts are currently stored in the "_scripts_" directory within program directory and have _.js_ extension. To make an extension to the editor, you can add new script simply by creating new file to the "_scripts_" directory. All scripts are initialized at program start-up.

Each script must have at least those 3 special functions:
  * `onMenuClicked()` - activated when script item is clicked in Graph menu;

  * `getDescription()` - returns string to represent menu name for script;

  * `getTypeOfScript()` - returns integer value to decide when menu is available:
    * `0` - at any time;
    * `1` - when only one vertex is selected;
    * `2` - when only two vertices are selected.

# Graph Editor ECMAScript API #
ECMAScript API has an additional objects to easier manipulation of the graph:
  * `GraphVertex`;
  * `GraphEdge`.

The attributes of the `GraphVertex` object are following:
  * `x` - _x_ position of vertex in the 3D space;
  * `y` - _y_ position of vertex in the 3D space;
  * `z` - _z_ position of vertex in the 3D space.

The attributes of the `GraphEdge` object are following:
  * `from` - index of vertex where the edge starts;
  * `to` - index of vertex where the edge ends.

To work with graph that is stored in the editor you can use following methods from the special static `GraphEditorApi` object:
  * `clearGraph()` - delete all vertices and edges from the graph;

  * `addVertices(vertices)` - add vertices to the graph:
    * `vertices` - array of `GraphVertex` objects from the API that should be added to the graph;

  * `addEdges(edges)` - add edges to the graph:
    * `edges` - array of `GraphEdge` objects from the API that should be added to the graph;

  * `getGraph(vertices, edges)` - get entire graph from editor:
    * `vertices` - array that will be used as storage for `GraphVertex` objects from graph;
    * `edges` - array that will be used as storage for `GraphEdge` objects from graph;

  * `setGraph(vertices, edges)` - replace the current graph in editor:
    * `vertices` - array of `GraphVertex` objects that should be added to the graph;
    * `edges` - array of `GraphEdge` objects that should be added to the graph;

  * `getEdgesOfVertex(vertexIndex, connectionWay, edges)` - get all edges of vertex with selected index:
    * `vertexIndex` - index of vertex in the graph to get edges of;
    * `connectionWay` - type of edges to look-up:
      * `CONNECTION_BOTH` - bidirectional edges connected with the vertex;
      * `CONNECTION_FROM` - directional edges that connected from the vertex;
      * `CONNECTION_TO` - directional edges that connected to the vertex;
    * `edges` - array that will be used as storage for `GraphEdge` objects from graph;

  * `deleteVertex(vertexIndex)` - delete vertex with selected index:
    * `vertexIndex` - index of vertex in the graph to delete;

  * `removeEdge(edge)` - delete edge with selected attributes:
    * `edge` - `GraphEdge` object to be deleted;

  * `getSelectedIndexes(indexes)` - get array of indexes of the selected vertices:
    * `indexes` - array of indexes of the selected vertices, where first element is the index of first selected vertex, and second is the index of second selected vertex. If vertex/vertices is/are not selected, then value of index/indexes will be equal to `-1`;

  * `getVertexByIndex(vertexIndex)` - get `GraphVertex` object of index:
    * `vertexIndex` - index of vertex in the graph to get object of;

  * `markGraphsPart(vertices, edges)` - mark the sub-graph in editor:
    * `vertices` - array of `GraphVertex` objects that should be marked in the graph;
    * `edges` - array of `GraphEdge` objects that should be marked in the graph.

# Examples #
The following examples could be found in the source code:
  * Finding child sub-graph - https://github.com/void-995/vge2/blob/master/src/scripts/subGraphChild.js
  * Finding parent sub-graph - https://github.com/void-995/vge2/blob/master/src/scripts//subGraphParent.js
  * Floyd-Warshall algorithm - https://github.com/void-995/vge2/blob/master/src/scripts//pathfindFloyd.js
