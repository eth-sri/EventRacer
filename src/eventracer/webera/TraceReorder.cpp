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

#include "VarsInfo.h"

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
			while (node_id >= static_cast<int>(m_actions.size())) {
				m_actions.push_back(std::string(""));
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
		if (schedule[i] == -2) {
			fprintf(f, "<change>\n");
		}
		if (schedule[i] == -1) {
			fprintf(f, "<relax>\n");
		}
		if (schedule[i] >= 0 && schedule[i] < maxi) {
			const std::string& s = m_actions[schedule[i]];
			if (!s.empty()) {
				fprintf(f, "%d;%s\n", schedule[i], s.c_str());
			}
		}
	}
	fclose(f);
}

bool TraceReorder::GetScheduleFromRaces(
		const VarsInfo& vinfo,
		const std::vector<int>& rev_races,
		const SimpleDirectedGraph& graph,
		const Options& options,
		std::vector<int>* schedule) const {

	std::vector<TraceReorder::Reverse> all_reverses;
	std::set<int> non_reversed_races;
	for (size_t race_id = 0; race_id < vinfo.races().size(); ++race_id) {
		const VarsInfo::RaceInfo& race = vinfo.races()[race_id];
		if (race.m_coveredBy == -1 && race.m_multiParentRaces.empty()) {
			non_reversed_races.insert(static_cast<int>(race_id));
		}
	}

	for (size_t i = 0; i < rev_races.size(); ++i) {
		int race_id = rev_races[i];
		if (race_id >= 0 && race_id < static_cast<int>(vinfo.races().size())) {
			const VarsInfo::RaceInfo& race = vinfo.races()[race_id];
			TraceReorder::Reverse rev;
			rev.node1 = race.m_event1;
			rev.node2 = race.m_event2;
			all_reverses.push_back(rev);

			non_reversed_races.erase(race_id);
			non_reversed_races.erase(race.m_coveredBy);
			for (size_t j = 0; j < race.m_multiParentRaces.size(); ++j) {
				non_reversed_races.erase(race.m_multiParentRaces[j]);
			}
		}
	}

	// Do not reverse any other races.
	std::vector<TraceReorder::Preserve> all_preserves;
	for (std::set<int>::const_iterator it = non_reversed_races.begin(); it != non_reversed_races.end(); ++it) {
		int race_id = *it;
		const VarsInfo::RaceInfo& race = vinfo.races()[race_id];
		TraceReorder::Preserve pres;
		pres.node1 = race.m_event1;
		pres.node2 = race.m_event2;
		bool can_enforce = true;
		for (size_t i = 0; i < all_reverses.size(); ++i) {
			const TraceReorder::Reverse& rev = all_reverses[i];
			if (rev.node2 == pres.node2 && rev.node1 <= pres.node1) {
				// Avoid enforcing races that "could" cause a cycle.
				// Note: in this case we are predicting "could" without actually checking the
				// happens-before, but over-approximating.
				can_enforce = false;
			}
		}

		if (can_enforce) {
			all_preserves.push_back(pres);
		}
//		printf("%d %d %d\n", race_id, race.m_event1, race.m_event2);
	}

	return GetSchedule(all_reverses, all_preserves, graph, options, schedule);
}

bool TraceReorder::GetSchedule(
		const std::vector<Reverse>& reverses,
		const std::vector<Preserve>& preserves,
		const SimpleDirectedGraph& graph,
		const Options& options,
		std::vector<int>* schedule) const {

	int num_reverses = 0;

	std::vector<int> in_degree(graph.numNodes(), 0);
	std::vector<std::vector<int> > rev_succ(graph.numNodes());
	for (size_t i = 0; i < reverses.size(); ++i) {
		rev_succ[reverses[i].node2].push_back(reverses[i].node1);
//		printf("Reverse %d < %d\n", reverses[i].node2, reverses[i].node1);
	}

	std::vector<std::vector<int> > pres_succ(graph.numNodes());
	for (size_t i = 0; i < preserves.size(); ++i) {
		pres_succ[preserves[i].node1].push_back(preserves[i].node2);
//		printf("Preserve %d < %d\n", preserves[i].node1, preserves[i].node2);
	}

	std::set<int> to_output;
	for (int node_id = 0; node_id < graph.numNodes(); ++node_id) {
		const std::vector<int>& succs1 = graph.nodeSuccessors(node_id);
		for (size_t i = 0; i < succs1.size(); ++i) {
			if (succs1[i] < node_id) fprintf(stderr, "Reverse arc\n");
			++in_degree[succs1[i]];
		}
		// Arcs to preserve.
		const std::vector<int>& succs3 = pres_succ[node_id];
		for (size_t i = 0; i < succs3.size(); ++i) {
			++in_degree[succs3[i]];
		}

		const std::vector<int>& succs2 = rev_succ[node_id];
		for (size_t i = 0; i < succs2.size(); ++i) {
			++in_degree[succs2[i]];
			++num_reverses;
		}
		to_output.insert(node_id);
	}

	// Number of events produced in the output.
	int last_num_output, num_output;
	bool change_marker_emitted = false;
	num_output = 0;
	schedule->clear();
	do {
		last_num_output = num_output;
		for (int node_id = 0; node_id < graph.numNodes(); ++node_id) {
			if (in_degree[node_id] == 0 && to_output.count(node_id) > 0) {
				if (options.include_change_marker &&
					change_marker_emitted == false &&
					!rev_succ[node_id].empty()) {
					schedule->push_back(-2);  // <change> marker.
					change_marker_emitted = true;
				}

				to_output.erase(node_id);
				schedule->push_back(node_id);
				++num_output;
				const std::vector<int>& succs1 = graph.nodeSuccessors(node_id);
				for (size_t i = 0; i < succs1.size(); ++i) {
					--in_degree[succs1[i]];
				}
				const std::vector<int>& succs3 = pres_succ[node_id];
				for (size_t i = 0; i < succs3.size(); ++i) {
					--in_degree[succs3[i]];
				}
				const std::vector<int>& succs2 = rev_succ[node_id];
				for (size_t i = 0; i < succs2.size(); ++i) {
					--in_degree[succs2[i]];
					--num_reverses;
					if (num_reverses == 0 && options.relax_replay_after_all_races) {
						// -1 is a relax tag in the schedule. It does not count as an event.
						schedule->push_back(-1);  // <relax> marker
					}

					if (options.minimize_variation_from_original) {
						// Move the next node_id to the reversal target. Then the trace replay will try to be
						// close to normal.
						if (succs2[i] < node_id) node_id = succs2[i] - 1;
					}
				}
			}
		}
	} while (last_num_output != num_output);

	return num_output == graph.numNodes();  // If all nodes were added, this is a success.
}


