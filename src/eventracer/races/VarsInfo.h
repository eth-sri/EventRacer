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

#ifndef VARSINFO_H_
#define VARSINFO_H_

#include "base.h"
#include <stddef.h>
#include <map>
#include <set>
#include <vector>

class ActionLog;
class SimpleDirectedGraph;
class EventGraphInterface;

class RaceGraph;

class VarsInfo {
public:
	VarsInfo();
	~VarsInfo();

	void init(const ActionLog& actions);

	void findRaces(const ActionLog& actions, const SimpleDirectedGraph& graph);

	// Calculates the number of variables, for which FastTrack would need to allocate vector clocks.
	int calculateFastTrackNumVCs();

	struct VarAccess {
		VarAccess() {}

		void clearRaces() {
		}

		bool operator<(const VarAccess& o) const {
			return m_eventActionId < o.m_eventActionId;
		}
		bool operator==(const VarAccess& o) const {
			return m_eventActionId == o.m_eventActionId;
		}

		// Whether the access is a read.
		bool m_isRead;
		// The id of the event action where the access occurs.
		int m_eventActionId;
		// The sequential id of the command in the event action.
		int m_commandIdInEvent;

		// Returns a number allowing to order commands in the trace.
		int64 traceOrder() const { return (static_cast<int64>(m_eventActionId) << 32) + m_commandIdInEvent; }
	};

	enum VarAccessType {
		MEMORY_READ,   // Read of a value. No write in the same atomic piece is present.
		MEMORY_WRITE,  // Write without reading the value first. Reads may follow in the same atomic piece, but they do not matter.
		MEMORY_UPDATE  // Read followed by a write.
	};

	struct VarData {
		VarData() : m_numWWRaces(0), m_numWRRaces(0), m_numRWRaces(0) {
		}

		std::vector<VarAccess> m_accesses;

		void clearRaces() {
			m_numRWRaces = m_numWRRaces = m_numWWRaces = 0;
			m_childRaces.clear();
			m_parentRaces.clear();
			m_noParentRaces.clear();
		}

		VarAccessType getVarAccessTypeForId(int access_index) const {
			if (m_accesses[access_index].m_isRead) {
				for (int i = access_index + 1; i < static_cast<int>(m_accesses.size()); ++i) {
					if (m_accesses[i].m_eventActionId != m_accesses[access_index].m_eventActionId) break;
					if (!m_accesses[i].m_isRead) return MEMORY_UPDATE;
				}
				return MEMORY_READ;
			} else{
				for (int i = access_index - 1; i >= 0; --i) {
					if (m_accesses[i].m_eventActionId != m_accesses[access_index].m_eventActionId) break;
					if (m_accesses[i].m_isRead) return MEMORY_UPDATE;
				}
				return MEMORY_WRITE;
			}
		}

		const VarAccess* findAccessLocation(bool is_read, int event_action_id) const {
			for (size_t i = 0; i < m_accesses.size(); ++i) {
				if (m_accesses[i].m_eventActionId == event_action_id && m_accesses[i].m_isRead == is_read) {
					return &m_accesses[i];
				}
			}
			return NULL;
		}

		int numReads() const {
			int result = 0;
			for (size_t i = 0; i < m_accesses.size(); ++i) {
				if (m_accesses[i].m_isRead) ++result;
			}
			return result;
		}

		int numWrites() const {
			int result = 0;
			for (size_t i = 0; i < m_accesses.size(); ++i) {
				if (!m_accesses[i].m_isRead) ++result;
			}
			return result;
		}

		const VarAccess* getWriteWithIndex(int index) const {
			int cnt = 0;
			for (size_t i = 0; i < m_accesses.size(); ++i) {
				if (!m_accesses[i].m_isRead) {
					if (cnt == index) return &m_accesses[i];
					++cnt;
				}
			}
			return NULL;
		}

		const VarAccess* getReadWithIndex(int index) const {
			int cnt = 0;
			for (size_t i = 0; i < m_accesses.size(); ++i) {
				if (m_accesses[i].m_isRead) {
					if (cnt == index) return &m_accesses[i];
					++cnt;
				}
			}
			return NULL;
		}

		int m_numWWRaces;
		int m_numWRRaces;
		int m_numRWRaces;

		// Races of child vars that are covered by races of the current var.
		std::vector<int> m_childRaces;
		// Races that are parents of races in the current var.
		std::vector<int> m_parentRaces;
		// The races of the current var that have no parents.
		std::vector<int> m_noParentRaces;
		// List of all races for a variable.
		std::vector<int> m_allRaces;
	};
	typedef std::map<int, VarData> AllVarData;

	const AllVarData& variables() const { return m_vars; }


	// Returns the command id for a read or a write of a variable in an event action.
	// If there is no such command id, returns -1.
	static int getCommandIdForVarReadInEventAction(const VarData& var, int event_action_id);
	static int getCommandIdForVarWriteInEventAction(const VarData& var, int event_action_id);

	// Returns the access type for a variable in an event action.
	//   If there is only a read, returns MEMORY_READ
	//   If there is a read followed by a write, returns MEMORY_UPDATE
	//   If there is a write (not prceded by a read), returns MEMORY_WRITE
	//
	// Note: there MUST be a read or a write at the event action in order for the
	// result to be correct.
	static VarAccessType getVarAccessTypeInEventAction(const VarData& var, int event_action_id);

	struct RaceInfo {
		static const char* AccessStr(VarAccessType access);

		const char* TypeStr() const;
		const char* TypeShortStr() const;

		RaceInfo(VarAccessType a1, VarAccessType a2, int e1, int e2, int command_in_e1, int command_in_e2, int v)
			: m_access1(a1), m_access2(a2),
			  m_event1(e1), m_event2(e2),
			  m_cmdInEvent1(command_in_e1), m_cmdInEvent2(command_in_e2),
			  m_varId(v), m_coveredBy(-1) {
		}

		bool canSynchronizeInThisOrder() const {
			return true;  // For experiments, we assume we can always synchronize.
			//return (m_access2 != VarsInfo::MEMORY_WRITE && m_access1 != VarsInfo::MEMORY_READ);
		}

		VarAccessType m_access1;
		VarAccessType m_access2;
		int m_event1;
		int m_event2;
		int m_cmdInEvent1;
		int m_cmdInEvent2;

		int m_varId;

		int m_coveredBy;
		std::vector<int> m_childRaces;

		// If a race is covered only by more than one other race, these
		// show up here.
		std::vector<int> m_multiParentRaces;
	};

	typedef std::vector<RaceInfo> AllRaces;

	const AllRaces& races() const { return m_races; }

	// Appends the set of direct child races of a race to the given set.
	void getDirectRaceChildren(int race_id, bool only_different_event_actions, std::set<int>* direct_child_races) const;

	bool timedOut() const {
		return m_timedOut;
	}

	int timeToFindRacesMs() const {
		return m_timeToFindRacesMs;
	}

	int timeInitMs() const {
		return m_initTime;
	}

	int numChains() const {
		return m_numChains;
	}

	int numNodes() const {
		return m_numNodes;
	}

	int numArcs() const {
		return m_numArcs;
	}

	const EventGraphInterface* fast_event_graph() const {
		return m_fastEventGraph;
	}

	bool hasPathViaRaces(int node1, int node2, int cmd_in_node2,
			std::vector<int>* race_path) const;

private:
	// Returns true if a computation timed out and sets the m_timedOut variable to true.
	bool shouldTimeout();

	void sortRaces();

	void findRaceDependency(const ActionLog& actions);

	// Races must be sorted before calling this.
	void findMultiRaceDependency(const ActionLog& actions);

	int64 m_startTime;
	bool m_timedOut;
	int m_timeToFindRacesMs;
	int m_initTime;
	int m_numChains;
	int m_numNodes;
	int m_numArcs;

	AllVarData m_vars;
	AllRaces m_races;

	EventGraphInterface* m_fastEventGraph;
	RaceGraph* m_raceGraph;
};

#endif /* VARSINFO_H_ */
