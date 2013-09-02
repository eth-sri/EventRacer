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

#ifndef GRAPHFIX_H_
#define GRAPHFIX_H_

class ActionLog;
class SimpleDirectedGraph;
class EventGraphInfo;
class StringSet;

// Performs a number of modifications on the EventGraph.
// For example, some of the modifications allow for different happens-before
// relations to be produced depending on configuration parameters.
class EventGraphFixer {
public:
	EventGraphFixer(ActionLog* log,
					StringSet* vars,
					StringSet* scopes,
					SimpleDirectedGraph* event_graph,
					EventGraphInfo* graph_info);
	~EventGraphFixer();

	// Remove empty events with no follower. This is only an optimization
	// since these events have no effect on the races.
	void dropNoFollowerEmptyEvents();

	// Some happens before arcs were not created by the browser immediately,
	// but were left as races. We add them as explicit arcs.
	void addScriptsAndResourcesHappensBefore();

	// The events on a target should always happen after the target was created.
	// This adds these happens before arcs.
	void addEventAfterTargetHappensBefore();

	// Change the happens before for automatically explored events to not
	// make the events dependent on each other.
	void makeIndependentEventExploration();

private:
	ActionLog* m_log;
	StringSet* m_vars;
	StringSet* m_scopes;
	SimpleDirectedGraph* m_eventGraph;
	EventGraphInfo* m_graphInfo;
};


#endif /* GRAPHFIX_H_ */
