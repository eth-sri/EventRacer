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

#include "EventGraphInfo.h"

void EventGraphInfo::init(const ActionLog& actions) {
	for (size_t i = 0; i < actions.arcs().size(); ++i) {
		const ActionLog::Arc& arc = actions.arcs()[i];
		m_arcDuration[std::make_pair(arc.m_tail, arc.m_head)] = arc.m_duration;
	}
}

int EventGraphInfo::getArcDuration(int source, int target) const {
	const std::map<std::pair<int, int>, int>::const_iterator it =
			m_arcDuration.find(std::make_pair(source, target));
	if (it == m_arcDuration.end()) return -1;
	return it->second;
}




