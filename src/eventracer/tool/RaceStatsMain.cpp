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

#include "RaceStats.h"

#include <dirent.h>
#include <stdio.h>
#include <vector>
#include <string>


int main(int argc, char* argv[]) {
	std::string path("/home/veselin/wk/eval/tiny_set_ae");
	if (argc == 2) {
		path = argv[1];
	}

	struct dirent *entry;
	DIR *dp;
	dp = opendir(path.c_str());
	if (dp == NULL) {
		perror("opendir");
		return 1;
	}

	std::vector<RaceFile*> files;
	while ((entry = readdir(dp))) {
		if (entry->d_type == DT_REG) {
			RaceFile* file = new RaceFile();
			std::string filename = path + "/" + entry->d_name;
			if (!file->Load(filename, false)) {
				perror("Failed loading race log");
				delete file;
				continue;
			}
			file->setFileId(entry->d_name);
			files.push_back(file);
		}
	}
	closedir(dp);
	printf("Loaded %d files\n", static_cast<int>(files.size()));

	printf("Computation time statistics\n");
	for (size_t file_i = 0; file_i < files.size(); ++file_i) {
		files[file_i]->printTimeStats();
	}

	printf("\nRace statistics\n");
	for (size_t file_i = 0; file_i < files.size(); ++file_i) {
		if (file_i == 0) files[file_i]->printVarStatsHeader();
		files[file_i]->printVarStats();
	}
/*
	printf("\nAccordion clocks statistics\n");
	for (size_t file_i = 0; file_i < files.size(); ++file_i) {
		files[file_i]->evaluateAccordionClocks();
	}
*/

	printf("\nHigh risk races.\n");
	for (size_t file_i = 0; file_i < files.size(); ++file_i) {
		files[file_i]->printHighRiskRaces();
	}

	for (size_t i = 0; i < files.size(); ++i) {
		delete files[i];
	}
	return 0;
}
