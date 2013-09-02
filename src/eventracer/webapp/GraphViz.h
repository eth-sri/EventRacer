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

#ifndef GRAPHVIZ_H_
#define GRAPHVIZ_H_

#include <map>
#include <string>
#include <set>
#include <utility>

class GraphViz {
public:
	struct Node {
		Node() : m_shape(NULL), m_style(NULL), m_color(NULL), m_fillcolor(NULL) {}
		std::string m_label;
		std::string m_url;
		const char* m_shape;
		const char* m_style;
		const char* m_color;
		const char* m_fillcolor;
	};

	struct Arc {
		Arc() : m_duration(-1), m_style(NULL), m_color(NULL), m_label(NULL), m_arrowHead(NULL), m_fontColor(NULL) {}
		int m_duration;
		const char* m_style;
		const char* m_color;
		const char* m_label;
		const char* m_arrowHead;
		const char* m_fontColor;
		std::string m_url;
	};

	Node* getNode(int node_id) {
		return &m_nodes[node_id];
	}

	Arc* getArc(int source, int target) {
		return &m_arcs[std::make_pair(source, target)];
	}

	void output(const std::string& graph_name, std::string* out_html);
private:
	typedef std::map<int, Node> NodeMap;
	typedef std::map<std::pair<int, int>, Arc> ArcMap;
	NodeMap m_nodes;
	ArcMap m_arcs;
};


#endif /* GRAPHVIZ_H_ */
