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

#include "VarsInfo.h"

#include "ActionLog.h"
#include "BitClocks.h"
#include "EventGraph.h"
#include "ThreadMapping.h"

#include "gflags/gflags.h"

#include <algorithm>
#include <set>
#include <utility>
#include <queue>


DEFINE_string(graph_connectivity_algorithm, "CD",
		"Graph connectivity algorithm. Can be one of CD - chain decomposition,"
		"BVC - bit vector clocks, BFS - breadth first search.");
DEFINE_int64(race_detection_timeout_seconds, 0, "If the timeout is set to a "
		"positive integer, race detection algorithms fail if computation takes"
		" more than the specified number of seconds.");


// Checks races for multi-coverage.
class RaceGraph {
public:
	RaceGraph(const VarsInfo& vars,
			  const EventGraphInterface& graph)
	    : m_vars(vars), m_races(vars.races()), m_graph(graph) {
		initTopRaces();
	}

	// Build a graph with edge between a pair of races (rj, ri) if the
	// second event of rj is before the first event of ri.
	void buildTopGraph() {
		m_topGraph.assign(m_topRaces.size(), std::vector<int>());
		for (size_t j = 0; j < m_topRaces.size(); ++j) {
			const VarsInfo::RaceInfo& rj = getRace(j);
			for (size_t i = j + 1; i < m_topRaces.size(); ++i) {
				const VarsInfo::RaceInfo& ri = getRace(i);
				if (m_graph.areOrdered(rj.m_event2, ri.m_event1)) {
					m_topGraph[j].push_back(i);
				}
			}
		}
	}

	void checkCoverage(VarsInfo::AllRaces* all_races) {
		int numMultiCovered = 0;
		for (size_t j = 0; j < m_topRaces.size(); ++j) {
			if (isMultiCovered(j, &(*all_races)[m_topRaces[j]].m_multiParentRaces)) {
				++numMultiCovered;
			}
		}
		printf("%d are multi-covered\n", numMultiCovered);
	}

	// Returns if there is a path in the graph from node1 to the given command in node2,
	// where we can also follow existing races.
	// If the function return true, the path via the races is in race_path.
	bool hasPathViaRaces(int node1, int node2, int cmd_in_node2,
			std::vector<int>* race_path) const {
		race_path->clear();
		if (node1 > node2) return false;
		if (m_graph.areOrdered(node1, node2)) return true;

		// Breadth-first search over races.
		std::vector<int> parent(m_topRaces.size(), -2);  // -2 means not visited.
		std::queue<int> q;
		for (size_t i = 0; i < m_topRaces.size(); ++i) {
			if (m_graph.areOrdered(node1, getRace(i).m_event1)) {
				q.push(i);
				parent[i] = -1;  // No parent, but visited.
			}
		}
		while (!q.empty()) {
			int currId = q.front();
			q.pop();
			const VarsInfo::RaceInfo& curr = getRace(currId);
			if (curr.m_event2 > node2) continue;
			if (!curr.canSynchronizeInThisOrder()) continue;

			if ((curr.m_event2 == node2 && curr.m_cmdInEvent2 < cmd_in_node2) ||
				(curr.m_event2 < node2 && m_graph.areOrdered(curr.m_event2, node2))) {
				while (currId >= 0) {
					race_path->push_back(m_topRaces[currId]);
					currId = parent[currId];
				}
				std::reverse(race_path->begin(), race_path->end());
				return true;
			}
			const std::vector<int>& next = m_topGraph[currId];
			for (size_t i = 0; i < next.size(); ++i) {
				if (parent[next[i]] == -2) {  // if not visited.
					q.push(next[i]);
					parent[next[i]] = currId;
				}
			}
		}
		return false;
	}

private:
	void initTopRaces() {
		for (size_t i = 0; i < m_races.size(); ++i) {
			if (m_races[i].m_coveredBy == -1) {
				m_topRaces.push_back(i);
			}
		}
		printf("Using %d uncovered races\n", static_cast<int>(m_topRaces.size()));
	}

	const VarsInfo::RaceInfo& getRace(int topId) const {
		return m_races[m_topRaces[topId]];
	}

	// A race R is multi-covered if there is a path from a race after the beginning of
	// R to a race before the end of R in the race graph.
	// If a race is multi-covered, covered_by is set to a list of races covering the race.
	bool isMultiCovered(int raceId, std::vector<int>* covered_by) const {
		const VarsInfo::RaceInfo& race = getRace(raceId);
		return hasPathViaRaces(race.m_event1, race.m_event2, race.m_cmdInEvent2, covered_by);
	}

	const VarsInfo& m_vars;
	const VarsInfo::AllRaces& m_races;
	const EventGraphInterface& m_graph;
	std::vector<int> m_topRaces;
	std::vector<std::vector<int> > m_topGraph;
};

//////////////////////////////////////////////////////////////////////////
// class VarsInfo
//////////////////////////////////////////////////////////////////////////


VarsInfo::VarsInfo() : m_startTime(0), m_timedOut(false), m_timeToFindRacesMs(0), m_numChains(0),
	m_fastEventGraph(NULL), m_raceGraph(NULL) {
}

VarsInfo::~VarsInfo() {
	delete m_fastEventGraph;
	delete m_raceGraph;
}

void VarsInfo::init(const ActionLog& actions) {
	for (int opid = 0; opid <= actions.maxEventActionId(); ++opid) {
		std::map<int, VarAccess> per_var_accesses;
		const ActionLog::EventAction& op = actions.event_action(opid);
		for (size_t cmdid = 0; cmdid < op.m_commands.size(); ++cmdid) {
			const ActionLog::Command& cmd = op.m_commands[cmdid];
			if (cmd.m_cmdType == ActionLog::WRITE_MEMORY) {
				VarAccess a;
				a.m_eventActionId = opid;
				a.m_commandIdInEvent = cmdid;
				a.m_isRead = false;
				m_vars[cmd.m_location].m_accesses.push_back(a);
			} else if (cmd.m_cmdType == ActionLog::READ_MEMORY) {
				VarAccess a;
				a.m_eventActionId = opid;
				a.m_commandIdInEvent = cmdid;
				a.m_isRead = true;
				m_vars[cmd.m_location].m_accesses.push_back(a);
			}
		}
	}
}

int VarsInfo::calculateFastTrackNumVCs() {
	if (m_timedOut) return -1;

	int num_allocated_vc = 0;

	for (AllVarData::iterator it = m_vars.begin(); it != m_vars.end(); ++it) {
		VarData& data = it->second;

		int last_read_id = -1;
		for (int i = 0; i < static_cast<int>(data.m_accesses.size()); ++i) {
			const VarAccess& currAccess = data.m_accesses[i];
			if (!currAccess.m_isRead) {
				last_read_id = -1;
			} else {
				if (last_read_id != -1) {
					const VarAccess& lastRead = data.m_accesses[last_read_id];
					if (!m_fastEventGraph->areOrdered(lastRead.m_eventActionId, currAccess.m_eventActionId)) {
						++num_allocated_vc;
					}
				}
				last_read_id = i;
			}
		}
	}

	return num_allocated_vc;
}

void VarsInfo::findRaces(const ActionLog& actions, const SimpleDirectedGraph& graph) {
	m_races.clear();


	m_numNodes = 0;
	m_numArcs = 0;
	for (int i = 0; i < graph.numNodes(); ++i) {
		m_numArcs += graph.nodeSuccessors(i).size();
		if (!graph.nodeSuccessors(i).empty() && !graph.nodePredecessors(i).empty()) {
			++m_numNodes;
		}
	}

	m_startTime = GetCurrentTimeMicros();
	m_numChains = 0;
	if (FLAGS_graph_connectivity_algorithm == "CD") {
		// Use vector clocks with chain decomposition.
		ThreadMapping* tmp = new ThreadMapping();
		tmp->build(graph);

		tmp->computeVectorClocks(graph);
		m_fastEventGraph = tmp;

		// Update statistics.
		m_numChains = tmp->num_threads();
	} else if (FLAGS_graph_connectivity_algorithm == "BFS") {
		// Use breadth-first search for connectivity algorithm.

		SimpleDirectedGraph* tmp = new SimpleDirectedGraph();
		*tmp = graph;
		m_fastEventGraph = tmp;
	} else if (FLAGS_graph_connectivity_algorithm == "BVC") {
		// Use bit vector clocks connectivity algorithm.

		BitClocks* tmp = new BitClocks();
		tmp->build(graph);
		m_fastEventGraph = tmp;
	}
	// Record how much time we needed for the connectivity algorithm initialization.
	m_initTime = (GetCurrentTimeMicros() - m_startTime) / 1000;


	int vars_ww = 0, vars_rw = 0, vars_wr = 0;

	// Perform race detection. The algorithm is as follows:
	//   - one pass forward finds all write-write and write-read races.
	//     this is, every read or write is checked for connectivity with the preceding
	//     write on the same variable.
	//     This should find the presence of write-write and write-read races.
	//   - a second pass backwards finds all read-write races. Every read is checked
	//     for connectivity with any following write.

	for (AllVarData::iterator it = m_vars.begin(); it != m_vars.end(); ++it) {
		VarData& data = it->second;

		int num_writes = data.numWrites();
		int num_reads = data.numReads();
		if (!(num_writes >= 2 || (num_writes >= 1 && num_reads >= 1))) {
			continue;
		}

		data.clearRaces();

		int last_write_id = -1;
		for (int i = 0; i < static_cast<int>(data.m_accesses.size()); ++i) {
			const VarAccess& currAccess = data.m_accesses[i];
			if (last_write_id != -1) {
				const VarAccess& lastWrite = data.m_accesses[last_write_id];
				if (!m_fastEventGraph->areOrdered(lastWrite.m_eventActionId, currAccess.m_eventActionId)) {
					// A write-write or write-read race was detected.
					m_races.push_back(RaceInfo(
									data.getVarAccessTypeForId(last_write_id),
									data.getVarAccessTypeForId(i),
									lastWrite.m_eventActionId,
									currAccess.m_eventActionId,
									lastWrite.m_commandIdInEvent,
									currAccess.m_commandIdInEvent,
									it->first));
					bool is_ww = !currAccess.m_isRead;
					if (is_ww) {
						++data.m_numWWRaces;
					} else {
						++data.m_numWRRaces;
					}
				}
			}
			if (!currAccess.m_isRead) {
				last_write_id = i;
			}
		}

		// Go in the reverse order of accesses to find read-write races.
		last_write_id = -1;
		for (int i = data.m_accesses.size(); i > 0;) {
			--i;
			const VarAccess& currAccess = data.m_accesses[i];
			if (last_write_id != -1) {
				const VarAccess& lastWrite = data.m_accesses[last_write_id];
				if (currAccess.m_isRead &&
						!m_fastEventGraph->areOrdered(currAccess.m_eventActionId, lastWrite.m_eventActionId)) {
					// A read-write race was detected.
					m_races.push_back(RaceInfo(
									data.getVarAccessTypeForId(i),
									data.getVarAccessTypeForId(last_write_id),
									currAccess.m_eventActionId,
									lastWrite.m_eventActionId,
									currAccess.m_commandIdInEvent,
									lastWrite.m_commandIdInEvent,
									it->first));
					++data.m_numRWRaces;
				}
			}
			if (!currAccess.m_isRead) {
				last_write_id = i;
			}
		}

		vars_ww += data.m_numWWRaces != 0;
		vars_rw += data.m_numRWRaces != 0;
		vars_wr += data.m_numWRRaces != 0;

		if (shouldTimeout()) break;
	}

	printf("Has %d vars with WW races, %d with RW and %d with WR.\n", vars_ww, vars_rw, vars_wr);
	findRaceDependency(actions);

	m_timeToFindRacesMs = (GetCurrentTimeMicros() - m_startTime) / 1000;
}

class Op2IsAfter {
public:
	bool operator() (const VarsInfo::RaceInfo& r1, const VarsInfo::RaceInfo& r2) const {
		if (r1.m_event2 == r2.m_event2) return r1.m_cmdInEvent2 < r2.m_cmdInEvent2;
		return r1.m_event2 < r2.m_event2;
	}
};

typedef std::pair<VarsInfo::RaceInfo, int> RaceAndPos;

class Op2IsAfterPair {
public:
	bool operator() (const RaceAndPos& r1, const RaceAndPos& r2) const {
		if (r1.first.m_event2 == r2.first.m_event2) {
			if (r1.first.m_cmdInEvent2 == r2.first.m_cmdInEvent2) return r1.second < r2.second;
			return r1.first.m_cmdInEvent2 < r2.first.m_cmdInEvent2;
		}
		return r1.first.m_event2 < r2.first.m_event2;
	}
};

namespace {
void RemapVector(const std::vector<int>& remapping, std::vector<int>* data) {
	for (size_t i = 0; i < data->size(); ++i) {
		(*data)[i] = remapping[(*data)[i]];
	}
}

}  // namespace

bool VarsInfo::shouldTimeout() {
	if (FLAGS_race_detection_timeout_seconds != 0) {
		bool result = (GetCurrentTimeMicros() - m_startTime) > FLAGS_race_detection_timeout_seconds * 1000000;
		if (result) {
			m_timedOut = true;
			fprintf(stderr, "Computation timed out.\n");
		}
		return result;
	}
	return false;
}

void VarsInfo::sortRaces() {
	if (m_races.empty()) return;
	std::vector<RaceAndPos> races(m_races.size(), RaceAndPos(m_races[0], 0));
	for (size_t i = 0; i < m_races.size(); ++i) {
		races[i].first = m_races[i];
		races[i].second = i;
	}
	std::sort(races.begin(), races.end(), Op2IsAfterPair());
	for (size_t i = 0; i < m_races.size(); ++i) {
		m_races[i] = races[i].first;
	}

	std::vector<int> race_id_remapping(m_races.size());
	for (size_t i = 0; i < m_races.size(); ++i) {
		// Create remapping : old position of a race to new position.
		race_id_remapping[races[i].second] = i;
	}
	for (AllVarData::iterator var_it = m_vars.begin(); var_it != m_vars.end(); ++var_it) {
		VarData& var = var_it->second;
		RemapVector(race_id_remapping, &var.m_childRaces);
		RemapVector(race_id_remapping, &var.m_parentRaces);
	}
}

void VarsInfo::findRaceDependency(const ActionLog& actions) {
	printf("Searching for race dependency...\n");
	sortRaces();

	for (size_t j = 0; j < m_races.size(); ++j) {
		m_races[j].m_coveredBy = -1;
		m_vars[m_races[j].m_varId].m_allRaces.push_back(j);
	}
	for (size_t j = 0; j < m_races.size(); ++j) {
		if (m_races[j].m_coveredBy != -1) continue;
		const RaceInfo& race1 = m_races[j];
		m_vars[race1.m_varId].m_noParentRaces.push_back(j);
		if (!race1.canSynchronizeInThisOrder()) continue;

		for (size_t i = j + 1; i < m_races.size(); ++i) {
			const RaceInfo& race2 = m_races[i];
			// race1 being a synchronization could prevent race2.

			if (m_fastEventGraph->areOrdered(race1.m_event2, race2.m_event2) &&
					m_fastEventGraph->areOrdered(race2.m_event1, race1.m_event1)) {
				m_vars[race1.m_varId].m_childRaces.push_back(i);
				m_vars[race2.m_varId].m_parentRaces.push_back(j);
				m_races[i].m_coveredBy = j;
				m_races[j].m_childRaces.push_back(i);
			}
		}
		if (shouldTimeout()) return;
	}

	printf("Searching for multi-race dependency...\n");
	findMultiRaceDependency(actions);
}

void VarsInfo::getDirectRaceChildren(int race_id, bool only_different_event_actions, std::set<int>* direct_child_races) const {
	const RaceInfo& base_race = m_races[race_id];

	std::vector<int> base_race_direct_children;
	for (size_t i = race_id + 1; i < m_races.size(); ++i) {
		const RaceInfo& race2 = m_races[i];

		if (only_different_event_actions && base_race.m_event1 == race2.m_event1 && base_race.m_event2 == race2.m_event2) {
			continue;
		}

		// base_race being a synchronization could prevent race2.
		if (m_fastEventGraph->areOrdered(base_race.m_event2, race2.m_event2) &&
				m_fastEventGraph->areOrdered(race2.m_event1, base_race.m_event1)) {
			// race2 is a child of base_race, but we do not know yet if it is a direct one.
			// Check if race2 is not covered by another child of base_race.
			bool covered_by_other_child = false;
			for (size_t j = 0; j < base_race_direct_children.size(); ++j) {
				const RaceInfo& race1 = m_races[base_race_direct_children[j]];
				if (!race1.canSynchronizeInThisOrder()) continue;
				// race1 being a synchronization could prevent race2.
				if (m_fastEventGraph->areOrdered(race1.m_event2, race2.m_event2) &&
						m_fastEventGraph->areOrdered(race2.m_event1, race1.m_event1)) {
					covered_by_other_child = true;
					break;
				}
			}
			if (!covered_by_other_child) {
				base_race_direct_children.push_back(i);
				direct_child_races->insert(i);
			}
		}
	}
}

bool VarsInfo::hasPathViaRaces(int node1, int node2, int cmd_in_node2,
		std::vector<int>* race_path) const {
	return m_raceGraph->hasPathViaRaces(node1, node2, cmd_in_node2, race_path);
}

void VarsInfo::findMultiRaceDependency(const ActionLog& actions) {
	delete m_raceGraph;
	m_raceGraph = new RaceGraph(*this, *m_fastEventGraph);
	m_raceGraph->buildTopGraph();
	m_raceGraph->checkCoverage(&m_races);
}

const char* VarsInfo::RaceInfo::TypeStr() const {
	switch (m_access1) {
	case MEMORY_READ: {
		switch (m_access2) {
		case MEMORY_READ: return "READ-READ";  // Should never happen.
		case MEMORY_WRITE: return "READ-WRITE";
		case MEMORY_UPDATE: return "READ-UPDATE";
		}
		break;
	}
	case MEMORY_WRITE: {
		switch (m_access2) {
		case MEMORY_READ: return "WRITE-READ";
		case MEMORY_WRITE: return "WRITE-WRITE";
		case MEMORY_UPDATE: return "WRITE-UPDATE";
		}
		break;
	}
	case MEMORY_UPDATE: {
		switch (m_access2) {
		case MEMORY_READ: return "UPDATE-READ";
		case MEMORY_WRITE: return "UPDATE-WRITE";
		case MEMORY_UPDATE: return "UPDATE-UPDATE";
		}
		break;
	}
	}
	return "Unknown race";
}

int VarsInfo::getCommandIdForVarReadInEventAction(const VarData& var, int event_action_id) {
	for (size_t i = 0; i < var.m_accesses.size(); ++i) {
		if (var.m_accesses[i].m_eventActionId == event_action_id && var.m_accesses[i].m_isRead) {
			return var.m_accesses[i].m_commandIdInEvent;
		}
	}
	return -1;
}

int VarsInfo::getCommandIdForVarWriteInEventAction(const VarData& var, int event_action_id) {
	for (size_t i = 0; i < var.m_accesses.size(); ++i) {
		if (var.m_accesses[i].m_eventActionId == event_action_id && !var.m_accesses[i].m_isRead) {
			return var.m_accesses[i].m_commandIdInEvent;
		}
	}
	return -1;
}

VarsInfo::VarAccessType VarsInfo::getVarAccessTypeInEventAction(const VarsInfo::VarData& var, int event_action_id) {
	int write_cmd = getCommandIdForVarWriteInEventAction(var, event_action_id);
	if (write_cmd == -1) {
		return VarsInfo::MEMORY_READ;
	} else {
		int read_cmd = getCommandIdForVarReadInEventAction(var, event_action_id);
		if (read_cmd == -1 || read_cmd > write_cmd) {
			return VarsInfo::MEMORY_WRITE;
		}
		return VarsInfo::MEMORY_UPDATE;
	}
}

const char* VarsInfo::RaceInfo::AccessStr(VarAccessType access) {
	switch (access) {
	case MEMORY_READ: return "READ";
	case MEMORY_WRITE: return "WRITE";
	case MEMORY_UPDATE: return "UPDATE";
	}
	return "Unknown access";
}
