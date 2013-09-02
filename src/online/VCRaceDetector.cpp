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

#include <iostream>


#include <map>
#include <utility>
#include <vector>

namespace RaceDetector {

// Throws 0 and return cerr if COND is not met.
#define CHECK(COND) \
	if (COND) ; else for (;;std::cerr << "\n^^^ CHECK failed ^^^\n", throw 0) std::cerr \

// Keeps one vector clock together with its chain index. An epoch is constructed by taking a pair (chain, vc[chain]).
class VectorClockWithChain {
public:
	VectorClockWithChain() : m_chainId(-1) { }

	int getChain() const {
		return m_chainId;
	}

	void setChain(int chainId) {
		m_chainId = chainId;
	}

	int getTimeStamp() const {
		return m_data[m_chainId];
	}

	bool isAfterEpoch(int chain, int timeStamp) const {
		return timeStamp <= getComponent(chain);
	}

	void joinWith(const VectorClockWithChain& other) {
		makeSizeAtLeast(other.m_data.size());
		for (size_t i = 0; i < other.m_data.size(); ++i) {
			if (m_data[i] < other.m_data[i]) {
				m_data[i] = other.m_data[i];
			}
		}
	}

	void incComponent(size_t component) {
		makeSizeAtLeast(component + 1);
		++m_data[component];
	}

	void joinWithEpoch(size_t component, int timeStamp) {
		makeSizeAtLeast(component + 1);
		if (m_data[component] < timeStamp) {
			m_data[component] = timeStamp;
		}
	}

	int getComponent(size_t component) const {
		if (component < m_data.size()) return m_data[component];
		return 0;
	}

	size_t numComponents() const {
		return m_data.size();
	}

private:
	void makeSizeAtLeast(size_t size) {
		if (m_data.size() < size) {
			m_data.resize(size, 0);
		}
	}

	int m_chainId;
	std::vector<int> m_data;
};

// FastTrack like state for a variable.
class RWState {
public:
	RWState() :
		lastWriteChain(0), lastWriteTimeStamp(0),
		lastReadChain(0), lastReadTimeStamp(0) {
	}

	int lastWriteChain;
	int lastWriteTimeStamp;

	int lastReadChain;
	int lastReadTimeStamp;

	std::vector<int> readVector;
};

typedef std::map<int, VectorClockWithChain> VCMap;
typedef std::map<std::string, RWState> VarRWState;
typedef std::map<std::pair<int, int>, int> ReverseTimeStampLookup;

// State for all variables and all the threads.
class VCInstrumentedState {
public:
	void addArc(int node1, int node2) {
		CHECK(node1 < node2) << "Invalid arc in a DAG - (" << node1 << "," << node2 << ")";
		m_data[node2].joinWith(m_data[node1]);
	}

	VectorClockWithChain* assignChain(int node) {
		VectorClockWithChain& vc = m_data[node];
		if (vc.getChain() != -1) return &vc;  // The node already has a chain assigned.
		for (size_t i = 0; i <= m_maxVC.numComponents(); ++i) {
			if (vc.getComponent(i) == m_maxVC.getComponent(i)) {
				vc.setChain(i);

				// Increment both the VC and the MAX i-th components (they are equal).
				vc.incComponent(i);
				m_maxVC.incComponent(i);

				m_lookupByChainTimestamp[std::pair<int, int>(vc.getChain(), vc.getTimeStamp())] = node;

				return &vc;
			}
		}

		// Should never happen.
		CHECK(false) << "Cannot assign chain.";
		return 0;
	}

	RWState& getVarRWState(const std::string& var) {
		return m_vars[var];
	}

	int getNodeByChainAndTimeStamp(int chain, int timestamp) {
		ReverseTimeStampLookup::const_iterator it =
				m_lookupByChainTimestamp.find(std::pair<int, int>(chain, timestamp));
		if (it == m_lookupByChainTimestamp.end()) return -1;
		return it->second;
	}

private:
	VectorClockWithChain m_maxVC;
	VCMap m_data;

	VarRWState m_vars;
	ReverseTimeStampLookup m_lookupByChainTimestamp;
};


VCRaceDetector::VCRaceDetector()
	: m_currentEventAction(-1),
	  m_isInEventAction(false),
	  m_eventActionHadOperations(false),
	  m_currentVC(0) {
	m_state = new VCInstrumentedState();
}

VCRaceDetector::~VCRaceDetector() {
	delete m_state;
}

void VCRaceDetector::beginEventAction(int eventActionId) {
	CHECK(!m_isInEventAction) << "Cannot begin recursive event actions.";
	CHECK(m_currentEventAction < eventActionId) << "Event action IDs must increase. was "
			<< m_currentEventAction << ", new " << eventActionId;

	m_isInEventAction = true;
	m_currentEventAction = eventActionId;
	m_eventActionHadOperations = false;
}

void VCRaceDetector::endEventAction() {
	CHECK(m_isInEventAction) << "Not in an event action";
	m_isInEventAction = false;
}

void VCRaceDetector::denoteCurrentEventAfter(int previousEventAction) {
	CHECK(m_isInEventAction) << "Not in an event action";
	CHECK(!m_eventActionHadOperations)
			<< "Do not change the happens-before after there were operations in the trace";
	m_state->addArc(previousEventAction, m_currentEventAction);
}

namespace {
void addEpochToVC(size_t chain, int timestamp, std::vector<int>* vc) {
	if (vc->size() <= chain) vc->resize(chain + 1, 0);
	(*vc)[chain] = timestamp;
}
}  // anonymous namespace

void VCRaceDetector::recordOperation(Race::Operation op, const std::string& variableName,
		std::vector<Race>* races) {
	CHECK(m_isInEventAction) << "Not in an event action";

	if (!m_eventActionHadOperations) {
		m_currentVC = m_state->assignChain(m_currentEventAction);
		m_eventActionHadOperations = true;
	}

	RWState& state = m_state->getVarRWState(variableName);

	// A race with a previous write.
	if (!m_currentVC->isAfterEpoch(state.lastWriteChain, state.lastWriteTimeStamp)) {
		Race r;
		r.op1 = Race::WRITE;
		r.eventAction1 = m_state->getNodeByChainAndTimeStamp(state.lastWriteChain, state.lastWriteTimeStamp);
		r.op2 = op;
		r.eventAction2 = m_currentEventAction;
		races->push_back(r);
	}

	if (op == Race::WRITE) {
		if (!state.readVector.empty()) {
			const std::vector<int>& readVector = state.readVector;
			for (size_t chain = 0; chain < readVector.size(); ++chain) {
				if (!m_currentVC->isAfterEpoch(chain, readVector[chain])) {
					Race r;
					r.op1 = Race::READ;
					r.eventAction1 = m_state->getNodeByChainAndTimeStamp(chain, readVector[chain]);
					r.op2 = Race::WRITE;
					r.eventAction2 = m_currentEventAction;
					races->push_back(r);
				}
			}
		} else {
			if (!m_currentVC->isAfterEpoch(state.lastReadChain, state.lastReadTimeStamp)) {
				Race r;
				r.op1 = Race::READ;
				r.eventAction1 = m_state->getNodeByChainAndTimeStamp(state.lastReadChain, state.lastReadTimeStamp);
				r.op2 = Race::WRITE;
				r.eventAction2 = m_currentEventAction;
				races->push_back(r);
			}
		}
		state.readVector.clear();
		state.lastReadChain = 0;
		state.lastReadTimeStamp = 0;
		state.lastWriteChain = m_currentVC->getChain();
		state.lastWriteTimeStamp = m_currentVC->getTimeStamp();
	} else if (op == Race::READ) {
		if (state.readVector.empty()) {
			if (state.lastReadTimeStamp == 0) {
				state.lastReadChain = m_currentVC->getChain();
				state.lastReadTimeStamp = m_currentVC->getTimeStamp();
			} else {
				addEpochToVC(state.lastReadChain, state.lastReadTimeStamp, &state.readVector);
				addEpochToVC(m_currentVC->getChain(), m_currentVC->getTimeStamp(), &state.readVector);
			}
		} else {
			addEpochToVC(m_currentVC->getChain(), m_currentVC->getTimeStamp(), &state.readVector);
		}
	}
}

void VCRaceDetector::recordRaceIsSync(const Race& race) {
	m_state->addArc(race.eventAction1, race.eventAction2);
}

}  // namespace RaceDetector
