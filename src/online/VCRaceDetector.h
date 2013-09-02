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

#include <string>
#include <vector>

namespace RaceDetector {

// Keeps a race between a pair of event actions.
struct Race {
	enum Operation {
		READ,
		WRITE
	};

	Operation op1;
	int eventAction1;
	Operation op2;
	int eventAction2;
};

class VCInstrumentedState;
class VectorClockWithChain;

class VCRaceDetector {
public:
	VCRaceDetector();
	virtual ~VCRaceDetector();

	// Denotes the start of an event action. The current event action
	// becomes the given id. The ids must be increasing and positive numbers.
	void beginEventAction(int eventActionId);

	// Denotes an end of an event action. After this call, no event action is started.
	void endEventAction();

	int currentEventAction() const {
		if (!m_isInEventAction) return -1;
		return m_currentEventAction;
	}

	// Denotes that the currently started event action is after previousEventAction.
	// Note that this call should be done after beginEventAction to ensure no false
	// positives.
	void denoteCurrentEventAfter(int previousEventAction);

	// Records that an operation is performed. Eventually appends a race if a race is discovered.
	void recordOperation(Race::Operation op, const std::string& variableName,
			std::vector<Race>* races);

	// For each race, a caller must report that a race is synchronization. In general, if not
	// sure, you should report all races as synchronization to avoid false positives.
	void recordRaceIsSync(const Race& race);

private:
	int m_currentEventAction;
	bool m_isInEventAction;
	bool m_eventActionHadOperations;
	VCInstrumentedState* m_state;
	VectorClockWithChain* m_currentVC;
};

}  // namespace RaceDetector
