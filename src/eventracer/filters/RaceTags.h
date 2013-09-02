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

#ifndef RACETAGS_H_
#define RACETAGS_H_

#include "base.h"

#include "VarsInfo.h"
#include <string>

class ActionLog;
class CallTraceBuilder;
class StringSet;

class RaceTags {
public:
	RaceTags(const VarsInfo& races,
			 const ActionLog& log,
			 const StringSet& vars,
			 const StringSet& scopes,
			 const StringSet& mem_values,
			 const CallTraceBuilder& event_cause);
	~RaceTags();

	void Init();

	enum RaceTag {
		WRITE_SAME_VALUE = 0,
		ONLY_LOCAL_WRITE,
		NO_EVENT_ATTACHED,
		LATE_EVENT_ATTACH,
		MAYBE_LAZY_INIT,
		RACE_WITH_UNLOAD,
		COOKIE_OR_CLASSNAME,
		NUM_RACE_TAGS
	};

	typedef int64 RaceTagSet;
	static RaceTagSet emptyTagSet();
	static RaceTagSet addTag(RaceTagSet tags, RaceTag tag);
	static RaceTagSet removeTag(RaceTagSet tags, RaceTag tag);
	static RaceTagSet mergeTags(RaceTagSet tags1, RaceTagSet tags2);
	static bool hasTag(RaceTagSet tags, RaceTag tag);
	static const char* tagName(RaceTag tag);
	static std::string tagsToString(RaceTagSet tags);

	RaceTagSet getVariableTags(int var_id) const;

	// Returns whether a variable may have an initialization race.
	bool hasUndefinedInitilizationRace(int var_id) const;
	bool hasNetworkResponseRace(int var_id, bool ww_race) const;

	struct VarSummary {
		RaceTagSet tags;
		bool has_undefined_init_race;
	};

	VarSummary getVarSummary(int var_id) const {
		VarSummary summary;
		summary.tags = getVariableTags(var_id);
		summary.has_undefined_init_race = hasUndefinedInitilizationRace(var_id);
		return summary;
	}

	bool isNetworkResponseRace(int race_id) const;

	// Returns the definition set of a variable in the racing variables.
	std::string getVarDefSet(int var_id) const;

private:
	RaceTags::RaceTagSet getEventRaceClasses(const VarsInfo::VarData& var) const;
	bool hasOnlySameValueWrites(const VarsInfo::VarData& var) const;
	bool isOnlyLocalWrites(const VarsInfo::VarData& var) const;
	bool hasReadInOpAfter(const VarsInfo::VarData& var, int op_id) const;

	// Number of consecutive read operations before an event fire scope is encountered.
	// If other operations except a read is reached, returns -1.
	int numReadCmdsUntilEventFire(int op_id, int cmd_id) const;

	bool isLazyInit(const VarsInfo::VarData& var) const;
	bool hasOnlyUnloadRaces(const VarsInfo::VarData& var) const;
	bool isUnloadOp(int op_id) const;
	bool isNetworkResponseOp(int op_id) const;
	bool isCounterVar(const VarsInfo::VarData& var) const;
	bool isCounterIncrementOrDecrementUpdate(const VarsInfo::VarData& var, int op_id) const;
	// Returns if the read operation at op_id and cmd_id is for sure a value type read (integer or boolean) or a NULL/undefined read.
	bool isValueTypeReadOrNull(int op_id, int cmd_id) const;
	bool isCookie(int var_id) const;
	bool isClassName(int var_id) const;

	// Returns the value of the value written or read in operation op_id, command cmd_id.
	// Returns NULL if no such value was written (Note: the value is returned always a string
	// and it may be the string NULL or undefined if this was the value read or written).
	const char* getValueOfReadOrWrite(int op_id, int cmd_id) const;

	double getExceptionCorruptionRiskRank(const VarsInfo::VarData& var) const;
	// Number of variable write commands in an operation before the given command id.
	int numWritesBeforeCommand(int op_id, int cmd_id) const;
	bool varIsUserVisible(int var_id) const;

	bool varHasNonObviousRaces(const VarsInfo::VarData& var) const;

	const VarsInfo& m_races;
	const ActionLog& m_log;
	const StringSet& m_vars;
	const StringSet& m_scopes;
	const StringSet& m_memValues;
	const CallTraceBuilder& m_eventCause;
	bool m_initialized;
};

#endif /* RACETAGS_H_ */
