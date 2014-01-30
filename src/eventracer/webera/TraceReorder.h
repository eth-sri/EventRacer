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

#ifndef TRACEREORDER_H_
#define TRACEREORDER_H_

#include <string>
#include <vector>

#include "EventGraph.h"

class VarsInfo;

class TraceReorder {
public:
	TraceReorder();
	virtual ~TraceReorder();

	void LoadSchedule(const char* filename);

	struct Reverse {
		// A reverse must put node2 before node1.
		int node1;
		int node2;
	};

	struct Preserve {
		// A preserve must keep node1 before node2.
		int node1;
		int node2;
	};

	struct Options {
		Options()
		  : include_change_marker(false),
		    relax_replay_after_all_races(false),
		    minimize_variation_from_original(true) {
		}

		// Whether to include a <change> marker denoting that non-determinism is to be expected after it.
		bool include_change_marker;

		// Whether the replay will contain a <relax> tag that will not strictly enforce the same scheduling
		// after all races are reversed.
		bool relax_replay_after_all_races;

		// Whether to try to minimize the variation from the original trace.
		bool minimize_variation_from_original;
	};

	bool GetScheduleFromRaces(
			const VarsInfo& vinfo,
			const std::vector<int>& rev_races,
			const SimpleDirectedGraph& graph,
			const Options& options,
			std::vector<int>* schedule) const;

	bool GetSchedule(
			const std::vector<Reverse>& reverses,
			const std::vector<Preserve>& preserves,
			const SimpleDirectedGraph& graph,
			const Options& options,
			std::vector<int>* schedule) const;

	void SaveSchedule(const char* filename, const std::vector<int>& schedule) const;

private:
	std::vector<std::string> m_actions;
};

#endif /* TRACEREORDER_H_ */
