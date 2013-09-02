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

#ifndef EVENTGRAPH_H_
#define EVENTGRAPH_H_

#include <stddef.h>
#include <vector>
#include <set>
#include <utility>

class EventGraphInterface {
public:
	virtual ~EventGraphInterface();
	virtual bool areOrdered(int source, int target) const = 0;
};

class SimpleDirectedGraph : public EventGraphInterface {
public:
	SimpleDirectedGraph();

	void createEmptyGraph(int node_count) {
		m_nodes.assign(node_count, Node());
	}
	void addNodesUpTo(int node_id) {
		m_nodes.resize(node_id + 1, Node());
	}
	int addNode() {
		m_nodes.push_back(Node());
		return m_nodes.size() - 1;
	}
	int numNodes() const {
		return m_nodes.size();
	}
	bool isNodeDeleted(int node_id) const {
		return m_nodes[node_id].m_deleted;
	}
	void addArc(int source, int target);
	bool addArcIfNeeded(int source, int target);
	void deleteArc(int source, int target);
	void deleteNode(int nodeId, bool always_add_shortcut = false);

	virtual bool areOrdered(int source, int target) const;
	bool areConnected(int source, int target) const;
	bool hasArc(int source, int target) const;

	// Breadth first iterator
	class BFIterator {
	public:
		BFIterator(const SimpleDirectedGraph& graph, int max_depth, bool forward)
		    : m_graph(graph), m_depthRemaining(max_depth), m_forward(forward), m_currentId(0) {
		}

		void addNode(int nodeId) {
			if (m_depthRemaining > 0 && m_visited.insert(nodeId).second) {
				m_next.push_back(nodeId);
			}
		}

		void addNodes(const std::vector<int>& nodes) {
			if (m_depthRemaining <= 0) return;
			for (size_t i = 0; i < nodes.size(); ++i) {
				addNode(nodes[i]);
			}
		}

		bool readNoAddFollowers(int* nodeId) {
			if (m_currentId >= m_current.size()) {
				if (!nextLevel()) return false;
			}
			*nodeId = m_current[m_currentId];
			++m_currentId;
			return true;
		}

		const std::vector<int>& getNodeFollowers(int nodeId) const {
			if (m_forward) {
				return m_graph.m_nodes[nodeId].m_successors;
			} else {
				return m_graph.m_nodes[nodeId].m_predecessors;
			}
		}

		void addNodeFollowers(int nodeId) {
			addNodes(getNodeFollowers(nodeId));
		}

		bool read(int* nodeId) {
			if (!readNoAddFollowers(nodeId)) return false;
			addNodeFollowers(*nodeId);
			return true;
		}

		bool isVisited(int nodeId) const {
			return m_visited.find(nodeId) != m_visited.end();
		}

	private:
		bool nextLevel() {
			if (m_next.empty()) return false;
			m_current.swap(m_next);
			m_next.clear();
			m_currentId = 0;
			--m_depthRemaining;
			return true;
		}

		const SimpleDirectedGraph& m_graph;
		int m_depthRemaining;
		bool m_forward;
		size_t m_currentId;
		std::vector<int> m_current;
		std::vector<int> m_next;
		std::set<int> m_visited;
	};
	friend class BFIterator;

	const std::vector<int>& nodePredecessors(int nodeId) const {
		return m_nodes[nodeId].m_predecessors;
	}

	const std::vector<int>& nodeSuccessors(int nodeId) const {
		return m_nodes[nodeId].m_successors;
	}

private:
	void deleteArcFromSuccessors(int source, int target);
	void deleteArcFromPredecessors(int source, int target);
	void addShortcutArcIfNeeded(int source, int target);

	struct Node {
		Node() : m_deleted(false) {
		}

		bool m_deleted;
		std::vector<int> m_predecessors;
		std::vector<int> m_successors;
	};

	std::vector<Node> m_nodes;
};

#endif /* EVENTGRAPH_H_ */
