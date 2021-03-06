/* ECMA-262 script, dialect: QtScript, GraphEditorApi extensions
 *************************************************************
 * File: subGraphChild.js
 * Author: Aleksandr Palamar
 * Date: 07.06.2010 (dd.mm.yyyy)
 * Purpose: Finding child sub-graph
 *************************************************************
*/

/*jslint nomen: true, plusplus: true, continue: true, sloppy: true*/
/*jslint indent: 4, maxerr: 50*/
/*global GraphEditorApi*/
/*global CONNECTION_FROM*/
/*global CONNECTION_TO*/
/*global GraphVertex*/
/*global GraphEdge*/

var verticesToSelect, edgesToSelect, maximumSubCalls;

verticesToSelect = [];
edgesToSelect = [];
maximumSubCalls = 16;

function buildChildSubGraph(index, subCallNumber) {
	if (typeof subCallNumber === "undefined") {
		subCallNumber = 0;
		verticesToSelect.length = 0;
		edgesToSelect.length = 0;
	}

	if (subCallNumber > maximumSubCalls) {
		return;
	}

	var i, edges;
	edges = [];

	GraphEditorApi.getEdgesOfVertex(index, CONNECTION_FROM, edges);
	verticesToSelect[verticesToSelect.length] = index;

	for (i = 0; i < edges.length; i++) {
		edgesToSelect[edgesToSelect.length] = edges[i];
		buildChildSubGraph(edges[i].to, ++subCallNumber);
	}
}

/* function
 *************************************************************
 * onMenuClicked():
 * activated when script item is clicked in Graph menu
 *************************************************************
*/

function onMenuClicked() {
	var selectedIndexes;
	selectedIndexes = [];
	GraphEditorApi.getSelectedIndexes(selectedIndexes);

	buildChildSubGraph(selectedIndexes[0]);
	GraphEditorApi.markGraphsPart(verticesToSelect, edgesToSelect);
}

/* function
 *************************************************************
 * getDescription():
 * string to represent menu name for script
 *************************************************************
*/
function getDescription() {
	return "Find child sub-graph";
}

/* function
 *************************************************************
 * getTypeOfScript():
 * integer value to decide when menu is available
 * 0 - at any time
 * 1 - when only one vertex is selected
 * 2 - when only two vertices are selected
 *************************************************************
*/
function getTypeOfScript() {
	return 1;
}
