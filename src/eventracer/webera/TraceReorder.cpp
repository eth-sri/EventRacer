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


#include "TraceReorder.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <map>

#include "file.h"

TraceReorder::TraceReorder() {
}

TraceReorder::~TraceReorder() {
}

void TraceReorder::LoadSchedule(const char* filename) {
	fprintf(stderr, "Loading schedule %s...", filename);
	m_actions.clear();
	FILE* f = fopen(filename, "rt");
	std::string line;
	while (ReadLine(f, &line)) {
		int node_id;
		if (sscanf(line.c_str(), "%d;", &node_id) == 1) {
			if (node_id >= static_cast<int>(m_actions.size())) {
				m_actions.resize(node_id + 1, std::string(""));
			}

			m_actions[node_id] = std::string(strchr(line.c_str(), ';') + 1);
		}
	}
	fclose(f);
	fprintf(stderr, "Done\n");
}

void TraceReorder::SaveSchedule(const char* filename, const std::vector<int>& schedule) const {
	fprintf(stderr, "Saving schedule to %s...\n", filename);
	FILE* f = fopen(filename, "wt");
	int maxi = m_actions.size();
	for (size_t i = 0; i < schedule.size(); ++i) {
		if (schedule[i] >= 0 && schedule[i] < maxi) {
			const std::string& s = m_actions[schedule[i]];
			if (!s.empty()) {
				fprintf(f, "%d;%s\n", schedule[i], s.c_str());
			}
		}
	}
	fclose(f);
}

bool TraceReorder::GetSchedule(
		const std::vector<Reverse>& reverses,
		const SimpleDirectedGraph& graph,
		std::vector<int>* schedule) {

	std::vector<int> in_degree(graph.numNodes(), 0);
	std::vector<std::vector<int> > rev_succ(graph.numNodes());
	for (size_t i = 0; i < reverses.size(); ++i) {
		rev_succ[reverses[i].node2].push_back(reverses[i].node1);
	}

	std::set<int> to_output;
	for (int node_id = 0; node_id < graph.numNodes(); ++node_id) {
		const std::vector<int>& succs1 = graph.nodeSuccessors(node_id);
		for (size_t i = 0; i < succs1.size(); ++i) {
			if (succs1[i] < node_id) fprintf(stderr, "Reverse arc\n");
			++in_degree[succs1[i]];
		}
		const std::vector<int>& succs2 = rev_succ[node_id];
		for (size_t i = 0; i < succs2.size(); ++i) {
			++in_degree[succs2[i]];
		}
		to_output.insert(node_id);
	}

	size_t last_num_output;
	schedule->clear();
	do {
		last_num_output = schedule->size();
		for (int node_id = 0; node_id < graph.numNodes(); ++node_id) {
			if (in_degree[node_id] == 0 && to_output.count(node_id) > 0) {
				to_output.erase(node_id);
				schedule->push_back(node_id);
				const std::vector<int>& succs1 = graph.nodeSuccessors(node_id);
				for (size_t i = 0; i < succs1.size(); ++i) {
					--in_degree[succs1[i]];
				}
				const std::vector<int>& succs2 = rev_succ[node_id];
				for (size_t i = 0; i < succs2.size(); ++i) {
					--in_degree[succs2[i]];
				}
			}
		}
	} while (last_num_output != schedule->size());

	return static_cast<int>(schedule->size()) == graph.numNodes();  // If all nodes were added, this is a success.
}


