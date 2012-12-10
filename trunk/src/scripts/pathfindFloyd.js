/* ECMA-262 script, dialect: QtScript, VGE2 extensions
 *************************************************************
 * File: pathfindFloyd.js
 * Author: Aleksandr Palamar
 * Date: 09.06.2010 (dd.mm.yyyy)
 * Purpose: Floyd-Warshall algorithm
 *************************************************************
*/

/*jslint nomen: true, plusplus: true, continue: true, sloppy: true*/
/*jslint newcap: true*/
/*jslint indent: 4, maxerr: 50*/
/*global VGE2*/
/*global CONNECTION_FROM*/
/*global CONNECTION_TO*/
/*global grVertex*/
/*global grEdge*/

/* function
 *************************************************************
 * onMenuClicked():
 * activated when script item is clicked in Graph menu
 *************************************************************
*/
function onMenuClicked() {
	var i, j, k,
		selectedIndexes,
		vertices, edges,
		matrixDistances, matrixIndexes,
		flag, distance,
		verticesToSelect, edgesToSelect;

	selectedIndexes = [];
	VGE2.getSelectedIndexes(selectedIndexes);

	vertices = [];
	edges = [];
	VGE2.getGraph(vertices, edges);

	matrixDistances = [];

	for (i = 0; i < vertices.length; i++) {
		matrixDistances[i] = [];
		for (j = 0; j < vertices.length; j++) {
			matrixDistances[i][j] = -1;
			for (k = 0; k < edges.length; k++) {
				if (edges[k].from === i && edges[k].to === j) {
					matrixDistances[i][j] = 0;
					matrixDistances[i][j] += (vertices[i].x - vertices[j].x) * (vertices[i].x - vertices[j].x);
					matrixDistances[i][j] += (vertices[i].y - vertices[j].y) * (vertices[i].y - vertices[j].y);
					matrixDistances[i][j] += (vertices[i].z - vertices[j].z) * (vertices[i].z - vertices[j].z);
					matrixDistances[i][j] = Math.sqrt(matrixDistances[i][j]);
					break;
				}
			}
		}
	}

	matrixIndexes = [];

	for (i = 0; i < vertices.length; i++) {
		matrixIndexes[i] = [];
		for (j = 0; j < vertices.length; j++) {
			matrixIndexes[i][j] = j;
		}
	}

	flag = true;
	while (flag) {
		flag = false;

		for (i = 0; i < vertices.length; i++) {
			for (j = 0; j < vertices.length; j++) {
				for (k = 0; k < vertices.length; k++) {
					if ((matrixDistances[j][i] === -1) || (matrixDistances[i][k] === -1)) {
						continue;
					}

					distance = matrixDistances[j][i] + matrixDistances[i][k];

					if ((distance < matrixDistances[j][k]) || (matrixDistances[j][k] === -1)) {
						matrixDistances[j][k] = distance;
						matrixIndexes[j][k] = matrixIndexes[j][i];
						flag = true;
					}
				}
			}
		}
	}

	for (i = 0; i < vertices.length; i++) {
		for (j = 0; j < vertices.length; j++) {
			if (matrixDistances[i][j] === -1) {
				matrixIndexes[i][j] = -1;
			}
		}
	}

	k = 0;
	i = selectedIndexes[0];
	j = selectedIndexes[1];

	verticesToSelect = [];
	edgesToSelect = [];

	if (matrixDistances[i][j] !== -1) {
		verticesToSelect[k] = i;
		k++;

		while (i !== j) {
			i = matrixIndexes[i][j];
			verticesToSelect[k] = i;
			edgesToSelect[k - 1] = new grEdge(verticesToSelect[k - 1], verticesToSelect[k]);
			k++;
		}
	}

	VGE2.markGraphsPart(verticesToSelect, edgesToSelect);
}

/* function
 *************************************************************
 * getDescription():
 * string to represent menu name for script
 *************************************************************
*/
function getDescription() {
	return "Floyd-Warshall algorithm";
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
	return 2;
}
