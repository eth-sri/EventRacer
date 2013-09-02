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

#ifndef CallTraceBuilder_H_
#define CallTraceBuilder_H_

#include <vector>

class ActionLog;
class SimpleDirectedGraph;

// For every event, keeps the event action that forked the event.
class CallTraceBuilder {
public:
	CallTraceBuilder();

	void Init(
			const ActionLog& log,
			const SimpleDirectedGraph& graph);

	// Returns the event action id that created a given event action.
	int eventCreatedBy(int event_action_id) const;

	// Return true if the given event action was created by another event action and then sets the previous_event and
	// previous_command_id to the command, which triggered the event action.
	bool getEventCreationCommand(int event_action_id, int* previous_event, int* previous_command_id) const;

	// Returns the call trace of a given command (in a given event action)
	void getCallTraceOfCommand(int event_action_id, int command_id, std::vector<int>* call_trace) const;

private:
	std::vector<int> m_causeEvent;
	std::vector<std::pair<int, int> > m_nodeTriggerPredecessors;
	std::vector<std::vector<int> > m_parentScope;
};


#endif /* CallTraceBuilder_H_ */
