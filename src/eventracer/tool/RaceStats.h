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

#ifndef RACESTATS_H_
#define RACESTATS_H_

#include <string>
#include "CallTraceBuilder.h"
#include "EventGraph.h"
#include "EventGraphInfo.h"
#include "StringSet.h"
#include "ActionLog.h"
#include "VarsInfo.h"
#include "RaceTags.h"

class RaceFile {
public:
	RaceFile();

	bool Load(const std::string& filename, bool eval_race_detector_time);

	void printRaceEventStats(bool only_uncovered);

	void printSimpleStats();

	void printVarStatsHeader();
	void printVarStats();

	void printTimeStats();

	void printHighRiskRaces();

	int numRaces() const;

	void setFileId(const std::string& file_id) { m_fileId = file_id; }
	const std::string& fileId() const { return m_fileId; }

	void evaluateAccordionClocks();

private:
	const char* getOpName(int var_id, int op_id) const;

	std::string m_filename;
	ActionLog m_actions;
	StringSet m_vars;
	StringSet m_scopes;
	StringSet m_js;
	StringSet m_memValues;
	std::string m_fileId;
	int64 m_fileSize;

	int m_timeToFindRacesMs;
	int m_timeToInitRaceFinderMs;

	VarsInfo m_vinfo;
	EventGraphInfo m_graphInfo;
	SimpleDirectedGraph m_inputEventGraph;
	SimpleDirectedGraph m_graphWithTimers;
	CallTraceBuilder m_eventCauseFinder;
	RaceTags m_tags;
};

#endif /* RACESTATS_H_ */
