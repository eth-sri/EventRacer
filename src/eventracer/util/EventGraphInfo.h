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

#ifndef EVENTGRAPHINFO_H_
#define EVENTGRAPHINFO_H_

#include "ActionLog.h"

#include <map>
#include <set>
#include <utility>

class EventGraphInfo {
public:
	void init(const ActionLog& actions);

	// Gets the duration of an arc. -1 if unknown.
	int getArcDuration(int source, int target) const;

	bool isNodeDropped(int node_id) const { return m_droppedNodes.count(node_id); }

	void dropNode(int node_id) { m_droppedNodes.insert(node_id); }
private:
	std::map<std::pair<int, int>, int> m_arcDuration;
	std::set<int> m_droppedNodes;
};

#endif /* EVENTGRAPHINFO_H_ */
