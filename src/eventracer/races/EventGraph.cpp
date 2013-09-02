/*
   Copyright 2013 Software Reliability Lab, ETH Zurich

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include "EventGraph.h"


EventGraphInterface::~EventGraphInterface() {
}


SimpleDirectedGraph::SimpleDirectedGraph() {
	addNode();  // Node 0 doesn't exist.
}

void SimpleDirectedGraph::addArc(int source, int target) {
	if (source == target) return;
	Node& node = m_nodes[source];
	for (size_t i = 0; i < node.m_successors.size(); ++i) {
		if (node.m_successors[i] == target) return;
	}
	node.m_successors.push_back(target);
	m_nodes[target].m_predecessors.push_back(source);
}

bool SimpleDirectedGraph::addArcIfNeeded(int source, int target) {
	if (areOrdered(source, target)) return false;
	addArc(source, target);
	return true;
}

void SimpleDirectedGraph::deleteArc(int source, int target) {
	deleteArcFromPredecessors(source, target);
	deleteArcFromSuccessors(source, target);
}

bool SimpleDirectedGraph::areOrdered(int source, int target) const {
	SimpleDirectedGraph::BFIterator it(*this, 0x3fffffff, true);
	it.addNode(source);
	int node;
	while (it.readNoAddFollowers(&node)) {
		if (node == target) return true;
		if (node < target) {
			it.addNodeFollowers(node);
		}
	}
	return false;
}

bool SimpleDirectedGraph::areConnected(int source, int target) const {
	if (source == target) return true;
	SimpleDirectedGraph::BFIterator source_it(*this, 0x3fffffff, true);
	source_it.addNode(source);
	SimpleDirectedGraph::BFIterator target_it(*this, 0x3fffffff, true);
	target_it.addNode(target);

	int from_source_node;
	while (source_it.read(&from_source_node)) {
		if (from_source_node == target) return true;
		int from_target_node;
		if (target_it.read(&from_target_node) && from_target_node == source) return true;
	}

	int from_target_node;
	while (target_it.read(&from_target_node)) {
		if (from_target_node == source) return true;
	}

	return false;
}

bool SimpleDirectedGraph::hasArc(int source, int target) const {
	const Node& node = m_nodes[source];
	for (size_t i = 0; i < node.m_successors.size(); ++i) {
		if (node.m_successors[i] == target) return true;
	}
	return false;
}

void SimpleDirectedGraph::deleteNode(int nodeId, bool always_add_shortcut) {
	Node& node = m_nodes[nodeId];
	if (node.m_deleted) return;  // Already deleted node.
	// Delete the links from other nodes to this node.
	for (size_t i = 0; i < node.m_predecessors.size(); ++i) {
		deleteArcFromSuccessors(node.m_predecessors[i], nodeId);
	}
	for (size_t i = 0; i < node.m_successors.size(); ++i) {
		deleteArcFromPredecessors(nodeId, node.m_successors[i]);
	}
	// Add shortcut arcs.
	for (size_t j = 0; j < node.m_predecessors.size(); ++j) {
		for (size_t i = 0; i < node.m_successors.size(); ++i) {
			if (always_add_shortcut) {
				addArc(node.m_predecessors[j], node.m_successors[i]);
			} else {
				addShortcutArcIfNeeded(node.m_predecessors[j], node.m_successors[i]);
			}
		}
	}
	// Clear the node.
	node.m_deleted = true;
	node.m_predecessors.clear();
	node.m_successors.clear();
}

void SimpleDirectedGraph::addShortcutArcIfNeeded(int source, int target) {
	SimpleDirectedGraph::BFIterator it(*this, 2, true);
	it.addNode(source);
	int node;
	while (it.read(&node)) {
		if (node == target) return;
	}
	addArc(source, target);
}

void SimpleDirectedGraph::deleteArcFromSuccessors(int source, int target) {
	Node& node = m_nodes[source];
	for (size_t i = 0; i < node.m_successors.size(); ++i) {
		if (node.m_successors[i] == target) {
			node.m_successors.erase(node.m_successors.begin() + i);
			return;
		}
	}
}

void SimpleDirectedGraph::deleteArcFromPredecessors(int source, int target) {
	Node& node = m_nodes[target];
	for (size_t i = 0; i < node.m_predecessors.size(); ++i) {
		if (node.m_predecessors[i] == source) {
			node.m_predecessors.erase(node.m_predecessors.begin() + i);
			return;
		}
	}
}
