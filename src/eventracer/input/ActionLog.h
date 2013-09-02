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

#ifndef ACTIONLOG_H_
#define ACTIONLOG_H_

#include <stdio.h>
#include <map>
#include <set>
#include <vector>

class ActionLog {
public:
	ActionLog();
	~ActionLog();

	enum CommandType {
		ENTER_SCOPE = 0,
		EXIT_SCOPE,
		READ_MEMORY,
		WRITE_MEMORY,
		TRIGGER_ARC,
		MEMORY_VALUE
	};
	static const char* CommandType_AsString(CommandType ctype);

	enum EventActionType {
		UNKNOWN = 0,
		TIMER,
		USER_INTERFACE,
		NETWORK,
		CONTINUATION
	};
	static const char* EventActionType_AsString(EventActionType otype);

	// Adds an arcs. Doesn't check for duplicates or validity.
	void addArc(int earlier_event_action_id, int later_event_action_id, int arcDuration);

	// Enters an operator. An operation should be entered only once and ideally it should be exited.
	void startEventAction(int operation);

	// Exits the currently opened operation. Returns false if not in an operation.
	bool endEventAction();

	// Sets the type of the current operation. Returns false if not in an operation.
	bool setEventActionType(EventActionType op_type);

	// Enters a scope in the current operation. Returns false if not in an operation.
	bool enterScope(int scopeId) {
		return logCommand(ENTER_SCOPE, scopeId);
	}
	// Exits the last entered scope.
	bool exitScope() {
		return logCommand(EXIT_SCOPE, -1);
	}

	// Returns whether logs of a certain command type will be written to the log.
	// For example, MEMORY_VALUE can only be written after a read or a write.
	bool willLogCommand(CommandType command);

	// Logs a command. Returns false if not in an operation.
	bool logCommand(CommandType command, int memoryLocation);

	// Saves the log to a file.
	void saveToFile(FILE* f);

	// Loads from log from a file.
	bool loadFromFile(FILE* f);

	struct Command {
		CommandType m_cmdType;
		// Memory location for reads/writes and scope id for scopes. Should be -1 if the location is unused.
		int m_location;

		bool operator==(const Command& o) const { return m_cmdType == o.m_cmdType && m_location == o.m_location; }
		bool operator<(const Command& o) const {
			return (m_cmdType == o.m_cmdType) ? (m_location < o.m_location) : (m_cmdType < o.m_cmdType);
		}
	};

	struct Arc {
		int m_tail;
		int m_head;
		// The duration of the arc, -1 if the duration is unknown.
		int m_duration;
	};

	struct EventAction {
		EventAction() : m_type(UNKNOWN) {}

		EventActionType m_type;
		std::vector<Command> m_commands;
	};

	const std::vector<Arc>& arcs() const { return m_arcs; }
	const EventAction& event_action(int i) const {
		EventActionSet::const_iterator it = m_eventActions.find(i);
		if (it == m_eventActions.end()) {
			return m_emptyEventAction;
		}
		return *(it->second);
	}
	EventAction* mutable_event_action(int i) {
		return m_eventActions[i];
	}
	int maxEventActionId() const { return m_maxEventActionId; }

private:
	typedef std::map<int, EventAction*> EventActionSet;
	EventAction m_emptyEventAction;
	EventActionSet m_eventActions;
	int m_maxEventActionId;
	std::vector<Arc> m_arcs;

	// Fields to help construction.
	int m_currentEventActionId;
	std::set<Command> m_cmdsInCurrentEvent;
};

#endif /* ACTIONLOG_H_ */
