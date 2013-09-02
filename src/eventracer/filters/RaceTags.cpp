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


#include "RaceTags.h"

#include "ActionLog.h"
#include "EventGraph.h"
#include "CallTraceBuilder.h"
#include "StringSet.h"
#include "strutil.h"

#include <string.h>
#include <stdlib.h>

RaceTags::RaceTags(
		const VarsInfo& races,
		const ActionLog& log,
		const StringSet& vars,
		const StringSet& scopes,
		const StringSet& mem_values,
		const CallTraceBuilder& event_cause)
	: m_races(races),
	  m_log(log),
	  m_vars(vars),
	  m_scopes(scopes),
	  m_memValues(mem_values),
	  m_eventCause(event_cause),
	  m_initialized(false) {
}

RaceTags::~RaceTags() {
}


RaceTags::RaceTagSet RaceTags::emptyTagSet() {
	return 0;
}

RaceTags::RaceTagSet RaceTags::addTag(RaceTagSet tags, RaceTag tag) {
	return tags | (1ll << tag);
}

RaceTags::RaceTagSet RaceTags::removeTag(RaceTagSet tags, RaceTag tag) {
	return tags & (~(1ll << tag));
}

RaceTags::RaceTagSet RaceTags::mergeTags(RaceTagSet tags1, RaceTagSet tags2) {
	return tags1 | tags2;
}

bool RaceTags::hasTag(RaceTagSet tags, RaceTag tag) {
	return (tags & (1ll << tag)) != 0;
}

const char* RaceTags::tagName(RaceTag tag) {
	switch (tag) {
	case WRITE_SAME_VALUE: return "WRITE_SAME_VALUE";
	case ONLY_LOCAL_WRITE: return "ONLY_LOCAL_WRITE";
	case NO_EVENT_ATTACHED: return "NO_EVENT_ATTACHED";
	case LATE_EVENT_ATTACH: return "LATE_EVENT_ATTACH";
	case MAYBE_LAZY_INIT: return "MAYBE_LAZY_INIT";
	case RACE_WITH_UNLOAD: return "RACE_WITH_UNLOAD";
	case COOKIE_OR_CLASSNAME: return "COOKIE_OR_CLASSNAME";
	default: return "Unknown tag";
	}
}

std::string RaceTags::tagsToString(RaceTagSet tags) {
	std::string r;
	for (int t = 0; t < static_cast<int>(NUM_RACE_TAGS); ++t) {
		if (hasTag(tags, static_cast<RaceTag>(t))) {
			if (!r.empty()) r += " ";
			r += tagName(static_cast<RaceTag>(t));
		}
	}
	return r;
}

RaceTags::RaceTagSet RaceTags::getVariableTags(int var_id) const {
	RaceTags::RaceTagSet result = emptyTagSet();
	VarsInfo::AllVarData::const_iterator it = m_races.variables().find(var_id);
	if (it == m_races.variables().end()) return result;
	const VarsInfo::VarData& var = it->second;
	if (isOnlyLocalWrites(var) && !varIsUserVisible(var_id)) {
		result = addTag(result, ONLY_LOCAL_WRITE);
	}
	if (hasOnlySameValueWrites(var)) {
		result = addTag(result, WRITE_SAME_VALUE);
	}
	result = mergeTags(result, getEventRaceClasses(var));
	if (isLazyInit(var)) {
		result = addTag(result, MAYBE_LAZY_INIT);
	}
	if (hasOnlyUnloadRaces(var)) {
		result = addTag(result, RACE_WITH_UNLOAD);
	}
/*	if (isCounterVar(var)) {
		result = addTag(result, COUNTER);
	}*/
	if (isCookie(var_id)) {
		result = addTag(result, COOKIE_OR_CLASSNAME);
	}
	if (isClassName(var_id)) {
		result = addTag(result, COOKIE_OR_CLASSNAME);
	}
	return result;
}

bool RaceTags::hasUndefinedInitilizationRace(int var_id) const {
	VarsInfo::AllVarData::const_iterator it = m_races.variables().find(var_id);
	if (it == m_races.variables().end()) return false;
	const VarsInfo::VarData& var = it->second;
	if (var.numWrites() > 0) {
		int write_op = var.getWriteWithIndex(0)->m_eventActionId;
		for (size_t i = 0; i < var.m_accesses.size(); ++i) {
			if (!var.m_accesses[i].m_isRead) continue;
			int read_op = var.m_accesses[i].m_eventActionId;
			if (read_op < write_op) return false;
			std::vector<int> tmp;
			if (!isValueTypeReadOrNull(read_op, var.m_accesses[i].m_commandIdInEvent) &&
				!m_races.hasPathViaRaces(write_op, read_op, var.m_accesses[i].m_commandIdInEvent, &tmp)) {
				bool result = true;
				for (size_t j = 0; j < var.m_accesses.size(); ++j) {
					if (var.m_accesses[j].m_isRead) continue;
					if (m_races.hasPathViaRaces(var.m_accesses[j].m_eventActionId,
							read_op, var.m_accesses[i].m_commandIdInEvent, &tmp)) {
						result = false;
						break;
					}
				}
				if (result) return true;
			}
		}
	}
	return false;
}

bool RaceTags::hasNetworkResponseRace(int var_id, bool ww_race) const {
	VarsInfo::AllVarData::const_iterator it = m_races.variables().find(var_id);
	if (it == m_races.variables().end()) return false;
	const VarsInfo::VarData& var = it->second;
	for (size_t i = 0; i < var.m_noParentRaces.size(); ++i) {
		if (isNetworkResponseRace(var.m_noParentRaces[i])) {
			const VarsInfo::RaceInfo& race = m_races.races()[var.m_noParentRaces[i]];
			if (ww_race == false) {
				return true;
			} else if (race.m_access1 == VarsInfo::MEMORY_WRITE &&
					race.m_access2 == VarsInfo::MEMORY_WRITE) {
				const char* v1 = getValueOfReadOrWrite(race.m_event1, race.m_cmdInEvent1);
				const char* v2 = getValueOfReadOrWrite(race.m_event2, race.m_cmdInEvent2);
				return v1 == NULL || v1 != v2;
			}
		}
	}
	return false;
}

bool RaceTags::isNetworkResponseRace(int race_id) const {
	if (race_id < 0 || race_id >= static_cast<int>(m_races.races().size())) return false;
	const VarsInfo::RaceInfo& race = m_races.races()[race_id];
	return isNetworkResponseOp(race.m_event1) || isNetworkResponseOp(race.m_event2);
}

std::string RaceTags::getVarDefSet(int var_id) const {
	VarsInfo::AllVarData::const_iterator it = m_races.variables().find(var_id);
	if (it == m_races.variables().end()) return "";
	const VarsInfo::VarData& var = it->second;

	std::set<std::string> def_set;
	for (size_t i = 0; i < var.m_accesses.size(); ++i) {
		const char* v = getValueOfReadOrWrite(var.m_accesses[i].m_eventActionId, var.m_accesses[i].m_commandIdInEvent);
		if (v != NULL) def_set.insert(v);
	}

	std::string result;
	for (std::set<std::string>::const_iterator it = def_set.begin(); it != def_set.end(); ++it) {
		if (!result.empty()) result += " ";
		result += *it;
	}
	return result;
}

RaceTags::RaceTagSet RaceTags::getEventRaceClasses(const VarsInfo::VarData& var) const {
	bool actual_race = false;
	std::set<int> race_reads;
	for (size_t i = 0; i < var.m_accesses.size(); ++i) {
		if (!var.m_accesses[i].m_isRead) continue;
		int num_reads_till_file = numReadCmdsUntilEventFire(var.m_accesses[i].m_eventActionId, var.m_accesses[i].m_commandIdInEvent);
		if (num_reads_till_file == -1) continue;
		if (num_reads_till_file == 0) actual_race = true;
		race_reads.insert(var.m_accesses[i].m_eventActionId);
	}
	if (race_reads.empty()) return emptyTagSet();  // Not an event variable.
	for (size_t i = 0; i < var.m_allRaces.size(); ++i) {
		const VarsInfo::RaceInfo& race = m_races.races()[var.m_allRaces[i]];
		// Unload races should not matter for events.
		if (isUnloadOp(race.m_event1) || isUnloadOp(race.m_event2)) continue;
		// Only if a race does not involve event invocation, do not return an event race tag.
		if (race_reads.count(race.m_event1) == 0 && race_reads.count(race.m_event2) == 0) return emptyTagSet();
	}
	return addTag(emptyTagSet(), actual_race ? RaceTags::LATE_EVENT_ATTACH : RaceTags::NO_EVENT_ATTACHED);
}

bool RaceTags::hasOnlySameValueWrites(const VarsInfo::VarData& var) const {
	int write_value = -1;
	for (size_t i = 0; i < var.m_accesses.size(); ++i) {
		if (var.m_accesses[i].m_isRead) continue;
		const ActionLog::EventAction& op = m_log.event_action(var.m_accesses[i].m_eventActionId);
		int cmd_id = var.m_accesses[i].m_commandIdInEvent;
		if (cmd_id + 1 < static_cast<int>(op.m_commands.size()) &&
			op.m_commands[cmd_id + 1].m_cmdType == ActionLog::MEMORY_VALUE) {
			int new_value = op.m_commands[cmd_id + 1].m_location;
			if (write_value == -1) write_value = new_value;
			if (write_value == new_value) continue;
		}
		return false;
	}
	// TODO(veselin): Fix this to be more accurate.
	for (size_t i = 0; i < var.m_allRaces.size(); ++i) {
		const VarsInfo::RaceInfo& race = m_races.races()[var.m_allRaces[i]];
		if (race.m_access1 != VarsInfo::MEMORY_WRITE ||
			race.m_access2 != VarsInfo::MEMORY_WRITE)
			return false;
	}
	return true;
}

bool RaceTags::isOnlyLocalWrites(const VarsInfo::VarData& var) const {
	for (size_t i = 0; i < var.m_noParentRaces.size(); ++i) {
		const VarsInfo::RaceInfo& race = m_races.races()[var.m_noParentRaces[i]];
		if (race.m_access1 != VarsInfo::MEMORY_WRITE ||
			race.m_access2 != VarsInfo::MEMORY_WRITE ||
			hasReadInOpAfter(var, race.m_event1) ||
			hasReadInOpAfter(var, race.m_event2)) {
			return false;
		}
	}
	return true;
}

bool RaceTags::hasReadInOpAfter(const VarsInfo::VarData& var, int op_id) const {
	for (size_t i = 0; i < var.m_accesses.size(); ++i) {
		if (var.m_accesses[i].m_eventActionId > op_id) {
			return var.m_accesses[i].m_isRead;
		}
	}
	return false;
}

int RaceTags::numReadCmdsUntilEventFire(int op_id, int cmd_id) const {
	const ActionLog::EventAction& op = m_log.event_action(op_id);
	int num_reads = 0;
	for (size_t i = cmd_id + 1; i < op.m_commands.size(); ++i) {
		const ActionLog::Command& cmd = op.m_commands[i];
		if (cmd.m_cmdType == ActionLog::ENTER_SCOPE &&
				strncmp(m_scopes.getString(cmd.m_location), "fire:", 5) == 0) {
			return num_reads;
		} else if (cmd.m_cmdType == ActionLog::READ_MEMORY) {
			++num_reads;
		} else {
			break;
		}
	}
	return -1;
}

bool RaceTags::isLazyInit(const VarsInfo::VarData& var) const {
	if (var.numWrites() == 1 && var.numReads() > 0) {
		if (var.getWriteWithIndex(0)->m_eventActionId == var.getReadWithIndex(0)->m_eventActionId &&
			var.getWriteWithIndex(0)->m_commandIdInEvent > var.getReadWithIndex(0)->m_commandIdInEvent) {
/*			// TODO: Check that it starts from NULL or undefined.
            const ActionLog::Operation& op = m_log.operation(var.m_reads[0].m_operationId);
			int cmd_id = var.m_reads[0].m_commandIdInOp;
			if (cmd_id + 1 < op.m_commands.size() &&
					op.m_commands[cmd_id + 1].m_cmdType == ActionLog::MEMORY_VALUE &&
					op.m_commands[cmd_id + 1].m_location) {

			}
*/
			return true;
		}
	}
	return false;
}

bool RaceTags::hasOnlyUnloadRaces(const VarsInfo::VarData& var) const {
	if (var.m_noParentRaces.size() == 0) return false;
	for (size_t i = 0; i < var.m_noParentRaces.size(); ++i) {
		const VarsInfo::RaceInfo& race = m_races.races()[var.m_noParentRaces[i]];
		if (isUnloadOp(race.m_event1) == false && isUnloadOp(race.m_event2) == false) return false;
	}
	return true;
}

bool RaceTags::isUnloadOp(int op_id) const {
	const ActionLog::EventAction& op = m_log.event_action(op_id);
	for (size_t i = 0; i < op.m_commands.size(); ++i) {
		const ActionLog::Command& cmd = op.m_commands[i];
		if (cmd.m_cmdType == ActionLog::ENTER_SCOPE) {
			const char* location = m_scopes.getString(cmd.m_location);
			return strncmp(location, "fire:unload", 10) == 0 ||
					strcmp(location, "delete_document") == 0;
		}
	}
	return false;
}

bool RaceTags::isNetworkResponseOp(int op_id) const {
	const ActionLog::EventAction& op = m_log.event_action(op_id);
	for (size_t i = 0; i < op.m_commands.size() && i < 32; ++i) {
		const ActionLog::Command& cmd = op.m_commands[i];
		if (cmd.m_cmdType == ActionLog::ENTER_SCOPE) {
			const char* location = m_scopes.getString(cmd.m_location);
			return strncmp(location, "fire:readystatechange", 21) == 0;
		}
	}
	return false;
}

bool RaceTags::isCounterVar(const VarsInfo::VarData& var) const {
	if (var.m_allRaces.size() == 0) return false;
	if (var.numWrites() == 0) return false;
	for (size_t i = 0; i < var.m_allRaces.size(); ++i) {
		const VarsInfo::RaceInfo& race = m_races.races()[var.m_allRaces[i]];
		if (race.m_access1 == VarsInfo::MEMORY_WRITE ||
			race.m_access2 == VarsInfo::MEMORY_WRITE) return false;
		if (race.m_access1 == VarsInfo::MEMORY_UPDATE &&
				!isCounterIncrementOrDecrementUpdate(var, race.m_event1)) return false;
		if (race.m_access2 == VarsInfo::MEMORY_UPDATE &&
				!isCounterIncrementOrDecrementUpdate(var, race.m_event2)) return false;
	}

	// Make sure it decrements to 0 in the end, since we have seen counters that only
	// increase.
	const VarsInfo::VarAccess* last_write = var.getWriteWithIndex(var.numWrites() - 1);
	const char* last_value = getValueOfReadOrWrite(
			last_write->m_eventActionId, last_write->m_commandIdInEvent);
	return last_value != NULL && strcmp(last_value, "0") == 0;
}

bool RaceTags::isCounterIncrementOrDecrementUpdate(const VarsInfo::VarData& var, int op_id) const {
	int read_cmd = VarsInfo::getCommandIdForVarReadInEventAction(var, op_id);
	int write_cmd = VarsInfo::getCommandIdForVarWriteInEventAction(var, op_id);
	if (read_cmd == -1 || write_cmd == -1) return false;
	if (read_cmd > write_cmd) return false;
	// Parse the read value.
	const char* read_value = getValueOfReadOrWrite(op_id, read_cmd);
	if (read_value == NULL) return false;
	int read_int;
	if (!ParseInt32(read_value, &read_int)) return false;
	// Parse the written value.
	const char* write_value = getValueOfReadOrWrite(op_id, write_cmd);
	if (write_value == NULL) return false;
	int write_int;
	if (!ParseInt32(write_value, &write_int)) return false;
	return abs(read_int - write_int) == 1;
}

bool RaceTags::isValueTypeReadOrNull(int op_id, int cmd_id) const {
	const char* read_value = getValueOfReadOrWrite(op_id, cmd_id);
	if (read_value == NULL) return false;
	int read_int;
	if (ParseInt32(read_value, &read_int)) return true;
	return (strcmp(read_value, "undefined") == 0 ||
			strcmp(read_value, "NULL") == 0 ||
			strcmp(read_value, "true") == 0 ||
			strcmp(read_value, "false") == 0);
}

bool RaceTags::isCookie(int var_id) const {
	const char* name = m_vars.getString(var_id);
	if (name == NULL) return false;
	return strstr(name, ".cookie") != NULL;
}

bool RaceTags::isClassName(int var_id) const {
	const char* name = m_vars.getString(var_id);
	if (name == NULL) return false;
	return strstr(name, ".className") != NULL;
}

const char* RaceTags::getValueOfReadOrWrite(int op_id, int cmd_id) const {
	const ActionLog::EventAction& op = m_log.event_action(op_id);
	if (cmd_id + 1 >= static_cast<int>(op.m_commands.size())) return NULL;
	const ActionLog::Command& cmd = op.m_commands[cmd_id + 1];
	if (cmd.m_cmdType != ActionLog::MEMORY_VALUE) return NULL;
	return m_memValues.getString(cmd.m_location);
}

double RaceTags::getExceptionCorruptionRiskRank(const VarsInfo::VarData& var) const {
	double result = 0.0;
	for (size_t i = 0; i < var.m_allRaces.size(); ++i) {
		const VarsInfo::RaceInfo& race = m_races.races()[var.m_allRaces[i]];
		if (race.m_coveredBy != -1) continue;
		if (race.m_access1 != VarsInfo::MEMORY_WRITE) {
			result += numWritesBeforeCommand(race.m_event1, race.m_cmdInEvent1);
		}
		if (race.m_access2 != VarsInfo::MEMORY_WRITE) {
			result += numWritesBeforeCommand(race.m_event2, race.m_cmdInEvent2);
		}
	}
	return result;
}

int RaceTags::numWritesBeforeCommand(int op_id, int cmd_id) const {
	const ActionLog::EventAction& op = m_log.event_action(op_id);
	int cmd_count = cmd_id;
	if (cmd_count > static_cast<int>(op.m_commands.size())) { cmd_count = op.m_commands.size(); }
	int write_count = 0;
	for (int i = 0; i < cmd_count; ++i) {
		if (op.m_commands[i].m_cmdType == ActionLog::WRITE_MEMORY) ++write_count;
	}
	return write_count;
}

bool RaceTags::varIsUserVisible(int var_id) const {
	std::string name = m_vars.getString(var_id);
	std::string::size_type p1 = name.find("[0x");
	std::string::size_type p2 = name.find("].");
	if (p1 == std::string::npos || p2 == std::string::npos) return false;
	const std::string class_name(name.begin(), name.begin() + p1);
	const std::string field_name(name.begin() + p2 + 2, name.end());
	if (class_name == "DOMNode") {
		return field_name == "innerHTML" ||
				field_name == "id" ||
				field_name == "className" ||
				field_name == "style" ||
				field_name == "dir" ||
				field_name == "accesskey" ||
				// img
				field_name == "src" ||
				field_name == "alt" ||
				field_name == "ismap" ||
				field_name == "usemap" ||
				// a
				field_name == "href" ||
				// iframe
				field_name == "seamless" ||
				field_name == "srcdoc" ||
				field_name == "width" ||
				field_name == "height" ||
				field_name == "sandbox" ||
				// textarea
				field_name == "readonly" ||
				field_name == "disabled" ||
				// button
				field_name == "type" ||
				field_name == "value";
	}
	return false;
}

bool RaceTags::varHasNonObviousRaces(const VarsInfo::VarData& var) const {
	for (size_t i = 0; i < var.m_allRaces.size(); ++i) {
		const VarsInfo::RaceInfo& race = m_races.races()[var.m_allRaces[i]];
		if (m_eventCause.eventCreatedBy(race.m_event1) == race.m_event1 &&
			m_eventCause.eventCreatedBy(race.m_event2) == race.m_event2) {
			return true;
		}
	}
	return false;
}
