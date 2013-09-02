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

#include <limits.h>
#include <algorithm>
#include <set>

#include "TimerGraph.h"

class OrderArcs {
public:
	bool operator()(const ActionLog::Arc& a1, const ActionLog::Arc& a2) const {
		if (a1.m_tail == a2.m_tail) return a1.m_head < a2.m_head;
		return a1.m_tail < a2.m_tail;
	}
};

TimerGraph::TimerGraph(const std::vector<ActionLog::Arc>& arcs, const SimpleDirectedGraph& graph) {
	int num_timed_arcs = 0;
	for (size_t i = 0; i < arcs.size(); ++i) {
		if (arcs[i].m_duration >= 0 &&
				!graph.isNodeDeleted(arcs[i].m_head) &&
				!graph.isNodeDeleted(arcs[i].m_tail)) {
			m_timedArcs.push_back(arcs[i]);
			++num_timed_arcs;
		}
	}
	std::sort(m_timedArcs.begin(), m_timedArcs.end(), OrderArcs());
	printf("Using %d timed arcs\n", num_timed_arcs);
}

void TimerGraph::build(SimpleDirectedGraph* graph) {
	std::vector<int> min_outgoing_duration(graph->numNodes(), 0x3fffffff);
	std::vector<std::vector<int> > outgoing_arc_indices(graph->numNodes());

	int num_added_arcs = 0;
	for (size_t arci = 0; arci < m_timedArcs.size(); ++arci) {
		const ActionLog::Arc& arc = m_timedArcs[arci];
		if (arci % 1000 == 999) {
			printf("Adding timed arcs %f%% done. %d arcs added.\n",
					(arci * 100.0) / m_timedArcs.size(), num_added_arcs);
		}
		SimpleDirectedGraph::BFIterator it(*graph, 0x3fffffff, false);
		it.addNode(arc.m_tail);
		int node_id;

		std::set<int> covered_arcs;
		while (it.readNoAddFollowers(&node_id)) {

			// All previous timers with lower or equal duration are before the current one.
			if (min_outgoing_duration[node_id] <= arc.m_duration) {
				const std::vector<int>& node_arcs = outgoing_arc_indices[node_id];
				for (size_t i = node_arcs.size(); i > 0;) {
					--i;
					const ActionLog::Arc& prev_arc = m_timedArcs[node_arcs[i]];
					if (it.isVisited(prev_arc.m_head)) continue;
					if (prev_arc.m_duration <= arc.m_duration) {
						// TODO(veselin): This is a bit slow for the cases when there are several seconds long arcs.
						if (covered_arcs.count(prev_arc.m_head) == 0) {
							num_added_arcs += graph->addArcIfNeeded(prev_arc.m_head, arc.m_head);
						}
						covered_arcs.insert(
								graph->nodePredecessors(prev_arc.m_head).begin(),
								graph->nodePredecessors(prev_arc.m_head).end());
					}
					if (prev_arc.m_duration == arc.m_duration) break;
				}
			}
			// If the previous timer has the same duration as the current one,
			// there's no need to explore more backwards.
			// If (the previous had strictly lower duration, there may be other
			// timers with higher than the previous duration, but lower that the
			// current duration).
			if (min_outgoing_duration[node_id] != arc.m_duration) {
				it.addNodeFollowers(node_id);
			}
		}

		min_outgoing_duration[arc.m_tail] = std::min(
				min_outgoing_duration[arc.m_tail],
				arc.m_duration);
		outgoing_arc_indices[arc.m_tail].push_back(arci);
	}
	printf("Timers added %d new arcs\n", num_added_arcs);
}
