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

#ifndef TIMERGRAPH_H_
#define TIMERGRAPH_H_

#include <vector>

#include "ActionLog.h"
#include "EventGraph.h"

class TimerGraph {
public:
	explicit TimerGraph(const std::vector<ActionLog::Arc>& arcs, const SimpleDirectedGraph& graph);

	void build(SimpleDirectedGraph* graph);

private:
	std::vector<ActionLog::Arc> m_timedArcs;
};

#endif /* TIMERGRAPH_H_ */
