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

#include "CallTraceBuilder.h"

#include "EventGraph.h"
#include "ActionLog.h"

#include <algorithm>
#include <utility>


CallTraceBuilder::CallTraceBuilder() {
}

void CallTraceBuilder::Init(
		const ActionLog& log,
		const SimpleDirectedGraph& graph) {
	const std::vector<ActionLog::Arc>& arcs = log.arcs();
	std::vector<std::pair<int, int> > sorted_arcs;
	sorted_arcs.reserve(arcs.size());
	for (size_t i = 0; i < arcs.size(); ++i) {
		if (arcs[i].m_duration > 0) {
			sorted_arcs.push_back(std::pair<int, int>(arcs[i].m_tail, arcs[i].m_head));
		}
	}
	std::sort(sorted_arcs.begin(), sorted_arcs.end());
	m_causeEvent.resize(graph.numNodes(), 0);
	for (size_t i = 0; i < m_causeEvent.size(); ++i) {
		m_causeEvent[i] = i;
	}
	for (size_t i = 0; i < sorted_arcs.size(); ++i) {
		m_causeEvent[sorted_arcs[i].second] = m_causeEvent[sorted_arcs[i].first];
	}

	m_nodeTriggerPredecessors.assign(log.maxEventActionId() + 1, std::pair<int, int>(-1, -1));
	m_parentScope.assign(log.maxEventActionId() + 1, std::vector<int>());
	std::vector<int> scope;
	for (int op_id = 0; op_id <= log.maxEventActionId(); ++op_id) {
		const ActionLog::EventAction& op = log.event_action(op_id);
		scope.clear();
		m_parentScope[op_id].assign(op.m_commands.size(), -1);
		for (size_t cmd_id = 0; cmd_id < op.m_commands.size(); ++cmd_id) {
			const ActionLog::Command& cmd = op.m_commands[cmd_id];
			m_parentScope[op_id][cmd_id] = scope.empty() ? -1 : scope[scope.size() - 1];
			if (cmd.m_cmdType == ActionLog::ENTER_SCOPE) {
				scope.push_back(cmd_id);
			} else if (cmd.m_cmdType == ActionLog::EXIT_SCOPE) {
				if (!scope.empty()) { scope.pop_back(); }
			} else if (cmd.m_cmdType == ActionLog::TRIGGER_ARC) {
				if (cmd.m_location >= 0 && cmd.m_location < static_cast<int>(m_nodeTriggerPredecessors.size())) {
					m_nodeTriggerPredecessors[cmd.m_location] = std::pair<int, int>(op_id, cmd_id);
				}
			}
		}
	}
}

int CallTraceBuilder::eventCreatedBy(int event_action_id) const {
	return event_action_id < static_cast<int>(m_causeEvent.size()) ?
			m_causeEvent[event_action_id] : event_action_id;
}

bool CallTraceBuilder::getEventCreationCommand(int node_id, int* previous_node, int* previous_command_id) const {
	if (node_id < 0 || node_id >= static_cast<int>(m_nodeTriggerPredecessors.size())) return false;
	if (m_nodeTriggerPredecessors[node_id].first < 0) return false;
	*previous_node = m_nodeTriggerPredecessors[node_id].first;
	*previous_command_id = m_nodeTriggerPredecessors[node_id].second;
	return true;
}

void CallTraceBuilder::getCallTraceOfCommand(int event_action_id, int command_id, std::vector<int>* scope) const {
	scope->clear();
	if (event_action_id < 0 || event_action_id >= static_cast<int>(m_parentScope.size())) return;
	const std::vector<int>& parents = m_parentScope[event_action_id];
	int cmd = command_id;
	while (cmd >= 0 && cmd < static_cast<int>(parents.size())) {
		//if (cmd > parents[cmd]) printf("Error\n");
		cmd = parents[cmd];
		if (cmd >= 0) scope->push_back(cmd);
	}
	std::reverse(scope->begin(), scope->end());
}
