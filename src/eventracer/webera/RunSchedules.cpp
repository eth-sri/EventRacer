/*
   Copyright 2014 Software Reliability Lab, ETH Zurich

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
#include <vector>

#include "gflags/gflags.h"

#include "RaceApp.h"
#include "stringprintf.h"
#include "strutil.h"
#include "TraceReorder.h"

DEFINE_string(in_schedule_file, "/tmp/schedule.data",
		"Filename with the schedules");
DEFINE_string(out_schedule_file, "/tmp/new_schedule.data",
		"Filename with the schedules");

namespace {

RaceApp* race_app;
TraceReorder* reorder;

void createReorders() {
	const VarsInfo& vinfo = race_app->vinfo();

	TraceReorder::Options options;
	options.include_change_marker = true;
	options.minimize_variation_from_original = true;
	options.relax_replay_after_all_races = true;

	int all_schedules = 0;
	int successful_reverses = 0;
	int successful_schedules = 0;


	for (int race_id = 0; race_id < static_cast<int>(vinfo.races().size()); ++race_id) {
		const VarsInfo::RaceInfo& race = vinfo.races()[race_id];
		if (!race.m_multiParentRaces.empty() || race.m_coveredBy != -1) continue;

		std::vector<int> rev_races;
		rev_races.push_back(race_id);

		++all_schedules;
		std::vector<int> new_schedule;
		if (reorder->GetScheduleFromRaces(vinfo, rev_races, race_app->graph(), options, &new_schedule)) {
			++successful_reverses;
			reorder->SaveSchedule(FLAGS_out_schedule_file.c_str(), new_schedule);
		}
	}

	printf("Tried %d schedules. %d generated, %d successful\n",
			all_schedules, successful_reverses, successful_schedules);
}

}  // namespace


int main(int argc, char* argv[]) {
	google::ParseCommandLineFlags(&argc, &argv, true);
	reorder = new TraceReorder();
	reorder->LoadSchedule(FLAGS_in_schedule_file.c_str());
	race_app = new RaceApp(0, argv[1], false);

	createReorders();

	return 0;
}
