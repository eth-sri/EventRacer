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

#include "RaceStats.h"

#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include <utility>

#include <stdio.h>
#include "string.h"
#include "stringprintf.h"
#include "strutil.h"

#include "ActionLog.h"
#include "RaceTags.h"
#include "GraphFix.h"
#include "TimerGraph.h"

using std::string;

namespace {
void dropNoFollowerEmptyEvents(
		const ActionLog& log,
		SimpleDirectedGraph* event_graph,
		EventGraphInfo* graph_info) {
	int num_dropped_events = 0;
	for (int i = event_graph->numNodes(); i > 0;) {
		--i;
		if (event_graph->nodeSuccessors(i).size() == 0 &&
				log.event_action(i).m_commands.size() == 0) {
			event_graph->deleteNode(i);
			graph_info->dropNode(i);
			++num_dropped_events;
		}
	}
	printf("Dropped %d events.\n", num_dropped_events);
}
}  // namespace

RaceFile::RaceFile()
    : m_tags(m_vinfo, m_actions, m_vars, m_scopes, m_memValues, m_eventCauseFinder) {
}

bool RaceFile::Load(const std::string& filename, bool eval_race_detector_time) {
	m_filename = filename;
	size_t pos = m_filename.find("LOG.");
	if (pos != std::string::npos) {
		m_filename = m_filename.substr(pos + 4, m_filename.size() - pos - 4);
	}
	printf("Loading %s...\n", filename.c_str());
	FILE* f = fopen(filename.c_str(), "rb");
	if (!f) {
		fprintf(stderr, "ERROR in fopen()\n");
		return false;
	}
	bool result = true;
	result &= m_vars.loadFromFile(f);
	result &= m_scopes.loadFromFile(f);
	result &= m_actions.loadFromFile(f);
	if (!feof(f)) {
		result &= m_js.loadFromFile(f);
	}
	if (!feof(f)) {
		result &= m_memValues.loadFromFile(f);
	}

	m_fileSize = ftell(f);

	fclose(f);
	printf("DONE\n");

	m_inputEventGraph.addNodesUpTo(m_actions.maxEventActionId());
	int num_arcs = 0, num_arcs_needed = 0;
	for (size_t i = 0; i < m_actions.arcs().size(); ++i) {
		const ActionLog::Arc& arc = m_actions.arcs()[i];
		if (arc.m_tail > arc.m_head) {
			fprintf(stderr, "Unexpected backwards arc %d -> %d\n", arc.m_tail, arc.m_head);
		}
		num_arcs_needed +=
				m_inputEventGraph.addArcIfNeeded(arc.m_tail, arc.m_head);
		++num_arcs;
	}
	printf("Created graph with %d nodes, %d arcs (%d in input).\n",
			m_inputEventGraph.numNodes(), num_arcs_needed, num_arcs);

	m_graphInfo.init(m_actions);

	m_eventCauseFinder.Init(m_actions, m_inputEventGraph);
	EventGraphFixer fixer(&m_actions, &m_vars, &m_scopes, &m_inputEventGraph, &m_graphInfo);
	fixer.dropNoFollowerEmptyEvents();
	fixer.makeIndependentEventExploration();
	fixer.addScriptsAndResourcesHappensBefore();
	fixer.addEventAfterTargetHappensBefore();
	m_vinfo.init(m_actions);

	printf("Variables loaded.\n");
	printf("Building timers graph...\n");
	m_graphWithTimers = m_inputEventGraph;
	TimerGraph timer_graph(m_actions.arcs(), m_graphWithTimers);
	timer_graph.build(&m_graphWithTimers);
	printf("Timers graph done.\n");

	printf("Checking for races...\n");
	int64 start_time = GetCurrentTimeMicros();
	m_vinfo.findRaces(m_actions, m_graphWithTimers);
	printf("Done checking for races... %lld ms\n", (GetCurrentTimeMicros() - start_time) / 1000);

	if (eval_race_detector_time && !m_vinfo.timedOut()) {
		int race_times[5];
		int init_times[5];
		race_times[0] = m_vinfo.timeToFindRacesMs();
		init_times[0] = m_vinfo.timeInitMs();
		for (int i = 1; i < 5; ++i) {
			VarsInfo vinfo1;
			vinfo1.init(m_actions);
			vinfo1.findRaces(m_actions, m_graphWithTimers);
			race_times[i] = vinfo1.timeToFindRacesMs();
			init_times[i] = vinfo1.timeInitMs();
		}
		std::sort(race_times, race_times + 5);
		std::sort(init_times, init_times + 5);

		for (int i = 0; i < 5; ++i) {
			printf("Times: %d,%d\n", race_times[i], init_times[i]);
		}
		m_timeToFindRacesMs = race_times[2];
		m_timeToInitRaceFinderMs = init_times[2];
	} else {
		m_timeToFindRacesMs = m_vinfo.timeToFindRacesMs();
		m_timeToInitRaceFinderMs = m_vinfo.timeInitMs();
	}

	return result;
}

const char* RaceFile::getOpName(int var_id, int op_id) const {
	VarsInfo::AllVarData::const_iterator var_it = m_vinfo.variables().find(var_id);
	if (var_it == m_vinfo.variables().end()) return "Unknown var";
	const VarsInfo::VarData& var = var_it->second;
	for (size_t i = 0; i < var.m_accesses.size(); ++i) {
		if (var.m_accesses[i].m_eventActionId != op_id) continue;
		std::vector<int> scope;
		m_eventCauseFinder.getCallTraceOfCommand(
				var.m_accesses[i].m_eventActionId,
				var.m_accesses[i].m_commandIdInEvent,
				&scope);
		if (scope.size() > 0) {
			int scope_loc = m_actions.event_action(op_id).m_commands[scope[0]].m_location;
			return m_scopes.getString(scope_loc);
		}
	}
	return "";
}

void RaceFile::printRaceEventStats(bool only_uncovered) {
	std::map<string, int> counts;
	for (size_t racei = 0; racei < m_vinfo.races().size(); ++racei) {
		const VarsInfo::RaceInfo& race = m_vinfo.races()[racei];
		if (only_uncovered && race.m_coveredBy != -1) {
			continue;
		}
		string op1 = getOpName(race.m_varId, race.m_event1);
		string op2 = getOpName(race.m_varId, race.m_event2);
		counts[op1 + "<" + op2]++;
	}

	std::vector<std::pair<int, string> > sorted_data;
	for (std::map<string, int>::const_iterator it = counts.begin(); it != counts.end(); ++it) {
		sorted_data.push_back(std::pair<int, string>(it->second, it->first));
	}
	std::sort(sorted_data.begin(), sorted_data.end());

	for (size_t i = 0; i < sorted_data.size(); ++i) {
		printf("  %80s  -> %d\n", sorted_data[i].second.c_str(), sorted_data[i].first);
	}
}

void RaceFile::printVarStats() {
	int num_vars = 0;
	int num_races = 0;
	int num_uncover1_races = 0;
	int num_uncovered_races = 0;
	int num_remaining_races = 0;

	int num_same_value = 0;
	int num_only_local_write = 0;
	int num_event_attach = 0;
	int num_lazy_init = 0;
	int num_cookie = 0;
	int num_unload = 0;

	int num_unclassified_init_races = 0;
	int num_init_races = 0;
	int num_net_races = 0;

	const VarsInfo::AllVarData& all_vars = m_vinfo.variables();
	for (VarsInfo::AllVarData::const_iterator it = all_vars.begin(); it != all_vars.end(); ++it) {
		int var_id = it->first;
		const VarsInfo::VarData& var = it->second;
		++num_vars;
		if (var.m_allRaces.empty()) continue;
		++num_races;
		if (var.m_noParentRaces.empty()) continue;
		++num_uncover1_races;
		bool all_multi_cover = true;
		for (size_t i = 0; i < var.m_noParentRaces.size(); ++i) {
			int race_id = var.m_noParentRaces[i];
			bool is_multi_covered = m_vinfo.races()[race_id].m_multiParentRaces.size() != 0;
			all_multi_cover &= is_multi_covered;
		}
		if (all_multi_cover) continue;
		++num_uncovered_races;

		RaceTags::RaceTagSet tags = m_tags.getVariableTags(var_id);
		if (RaceTags::hasTag(tags, RaceTags::WRITE_SAME_VALUE)) ++num_same_value;
		if (RaceTags::hasTag(tags, RaceTags::ONLY_LOCAL_WRITE)) ++num_only_local_write;
		if (RaceTags::hasTag(tags, RaceTags::LATE_EVENT_ATTACH) ||
				RaceTags::hasTag(tags, RaceTags::NO_EVENT_ATTACHED)) ++num_event_attach;
		if (RaceTags::hasTag(tags, RaceTags::MAYBE_LAZY_INIT)) ++num_lazy_init;
		if (RaceTags::hasTag(tags, RaceTags::COOKIE_OR_CLASSNAME)) ++num_cookie;
		if (RaceTags::hasTag(tags, RaceTags::RACE_WITH_UNLOAD)) ++num_unload;

		if (tags == RaceTags::emptyTagSet()) ++num_remaining_races;

		if (m_tags.hasUndefinedInitilizationRace(var_id)) {
			++num_unclassified_init_races;
			if (tags == RaceTags::emptyTagSet()) ++num_init_races;
		}
		if (m_tags.hasNetworkResponseRace(var_id, false) && tags == RaceTags::emptyTagSet()) {
			++num_net_races;
		}
	}

	printf("%25s ,%7d,%7d,%7d,%7d,%4d,%4d,%4d,%4d,%4d,%4d,%6d,%5d,%5d,%5d\n",
			m_filename.c_str(),
			num_vars, num_races, num_uncover1_races, num_uncovered_races,
			num_same_value, num_only_local_write, num_event_attach, num_lazy_init, num_cookie, num_unload,
			num_remaining_races, num_unclassified_init_races, num_init_races, num_net_races);
}

void RaceFile::printVarStatsHeader() {
	printf("%25s ,NumVars,NumRace,Uncovr1,Uncover,SAME,LOCL,EVNT,LAZY,COOK,UNLD,Remain,InitU,InitR,Net_R\n",
		"Filename");
}

void RaceFile::printTimeStats() {
	printf("%25s,%8s,%8d,%8d,%5d,%8d,%8d,%8d,%7d,%9lld\n",
			m_filename.c_str(),
			m_vinfo.timedOut() ? "TIMEOUT" : "OK",
			m_vinfo.numNodes(), m_vinfo.numArcs(), m_vinfo.numChains(), m_vinfo.calculateFastTrackNumVCs(),
			m_timeToFindRacesMs, m_timeToInitRaceFinderMs,
			static_cast<int>(m_vinfo.races().size()),
			m_fileSize);
}

void RaceFile::printHighRiskRaces() {
	const VarsInfo::AllVarData& all_vars = m_vinfo.variables();
	for (VarsInfo::AllVarData::const_iterator it = all_vars.begin(); it != all_vars.end(); ++it) {
		int var_id = it->first;
		const VarsInfo::VarData& var = it->second;
		if (var.m_allRaces.empty()) continue;
		if (var.m_noParentRaces.empty()) continue;
		bool all_multi_cover = true;
		for (size_t i = 0; i < var.m_noParentRaces.size(); ++i) {
			int race_id = var.m_noParentRaces[i];
			bool is_multi_covered = m_vinfo.races()[race_id].m_multiParentRaces.size() != 0;
			all_multi_cover &= is_multi_covered;
		}
		if (all_multi_cover) continue;
		RaceTags::RaceTagSet tags = m_tags.getVariableTags(var_id);
		if (tags == RaceTags::emptyTagSet()) {
			bool isInit = m_tags.hasUndefinedInitilizationRace(var_id);
			bool isNet = m_tags.hasNetworkResponseRace(var_id, false);
			if (isInit || isNet) {
				printf("%25s : %s%s* %s\n", m_filename.c_str(),
						isNet ? "N" : "", isInit ? "I" : "",
						m_vars.getString(var_id));
			}
		}
	}
}

void RaceFile::printSimpleStats() {
	int max_child_races = -1;
	int max_child_race_vars = -1;
	int top_race_id = -1;
	int num_uncovered_races = 0;
	std::set<int> uncovered_race_vars;
	for (size_t racei = 0; racei < m_vinfo.races().size(); ++racei) {
		const VarsInfo::RaceInfo& race = m_vinfo.races()[racei];
		if (race.m_coveredBy == -1) {
			uncovered_race_vars.insert(race.m_varId);
			++num_uncovered_races;
			//int num_child_races = 0;
			//for (size_t i = 0; i < race.m_childRaces.size(); ++i) {
			//	const VarsInfo::RaceInfo& crace = m_vinfo.races()[race.m_childRaces[i]];
			//	if (crace.m_op1 != race.m_op1 || crace.m_op2 != race.m_op2) ++num_child_races;
			//}
			int num_child_races = race.m_childRaces.size();
			std::set<int> child_race_vars;
			for (size_t i = 0; i < race.m_childRaces.size(); ++i) {
				const VarsInfo::RaceInfo& crace = m_vinfo.races()[race.m_childRaces[i]];
				child_race_vars.insert(crace.m_varId);
			}
			int num_child_race_vars = child_race_vars.size();

			if (num_child_races > max_child_races) {
				max_child_races = num_child_races;
				max_child_race_vars = num_child_race_vars;
				top_race_id = racei;
			}
		}
	}
	std::string top_race;
	if (top_race_id >= 0) {
		const VarsInfo::RaceInfo& race = m_vinfo.races()[top_race_id];
		//const VarsInfo::VarData& var = m_vinfo.variables()[race.m_varId];
		StringAppendF(&top_race, "%s %s",
				race.TypeStr(),
				m_vars.getString(race.m_varId));
	}

	printf("%25s | Races:%5d | Uncovered:%5d  (%5d vars) | Top covers:%5d (%5d vars) is %s\n",
			m_filename.c_str(),
			static_cast<int>(m_vinfo.races().size()),
			num_uncovered_races,
			static_cast<int>(uncovered_race_vars.size()),
			max_child_races,
			max_child_race_vars,
			top_race.c_str());
}

int RaceFile::numRaces() const {
	return m_vinfo.races().size();
}

void RaceFile::evaluateAccordionClocks() {
	std::map<int, int> loc_to_op_id;
	std::vector<int> cmds_in_op_id(m_actions.maxEventActionId() + 1, 0);
	for (int op_id = 0; op_id < m_actions.maxEventActionId(); ++op_id) {
		const ActionLog::EventAction& op = m_actions.event_action(op_id);
		for (size_t cmd_id = 0; cmd_id < op.m_commands.size(); ++cmd_id) {
			const ActionLog::Command& cmd = op.m_commands[cmd_id];
			if (cmd.m_cmdType == ActionLog::READ_MEMORY ||
				cmd.m_cmdType == ActionLog::WRITE_MEMORY) {
				const char* var_name = m_vars.getString(cmd.m_location);
				if (strncmp(var_name, "Window", 6) == 0 ||
					strncmp(var_name, "Tree", 4) == 0 ||
					strncmp(var_name, "NodeTree", 8) == 0) {
					std::map<int, int>::const_iterator it = loc_to_op_id.find(cmd.m_location);
					if (it != loc_to_op_id.end()) {
						cmds_in_op_id[it->second]--;
					}
					loc_to_op_id[cmd.m_location] = op_id;
					cmds_in_op_id[op_id]++;
				}
			}
		}
	}

	int num_live_objects = 0;
	for (size_t i = 0; i < cmds_in_op_id.size(); ++i) {
		if (cmds_in_op_id[i] > 0) ++num_live_objects;
	}
	printf("%s,%d,%d,%d,%d\n", m_filename.c_str(), m_vinfo.numNodes(), m_vinfo.numArcs(), m_vinfo.numChains(), num_live_objects);

	//m_vinfo.fast_event_graph()
}



