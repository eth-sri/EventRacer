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

#ifndef EVENTGRAPHVIZ_H_
#define EVENTGRAPHVIZ_H_

#include <map>
#include <set>
#include <string>

#include "ActionLogPrint.h"
#include "GraphViz.h"
#include "EventGraph.h"
#include "EventGraphInfo.h"
#include "ActionLog.h"
#include "UrlEncoding.h"
#include "VarsInfo.h"

class NodeSelection;

class EventGraphDisplay {
public:
	explicit EventGraphDisplay(
			const std::string& cmd,
			const std::string& file_name,
			const URLParams& params,
			const ActionLog* action_log,
			const EventGraphInfo* graph_info,
			const SimpleDirectedGraph* original_graph,
			const SimpleDirectedGraph* timer_graph);
	~EventGraphDisplay();

	static const int NODE_FOCUS = 20;
	static const int NODE_FIRST_NODE = 0;

	static const int NODE_HAS_ACCESS = 16;

	static const int NODE_FOCUS_RACE = 19;
	static const int NODE_FOCUS_PARENT_RACE = 17;

	static const int NODE_FOCUS_CAUSE = 17;

	void tryIncludeNode(int node_id, int priority, const std::string& comment);

	void outputGraph(const ActionLogPrinter* action_printer, std::string* output);

	void addRaceArc(int race_id, const VarsInfo::RaceInfo& race, const char* color);
private:
	void addNode(const ActionLogPrinter* action_printer, int node_id);

	void addArcIfThere(int from, int to);

	struct Race {
		Race(int id, const VarsInfo::RaceInfo& info, const char* color) : m_id(id), m_varInfo(&info), m_color(color) {}
		int m_id;
		const VarsInfo::RaceInfo* m_varInfo;
		const char* m_color;
	};

	std::set<int> m_includedNodes;
	std::map<int, std::string> m_captions;
	std::vector<Race> m_raceArcs;

	std::string m_linkCmd;
	std::string m_fileName;
	URLParams m_params;
	const ActionLog* m_actionLog;
	const EventGraphInfo* m_graphInfo;
	const SimpleDirectedGraph* m_originalGraph;
	const SimpleDirectedGraph* m_timerGraph;

	// The output graph visualizer.
	GraphViz m_graphViz;

	// The node which should always be focused. (-1 if none).
	int m_focusNode;

	NodeSelection* m_nodeSelection;
};


#endif /* EVENTGRAPHVIZ_H_ */
