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
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <sys/stat.h>

#include "gflags/gflags.h"

#include "base.h"
#include "mutex.h"
#include "stringprintf.h"
#include "mongoose.h"
#include "Escaping.h"
#include "RaceApp.h"

#include "UrlEncoding.h"


DEFINE_string(port, "8000", "Port where the web server listens.");
DECLARE_string(dot_temp_dir);

DEFINE_string(action_log_dir, "/home/veselin/wk/alog",
		"Directory with action log files.");
DEFINE_string(webkit_browser_exec, "/home/veselin/gitwk/eventracer/fetch_with_auto_explore.sh",
		"Executable for the webkit browser.");

DEFINE_int32(num_cached_race_files, 5, "Maximum number of race files stored in memory.");

namespace {

std::string PathFromFetchId(int64 fetch_id) {
	return StringPrintf("%s/%lld", FLAGS_action_log_dir.c_str(), fetch_id);
}

int64 GenerateFetchId() {
	int64 fetch_id;
	std::string path;
	for (;;) {
		fetch_id = GetCurrentTimeMicros();
		path = PathFromFetchId(fetch_id);
		int error = mkdir(path.c_str(), 0775);
		if (error == 0) break;  // Directory was created successfully.
		if (error == EEXIST) {
			// Random sleep.
			usleep(rand() % 1000 + 1);
		} else {
			fprintf(stderr, "Error from file system.\n");
			exit(1);
		}
	}
	return fetch_id;
}

bool FileExists(std::string file_name) {
	struct stat s;
	return stat(file_name.c_str(), &s) == 0;
}

std::string ERLogFileNameFromFetchID(int64 fetch_id) {
	return PathFromFetchId(fetch_id) + "/ER_actionlog";
}

void HandleFetch(std::string params, std::string* reply) {
	URLParams dp;
	dp.parse(params);
	std::string url = dp.getString("url");

	if (url.empty() || url.find(' ') != std::string::npos) {
		*reply = "<html><head></head><body>Please provide a valid \"url\" parameter to fetch</body></html>";
		return;
	}

	int64 fetch_id = GenerateFetchId();

	std::string command;

	StringAppendF(&command, "%s \"%s\" %s",
			FLAGS_webkit_browser_exec.c_str(),
			StringEscape(url).c_str(),
			PathFromFetchId(fetch_id).c_str());
	printf("Running command: \"%s\"\n", command.c_str());

	// Fetch the web site.
	int system_code = system(command.c_str());
	// Ignore system code.

	if (FileExists(ERLogFileNameFromFetchID(fetch_id))) {
		StringAppendF(reply,
				"<html><head></head><body>"
				"<center>"
				"  <p>The website %s was explored</p>"
				"  <h2><a href=\"/view/%lld/varlist\" target=\"_blank\">Click to view races</a></h2>"
				"  <p>(opens a new window)</p>"
				"  <!-- Return Code = %d , FetchID = %lld -->"
				"</center>"
				"</body></html>",
				HTMLEscape(url).c_str(), fetch_id, system_code, fetch_id);
	} else {
		StringAppendF(reply,
				"<html><head></head><body>"
				"<center>"
				"  <p>Failed to fetch %s/p>"
				"  <!-- Return Code = %d , FetchID = %lld -->"
				"</center>"
				"</body></html>",
				HTMLEscape(url).c_str(), system_code, fetch_id);
	}
}

mutex race_apps_mutex;
std::vector<std::pair<int64, RaceApp*> > race_apps;

RaceApp* GetRaceAppFromFetchId(int64 fetch_id) {
	lock_guard<mutex> mutex_lock(race_apps_mutex);  // Keep the mutex locked while the log file is loading.
	for (size_t i = 0; i < race_apps.size(); ++i) {
		if (race_apps[i].first == fetch_id) {
			// TODO: move the app forward as the most recently used app.
			return race_apps[i].second;
		}
	}

	if (static_cast<int>(race_apps.size()) >= FLAGS_num_cached_race_files) {
		delete race_apps[race_apps.size() - 1].second;
		race_apps.erase(race_apps.begin() + (race_apps.size() - 1));
	}


	std::string file_name = ERLogFileNameFromFetchID(fetch_id);
	if (!FileExists(file_name)) return NULL;

	RaceApp* new_app = new RaceApp(fetch_id, file_name);
	race_apps.push_back(std::pair<int64, RaceApp*>(fetch_id, new_app));
	return new_app;
}

//RaceApp* race_app;

static int request_handler(struct mg_connection *conn) {
	const struct mg_request_info *request_info = mg_get_request_info(conn);

	std::string request_path = request_info->uri == NULL ? "" : request_info->uri;
	std::string params = request_info->query_string == NULL ? "" : request_info->query_string;
	printf("Handling request='%s' with params='%s'\n", request_path.c_str(), params.c_str());

	std::string reply;
	if (request_path == "/fetch") {
		HandleFetch(params, &reply);
	} else if (strncmp(request_path.c_str(), "/view/", 6) == 0) {
		int64 fetch_id;
		if (sscanf(request_path.c_str(), "/view/%lld", &fetch_id) == 1) {
			RaceApp* race_app = GetRaceAppFromFetchId(fetch_id);
			if (race_app) {
				size_t pos = request_path.find("/", 6);
				if (pos == std::string::npos) return 0;

				request_path = request_path.substr(pos, request_path.size() - pos);
				printf("Req path: %s\n", request_path.c_str());
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
			} else {
				return 0;
			}
		} else {
			return 0;
		}
	} else {
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

	if (FLAGS_action_log_dir.empty()) {
		fprintf(stderr, "--action_log_dir is a required parameter");
		return -1;
	}

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

	// Creating a race app.
	//race_app = new RaceApp(argv[1]);

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

