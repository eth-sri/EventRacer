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

	bool GetSchedule(
			const std::vector<Reverse>& reverses,
			const SimpleDirectedGraph& graph,
			std::vector<int>* schedule);

	void SaveSchedule(const char* filename, const std::vector<int>& schedule) const;

private:
	std::vector<std::string> m_actions;
};

#endif /* TRACEREORDER_H_ */
