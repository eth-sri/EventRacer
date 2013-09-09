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

#include "mongoose.h"
#include "RaceApp.h"

DEFINE_string(port, "8000", "Port where the web server listens.");
DECLARE_string(dot_temp_dir);

namespace {

RaceApp* race_app;

static int request_handler(struct mg_connection *conn) {
	const struct mg_request_info *request_info = mg_get_request_info(conn);

	std::string request_path = request_info->uri == NULL ? "" : request_info->uri;
	std::string params = request_info->query_string == NULL ? "" : request_info->query_string;
	printf("Handling request='%s' with params='%s'\n", request_path.c_str(), params.c_str());

	std::string reply;
	if (request_path == "/info" || request_path == "/") {
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
			request_path.find(".gif") != std::string::npos ? "image/gif" : "text/html",
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

	return 0;
}
