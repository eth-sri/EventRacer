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

#include "GraphFix.h"

#include "ActionLog.h"
#include "EventGraph.h"
#include "EventGraphInfo.h"
#include "StringSet.h"
#include "stringprintf.h"

#include <map>
#include <string>
#include <string.h>

EventGraphFixer::EventGraphFixer(
		ActionLog* log,
		StringSet* vars,
		StringSet* scopes,
		SimpleDirectedGraph* event_graph,
		EventGraphInfo* graph_info)
    : m_log(log), m_vars(vars), m_scopes(scopes), m_eventGraph(event_graph), m_graphInfo(graph_info) {
}

EventGraphFixer::~EventGraphFixer() {
}

void EventGraphFixer::dropNoFollowerEmptyEvents() {
	int num_dropped_events = 0;
	for (int i = m_eventGraph->numNodes(); i > 0;) {
		--i;
		if (m_eventGraph->nodeSuccessors(i).size() == 0 &&
				m_log->event_action(i).m_commands.size() == 0) {
			m_eventGraph->deleteNode(i);
			m_graphInfo->dropNode(i);
			++num_dropped_events;
		}
	}
	printf("Dropped %d events.\n", num_dropped_events);
}

namespace {
bool getScriptOrResourceRunnerString(const char* varId, std::string* id) {
	if (strncmp(varId, "CachedResource-", 15) == 0) {
		id->assign(varId + 15);
		return true;
	}
	if (strncmp(varId, "ScriptRunner-", 13) == 0) {
		id->assign(varId + 13);
		return true;
	}
	return false;
}

bool getTargetNodeString(const char* varId, std::string* id) {
	if (strncmp(varId, "NodeTree:", 9) == 0) {
		id->assign(varId + 9);
		return true;
	}
	return false;
}
}  // namespace

void EventGraphFixer::addScriptsAndResourcesHappensBefore() {
	std::string script_id;
	int num_arcs_added = 0;
	std::map<std::string, int> m_lastLoc;
	for (int op_id = 0; op_id <= m_log->maxEventActionId(); ++op_id) {
		if (m_eventGraph->isNodeDeleted(op_id)) continue;
		const ActionLog::EventAction& op = m_log->event_action(op_id);
		for (size_t i = 0; i < op.m_commands.size(); ++i) {
			const ActionLog::Command& cmd = op.m_commands[i];
			if (cmd.m_cmdType == ActionLog::WRITE_MEMORY ||
					cmd.m_cmdType == ActionLog::READ_MEMORY) {
				if (getScriptOrResourceRunnerString(m_vars->getString(cmd.m_location), &script_id)) {
					std::map<std::string, int>::const_iterator it = m_lastLoc.find(script_id);
					if (it != m_lastLoc.end()) {
						if (m_eventGraph->addArcIfNeeded(it->second, op_id))
							++num_arcs_added;
					}
					m_lastLoc[script_id] = op_id;
				}
			}
		}
	}
	printf("Added %d script arcs\n", num_arcs_added);
}

void EventGraphFixer::addEventAfterTargetHappensBefore() {
	std::string node_id;
	int num_arcs_added = 0;
	std::map<std::string, int> m_lastLoc;
	for (int event_action_id = 0; event_action_id <= m_log->maxEventActionId(); ++event_action_id) {
		if (m_eventGraph->isNodeDeleted(event_action_id)) continue;
		const ActionLog::EventAction& op = m_log->event_action(event_action_id);
		for (size_t i = 0; i < op.m_commands.size(); ++i) {
			const ActionLog::Command& cmd = op.m_commands[i];
			if (cmd.m_cmdType == ActionLog::WRITE_MEMORY) {
				if (getTargetNodeString(m_vars->getString(cmd.m_location), &node_id)) {
					m_lastLoc[node_id] = event_action_id;
					m_log->mutable_event_action(event_action_id)->m_commands[i].m_location =
							m_vars->addString(StringPrintf("%s-%d", m_vars->getString(cmd.m_location), event_action_id).c_str());
				}
			} else if (cmd.m_cmdType == ActionLog::READ_MEMORY) {
				if (getTargetNodeString(m_vars->getString(cmd.m_location), &node_id)) {
					std::map<std::string, int>::const_iterator it = m_lastLoc.find(node_id);
					if (it != m_lastLoc.end()) {
						if (m_eventGraph->addArcIfNeeded(it->second, event_action_id))
							++num_arcs_added;
						m_log->mutable_event_action(event_action_id)->m_commands[i].m_location =
								m_vars->addString(StringPrintf("%s-%d", m_vars->getString(cmd.m_location), it->second).c_str());
					}
				}
			}
		}
	}
	printf("Added %d event arcs\n", num_arcs_added);
}

void EventGraphFixer::makeIndependentEventExploration() {
	int num_independent_arcs = 0;
	int last_ui_event = -1, last_auto_event = -1, last_non_auto_event = -1;
	std::vector<int> merging_arcs;
	for (int event_action_id = 0; event_action_id <= m_log->maxEventActionId(); ++event_action_id) {
		if (m_eventGraph->isNodeDeleted(event_action_id)) continue;
		const ActionLog::EventAction& op = m_log->event_action(event_action_id);
		if (op.m_type != ActionLog::USER_INTERFACE) continue;
		if (op.m_commands.size() > 0 &&
				op.m_commands[0].m_cmdType == ActionLog::ENTER_SCOPE &&
				strcmp(m_scopes->getString(op.m_commands[0].m_location), "auto:explore") == 0) {
			if (last_non_auto_event != -1 && last_ui_event == last_auto_event) {
				m_eventGraph->deleteArc(last_auto_event, event_action_id);
				m_eventGraph->addArc(last_non_auto_event, event_action_id);
				merging_arcs.push_back(last_auto_event);
				++num_independent_arcs;
			}
			last_auto_event = event_action_id;
		} else {
			for (size_t i = 0; i < merging_arcs.size(); ++i) {
				m_eventGraph->addArc(merging_arcs[i], event_action_id);
			}
			merging_arcs.clear();
			last_non_auto_event = event_action_id;
		}
		last_ui_event = event_action_id;
	}
	printf("Changed %d arcs to make independent graph exploration.\n", num_independent_arcs);
}


