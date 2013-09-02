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

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "GraphViz.h"
#include "stringprintf.h"

#include "gflags/gflags.h"

DEFINE_string(dot_executable, "/usr/bin/dot", "Executable with the dot tool. If empty, the dot tool is not called.");
DEFINE_string(dot_temp_dir, "/tmp/raceanalyzer", "Directory where a temporary .dot file will be placed.");

using std::string;

class DotStyleBuilder {
public:
	DotStyleBuilder(FILE* f) : m_f(f), m_opened(false) {}
	void add(const char* key, const char* value) {
		fprintf(m_f, m_opened ? ", %s=\"%s\"" : " [%s=\"%s\"", key, value);
		m_opened = true;
	}
	void finish() {
		fprintf(m_f, m_opened ? "];\n" : ";\n");
	}
private:
	FILE* m_f;
	bool m_opened;
};


void GraphViz::output(const std::string& graph_name, std::string* out_html) {
	if (FLAGS_dot_executable.empty()) return;  // Do not print graph.

	mkdir(FLAGS_dot_temp_dir.c_str(), 0777);
	string file_prefix = FLAGS_dot_temp_dir + "/" + graph_name;
	string dot_file_name = file_prefix + ".dot";
	FILE* dotf = fopen(dot_file_name.c_str(), "wt");
	if (!dotf) {
		fprintf(stderr, "Can open output dot file %s.", dot_file_name.c_str());
		return;
	}
	fprintf(dotf, "digraph %s {\n", graph_name.c_str());
	fprintf(dotf, "  node[fontsize=7.5];\n");
	for (NodeMap::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it) {
		string label;
		if (it->second.m_label.empty()) {
			StringAppendF(&label, "Node %d", it->first);
		} else {
			label = it->second.m_label;
		}
		fprintf(dotf, "  N%d", it->first);
		DotStyleBuilder nodeStyle(dotf);
		nodeStyle.add("label", label.c_str());
		if (!it->second.m_url.empty()) {
			nodeStyle.add("URL", it->second.m_url.c_str());
		}
		if (it->second.m_color) {
			nodeStyle.add("color", it->second.m_color);
		}
		if (it->second.m_fillcolor) {
			nodeStyle.add("fillcolor", it->second.m_fillcolor);
		}
		if (it->second.m_style) {
			nodeStyle.add("style", it->second.m_style);
		}
		if (it->second.m_shape) {
			nodeStyle.add("shape", it->second.m_shape);
		}
		nodeStyle.finish();
	}
	for (ArcMap::const_iterator it = m_arcs.begin(); it != m_arcs.end(); ++it) {
		fprintf(dotf, "  N%d -> N%d", it->first.first, it->first.second);
		DotStyleBuilder arcStyle(dotf);
		if (it->second.m_label != NULL) {
			arcStyle.add("label", it->second.m_label);
		} else if (it->second.m_duration >= 0) {
			arcStyle.add("label", StringPrintf("%d", it->second.m_duration).c_str());
		}
		if (it->second.m_color) {
			arcStyle.add("color", it->second.m_color);
		}
		if (it->second.m_style) {
			arcStyle.add("style", it->second.m_style);
		}
		if (it->second.m_arrowHead) {
			arcStyle.add("arrowhead", it->second.m_arrowHead);
		}
		if (it->second.m_fontColor) {
			arcStyle.add("fontcolor", it->second.m_fontColor);
		}
		if (!it->second.m_url.empty()) {
			arcStyle.add("URL", it->second.m_url.c_str());
		}
		arcStyle.finish();
	}
	fprintf(dotf, "}");
	fclose(dotf);

	string map_file_name = file_prefix + ".map";
	string img_file_name = file_prefix + ".gif";

	// Example:
	//  dot -Tcmapx -o <file>.map -Tgif -o <file>.gif <file>.dot
	std::string command = FLAGS_dot_executable + " -Tcmapx -o " + map_file_name +
			" -Tgif -o " + img_file_name + " " + dot_file_name;
	if (system(command.c_str()) != 0) {
		fprintf(stderr, "Can't start %s\n", command.c_str());
	}

    StringAppendF(out_html,
    		"<IMG SRC=\"/%s.gif\" USEMAP=\"#%s\">",
    		graph_name.c_str(), graph_name.c_str());

    FILE* mapf = fopen(map_file_name.c_str(), "rb");
    if (!mapf) return;
	char buf[512];
	for (;;) {
		size_t bytes_read = fread(buf, sizeof(char), 512, mapf);
		if (bytes_read == 0) break;
		out_html->append(buf, bytes_read);
	}
    fclose(mapf);
}
