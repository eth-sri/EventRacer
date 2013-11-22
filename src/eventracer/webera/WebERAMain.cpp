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
#include <string.h>
#include <unistd.h>

#include "gflags/gflags.h"

#include "Escaping.h"
#include "mongoose.h"
#include "RaceApp.h"
#include "stringprintf.h"
#include "strutil.h"
#include "TraceReorder.h"
#include "UrlEncoding.h"

DEFINE_string(port, "8000", "Port where the web server listens.");
DECLARE_string(dot_temp_dir);

DEFINE_string(in_schedule_file, "/tmp/schedule.data",
		"Filename with the schedules");

DEFINE_string(out_schedule_file, "/tmp/new_schedule.data",
		"Filename with the schedules");

DEFINE_string(javascript_files, "http://localhost:8000/",
		"The path to the JavaScript/CSS files");

namespace {

RaceApp* race_app;
TraceReorder* reorder;

void handleCreateSchedule(const std::string& params, std::string* reply) {
	URLParams p;
	p.parse(params);
	std::vector<std::string> s;
	SplitStringUsing(p.getString("r"), 'R', &s);

	std::vector<TraceReorder::Reverse> all_reverses;
	const VarsInfo& vinfo = race_app->vinfo();
	for (size_t i = 0; i < s.size(); ++i) {
		int race_id;
		if (sscanf(s[i].c_str(), "%d", &race_id) == 1 &&
				race_id >= 0 && race_id < static_cast<int>(vinfo.races().size())) {
			const VarsInfo::RaceInfo& race = vinfo.races()[race_id];
			TraceReorder::Reverse rev;
			rev.node1 = race.m_event1;
			rev.node2 = race.m_event2;
			all_reverses.push_back(rev);
		}
	}

	std::vector<int> order;
	if (!reorder->GetSchedule(all_reverses, race_app->graph(), &order)) {
		reply->append("{\"status\": \"Could not get create the desired schedule...\"}");
	} else {
		reorder->SaveSchedule(FLAGS_out_schedule_file.c_str(), order);
		StringAppendF(reply, "{\"status\": \"Schedule with %d reversed races written to %s\"}",
				static_cast<int>(all_reverses.size()), FLAGS_out_schedule_file.c_str());
	}
}

void handleMain(const std::string& params, std::string* reply) {
	reply->append("<html><body>");
	StringAppendF(reply,
			"<html><head>"
			"<link rel=\"stylesheet\" type=\"text/css\" href=\"%s/sortable.css\">"
			"</head><body>\n",
			FLAGS_javascript_files.c_str());

	StringAppendF(reply,
			"<h2>Welcome to WebERA. Drag races to create schedules with reversed races.</h2>"
			"<h2>or click on races too see them in <a href=\"/varlist\">EventRacer</a>.</h2>"
			"<h4><b>Status:</b> <span id=\"status\">drag races below, please!</span>"
			"&nbsp;&nbsp;<input type=\"button\" id=\"reschedule\" value=\"[re]create schedule!\"></h4>\n"
			"<section id=\"connected\">"
			"<ul class=\"connected list\" id=\"races\">Races to reverse:</ul>\n"
			"<ul class=\"connected list no2\">Uncovered races:");

	const VarsInfo& vinfo = race_app->vinfo();
	const StringSet& vars = race_app->vars();
	int num_races = vinfo.races().size();
	for (int race_id = 0; race_id < num_races; ++race_id) {
		const VarsInfo::RaceInfo& race = vinfo.races()[race_id];
		if (race.m_coveredBy == -1 && race.m_multiParentRaces.empty()) {
			StringAppendF(reply,
					"<li id=\"R%d\"><a href=\"/race?id=%d\" target=\"_blank\">%d: %s</a> %s</li>",
					race_id, race_id, race_id,
					race.TypeShortStr(),
					HTMLEscape(vars.getString(race.m_varId)).c_str());
		}
	}

	StringAppendF(reply, "</ul></section>");

	StringAppendF(reply,
			"<script src=\"%s/jquery.min.js\"></script>"
			"<script src=\"%s/jquery.sortable.js\"></script>"
			"<script src=\"%s/webera.js\"></script>",
			FLAGS_javascript_files.c_str(),
			FLAGS_javascript_files.c_str(),
			FLAGS_javascript_files.c_str());
	StringAppendF(reply, "</body>\n");
}

static int request_handler(struct mg_connection *conn) {
	const struct mg_request_info *request_info = mg_get_request_info(conn);

	std::string request_path = request_info->uri == NULL ? "" : request_info->uri;
	std::string params = request_info->query_string == NULL ? "" : request_info->query_string;
	printf("Handling request='%s' with params='%s'\n", request_path.c_str(), params.c_str());

	std::string reply;
	if (request_path == "/") {
		handleMain(params, &reply);
	} else if (request_path == "/schedule.json") {
		handleCreateSchedule(params, &reply);
	} else if (request_path == "/info") {
		race_app->handleInfo(params, &reply);
	} else if (request_path == "/varlist") {
		race_app->handleVarList(params, &reply);
	} else if (request_path == "/var") {
		race_app->handleVarDetails(params, &reply);
	} else if (request_path == "/child") {
		race_app->handleRaceChildren(params, &reply);
	} else if (request_path == "/race") {
		race_app->handleRaceDetails(params, &reply);
	} else if (request_path == "/hb") {
		race_app->handleBrowseGraph(params, &reply);
	} else if (request_path == "/code") {
		race_app->handleShowCode(params, &reply);
	} else if (request_path == "/js") {
		race_app->handleShowJS(params, &reply);
	} else if (request_path == "/rel") {
		race_app->handleNodeRelation(params, &reply);
	} else if (request_path == "/undef") {
		race_app->handleUndefRaces(params, &reply);
	} else {
		// Returning 0 means that mongoose must handle replying.
		return 0;
	}

	// Send HTTP reply to the client
	mg_printf(conn,
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: %s\r\n"
			"Content-Length: %d\r\n"        // Always set Content-Length
			"\r\n"
			"%s",
			request_path.find(".gif") != std::string::npos ? "image/gif" :
					(request_path.find(".json") != std::string::npos ? "application/json" : "text/html"),
			static_cast<int>(reply.size()), reply.c_str());

	// Returning non-zero tells mongoose that our function has replied to
	// the client, and mongoose should not send client any more data.
	return 1;
}


}  // namespace


int main(int argc, char* argv[]) {
	google::ParseCommandLineFlags(&argc, &argv, true);

	struct mg_context *ctx;
	struct mg_callbacks callbacks;

	// List of options. Last element must be NULL.
	const char *options[] = {
			"listening_ports", FLAGS_port.c_str(),
			"document_root", FLAGS_dot_temp_dir.c_str(),
			NULL};

	// Prepare callbacks structure. We have only one callback, the rest are NULL.
	memset(&callbacks, 0, sizeof(callbacks));
	callbacks.begin_request = &request_handler;

	if (argc != 2) {
		fprintf(stderr, "One must specify an input ER_actionlog file\nUsage %s <file>\n", argv[0]);
		return 1;
	}

	reorder = new TraceReorder();
	reorder->LoadSchedule(FLAGS_in_schedule_file.c_str());
	// Creating a race app.
	race_app = new RaceApp(0, argv[1]);

	// Start the web server.
	ctx = mg_start(&callbacks, NULL, options);

	printf("Web server started on port %s. Open http://localhost:%s/ in your browser...\n",
			FLAGS_port.c_str(), FLAGS_port.c_str());

	for (;;) {
		sleep(10);
	}

	// Stop the server.
	mg_stop(ctx);

	delete race_app;
	delete reorder;

	return 0;
}
