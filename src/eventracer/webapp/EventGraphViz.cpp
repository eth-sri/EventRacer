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

#include "EventGraphViz.h"
#include "stringprintf.h"

// Selects which nodes to display.
class NodeSelection {
public:
	explicit NodeSelection(const SimpleDirectedGraph* graph) : m_graph(graph) {
	}

	void addNode(int node_id, int priority) {
		if (node_id < 0 || node_id >= m_graph->numNodes()) return;
		std::map<int, int>::iterator it =
				m_nodePriority.insert(std::make_pair(node_id, priority)).first;
		m_prioritySet.erase(std::pair<int, int>(-it->second, it->first));
		it->second = priority;
		m_prioritySet.insert(std::pair<int, int>(-priority, node_id));
	}

	bool getNode(int* node_id) {
		int priority;
		if (!getNextNodePrioritySet(node_id, &priority)) return false;
		addNeighborNodes(m_graph->nodePredecessors(*node_id), priority);
		addNeighborNodes(m_graph->nodeSuccessors(*node_id), priority);
		addNode((*node_id) + 1, priority - 10);
		return true;
	}

private:
	void addNeighborNodes(const std::vector<int>& nodes, int base_priority) {
		for (size_t i = 0; i < nodes.size(); ++i) {
			addNode(nodes[i], (i > 4) ? (base_priority - 5) : (base_priority - 2));
		}
	}

	bool getNextNodePrioritySet(int* node_id, int* priority) {
		for (;;) {
			std::set<std::pair<int, int> >::iterator it = m_prioritySet.begin();
			if (it == m_prioritySet.end()) break;
			*priority = -it->first;
			*node_id = it->second;
			m_prioritySet.erase(it);
			if (m_selectedNodes.insert(*node_id).second) {
				return true;
			}
		}
		return false;
	}

	const SimpleDirectedGraph* m_graph;

	// An ordered set by priority and then the node id.
	std::set<std::pair<int, int> > m_prioritySet;

	// For every node, the priority it has.
	std::map<int, int> m_nodePriority;
	std::set<int> m_selectedNodes;
};


EventGraphDisplay::EventGraphDisplay(
			const std::string& linkcmd,
			const std::string& file_name,
			const URLParams& params,
			const ActionLog* action_log,
			const EventGraphInfo* graph_info,
			const SimpleDirectedGraph* original_graph,
			const SimpleDirectedGraph* timer_graph)
    : m_linkCmd(linkcmd),
      m_fileName(file_name),
      m_params(params),
      m_actionLog(action_log),
      m_graphInfo(graph_info),
      m_originalGraph(original_graph),
      m_timerGraph(timer_graph),
      m_nodeSelection(new NodeSelection(m_timerGraph)) {
	m_focusNode = params.getIntDefault("focus", -1);
}

EventGraphDisplay::~EventGraphDisplay() {
	delete m_nodeSelection;
}

void EventGraphDisplay::tryIncludeNode(int node_id, int priority, const std::string& comment) {
	m_nodeSelection->addNode(node_id, priority);
	// Append comment to the node.
	if (!comment.empty()) {
		std::string& caption = m_captions[node_id];
		if (!caption.empty()) {
			caption.append("\\n");
		}
		caption.append(comment);
	}
}

void EventGraphDisplay::addRaceArc(int race_id, const VarsInfo::RaceInfo& race, const char* color) {
	m_raceArcs.push_back(Race(race_id, race, color));
}

void EventGraphDisplay::outputGraph(const ActionLogPrinter* action_printer, std::string* output) {
	m_nodeSelection->addNode(m_focusNode, NODE_FOCUS);
	m_nodeSelection->addNode(0, NODE_FIRST_NODE);
	int node_id;
	while (m_nodeSelection->getNode(&node_id) && m_includedNodes.size() < 10) {
		if (m_graphInfo->isNodeDropped(node_id)) continue;
		addNode(action_printer, node_id);
	}
	for (std::set<int>::const_iterator it1 = m_includedNodes.begin();
			it1 != m_includedNodes.end(); ++it1) {
		for (std::set<int>::const_iterator it2 = m_includedNodes.begin();
				it2 != m_includedNodes.end(); ++it2) {
			addArcIfThere(*it1, *it2);
		}
	}
	for (size_t i = 0; i < m_raceArcs.size(); ++i) {
		const VarsInfo::RaceInfo& race = *m_raceArcs[i].m_varInfo;
		if (m_includedNodes.count(race.m_event1) &&
				m_includedNodes.count(race.m_event2)) {
			m_graphViz.getArc(race.m_event1, race.m_event2)->m_color = m_raceArcs[i].m_color;
			m_graphViz.getArc(race.m_event1, race.m_event2)->m_fontColor = m_raceArcs[i].m_color;
			m_graphViz.getArc(race.m_event1, race.m_event2)->m_label = race.TypeStr();
			m_graphViz.getArc(race.m_event1, race.m_event2)->m_style = "dashed";
			m_graphViz.getArc(race.m_event1, race.m_event2)->m_arrowHead = "dot";
			StringAppendF(&m_graphViz.getArc(race.m_event1, race.m_event2)->m_url, "race?focus=%d&id=%d", m_focusNode, m_raceArcs[i].m_id);
		}
	}
	m_graphViz.output(m_fileName, output);
}

void EventGraphDisplay::addNode(const ActionLogPrinter* action_printer, int node_id) {
	m_includedNodes.insert(node_id);

	GraphViz::Node* node = m_graphViz.getNode(node_id);

	URLParams nodeURL = m_params;
	nodeURL.setInt("focus", node_id);
	if (node_id == m_focusNode) {
		node->m_color = "red";
	}
	switch (m_actionLog->event_action(node_id).m_type) {
	case ActionLog::UNKNOWN:
		node->m_shape = "box";
		break;
	case ActionLog::TIMER:
		node->m_shape = "hexagon";
		break;
	case ActionLog::USER_INTERFACE:
		node->m_shape = "doubleoctagon";
		break;
	case ActionLog::NETWORK:
		node->m_shape = "ellipse";
		break;
	case ActionLog::CONTINUATION:
		node->m_shape = "hexagon";
		break;
	}
	node->m_url = m_linkCmd + "?" + nodeURL.toString();

	action_printer->getEventActionSummary(node_id, "\\n", &node->m_label);
	if (!m_captions[node_id].empty()) {
		node->m_label.append("\\n");
		node->m_label.append(m_captions[node_id]);
		node->m_style = "filled";
		node->m_fillcolor = "lightgrey";
	}
}

void EventGraphDisplay::addArcIfThere(int source, int target) {
	if (source >= target) return;
	if (m_timerGraph->hasArc(source, target)) {
		if (m_originalGraph->hasArc(source, target)) {
			m_graphViz.getArc(source, target)->m_style = "bold";
		} else {
			m_graphViz.getArc(source, target)->m_color = "blue";
		}
		int duration = m_graphInfo->getArcDuration(source, target);
		if (duration >= 0) {
			m_graphViz.getArc(source, target)->m_duration = duration;
		}
	} else {
		SimpleDirectedGraph::BFIterator it(*m_timerGraph, 0x3fffffff, true);
		it.addNode(source);
		int node_id;
		while (it.readNoAddFollowers(&node_id)) {
			if (node_id == target) {
				m_graphViz.getArc(source, target)->m_style = "dotted";
			}
			if (node_id < target &&
					(node_id == source || m_includedNodes.count(node_id) == 0)) {
				it.addNodeFollowers(node_id);
			}
		}
	}
}
