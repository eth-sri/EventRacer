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


#include "VCRaceDetector.h"

#include <stdio.h>

#include <string>

void printRaces(const std::vector<RaceDetector::Race>& races) {
	printf("%d races\n", static_cast<int>(races.size()));
	for (size_t i = 0; i < races.size(); ++i) {
		printf("%d - %d\n", races[i].eventAction1, races[i].eventAction2);
	}
}

void expectRace(RaceDetector::VCRaceDetector* rd, RaceDetector::Race::Operation op, const char* var, const char* exp_races) {
	std::vector<RaceDetector::Race> races;
	rd->recordOperation(op, var, &races);

	std::string races_str;
	for (size_t i = 0; i < races.size(); ++i) {
		char tmp[32];
		sprintf(tmp, "[%s%d-%s%d]",
				races[i].op1 == RaceDetector::Race::WRITE ? "w" : "r",
				races[i].eventAction1,
				races[i].op2 == RaceDetector::Race::WRITE ? "w" : "r",
				races[i].eventAction2);
		races_str.append(tmp);
	}

	if (races_str != exp_races) {
		fprintf(stderr, "Test failed on variable %s! Expected races: %s, actual %s\n^^^ FAIL ^^^\n",
				var, exp_races, races_str.c_str());
		throw 0;
	}
}


void testAllIsRacing() {
	printf("Starting test testAllIsRacing...\n");
	RaceDetector::VCRaceDetector rd;

	rd.beginEventAction(0);
	expectRace(&rd, RaceDetector::Race::WRITE, "a", "");
	rd.endEventAction();

	rd.beginEventAction(1);
	expectRace(&rd, RaceDetector::Race::READ, "a", "[w0-r1]");
	rd.endEventAction();

	rd.beginEventAction(2);
	expectRace(&rd, RaceDetector::Race::READ, "a", "[w0-r2]");
	expectRace(&rd, RaceDetector::Race::READ, "b", "");
	expectRace(&rd, RaceDetector::Race::READ, "c", "");
	rd.endEventAction();

	rd.beginEventAction(3);
	expectRace(&rd, RaceDetector::Race::WRITE, "a", "[w0-w3][r1-w3][r2-w3]");
	expectRace(&rd, RaceDetector::Race::WRITE, "b", "[r2-w3]");
	rd.endEventAction();

	rd.beginEventAction(4);
	expectRace(&rd, RaceDetector::Race::WRITE, "a", "[w3-w4]");
	expectRace(&rd, RaceDetector::Race::READ, "c", "");
	rd.endEventAction();

	rd.beginEventAction(5);
	expectRace(&rd, RaceDetector::Race::READ, "a", "[w4-r5]");
	rd.endEventAction();

	rd.beginEventAction(6);
	expectRace(&rd, RaceDetector::Race::WRITE, "a", "[w4-w6][r5-w6]");
	expectRace(&rd, RaceDetector::Race::WRITE, "c", "[r2-w6][r4-w6]");
	rd.endEventAction();
	printf("Success\n");
}

void testNoneIsRacing() {
	printf("Starting test testNoneIsRacing...\n");
	RaceDetector::VCRaceDetector rd;

	rd.beginEventAction(0);
	expectRace(&rd, RaceDetector::Race::WRITE, "a", "");
	rd.endEventAction();

	rd.beginEventAction(1);
	rd.denoteCurrentEventAfter(0);
	expectRace(&rd, RaceDetector::Race::READ, "a", "");
	rd.endEventAction();

	rd.beginEventAction(2);
	rd.denoteCurrentEventAfter(0);
	expectRace(&rd, RaceDetector::Race::READ, "a", "");
	rd.endEventAction();

	rd.beginEventAction(3);
	rd.denoteCurrentEventAfter(1);
	rd.denoteCurrentEventAfter(2);
	expectRace(&rd, RaceDetector::Race::WRITE, "a", "");
	rd.endEventAction();

	rd.beginEventAction(4);
	rd.denoteCurrentEventAfter(3);
	expectRace(&rd, RaceDetector::Race::WRITE, "a", "");
	rd.endEventAction();

	rd.beginEventAction(5);
	rd.denoteCurrentEventAfter(4);
	expectRace(&rd, RaceDetector::Race::READ, "a", "");
	rd.endEventAction();

	rd.beginEventAction(6);
	rd.denoteCurrentEventAfter(5);
	expectRace(&rd, RaceDetector::Race::WRITE, "a", "");
	rd.endEventAction();
	printf("Success\n");
}

void testSomeIsRacing1() {
	printf("Starting test testSomeIsRacing1...\n");
	RaceDetector::VCRaceDetector rd;

	rd.beginEventAction(0);
	expectRace(&rd, RaceDetector::Race::WRITE, "a", "");
	rd.endEventAction();

	rd.beginEventAction(1);
	rd.denoteCurrentEventAfter(0);
	expectRace(&rd, RaceDetector::Race::READ, "a", "");
	rd.endEventAction();

	rd.beginEventAction(2);
	rd.denoteCurrentEventAfter(0);
	expectRace(&rd, RaceDetector::Race::READ, "a", "");
	rd.endEventAction();

	rd.beginEventAction(3);
	rd.denoteCurrentEventAfter(2);
	expectRace(&rd, RaceDetector::Race::WRITE, "a", "[r1-w3]");
	rd.endEventAction();

	rd.beginEventAction(4);
	rd.denoteCurrentEventAfter(3);
	expectRace(&rd, RaceDetector::Race::WRITE, "a", "");
	rd.endEventAction();

	rd.beginEventAction(5);
	rd.denoteCurrentEventAfter(3);
	expectRace(&rd, RaceDetector::Race::READ, "a", "[w4-r5]");
	rd.endEventAction();

	rd.beginEventAction(6);
	rd.denoteCurrentEventAfter(5);
	expectRace(&rd, RaceDetector::Race::WRITE, "a", "[w4-w6]");
	rd.endEventAction();

	rd.beginEventAction(7);
	rd.denoteCurrentEventAfter(5);
	expectRace(&rd, RaceDetector::Race::WRITE, "a", "[w6-w7]");
	rd.endEventAction();
	printf("Success\n");
}

void testCoveringRaces() {
	printf("Starting test testCoveringRaces...\n");
	RaceDetector::VCRaceDetector rd;

	rd.beginEventAction(0);
	expectRace(&rd, RaceDetector::Race::WRITE, "a", "");
	rd.endEventAction();

	rd.beginEventAction(1);
	expectRace(&rd, RaceDetector::Race::READ, "a", "[w0-r1]");
	expectRace(&rd, RaceDetector::Race::READ, "a", "[w0-r1]");

	RaceDetector::Race r;
	r.eventAction1 = 0;
	r.eventAction2 = 1;
	rd.recordRaceIsSync(r);
	expectRace(&rd, RaceDetector::Race::READ, "a", "");
	expectRace(&rd, RaceDetector::Race::WRITE, "a", "");

	rd.endEventAction();
}

int main(void) {
	testAllIsRacing();
	testNoneIsRacing();
	testSomeIsRacing1();
	testCoveringRaces();
	return 0;
}
