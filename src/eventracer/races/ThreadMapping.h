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

#ifndef THREADMAPPING_H_
#define THREADMAPPING_H_

#include <vector>
#include "EventGraph.h"

// Maps atomic pieces to threads.
class ThreadMapping : public EventGraphInterface {
public:
	ThreadMapping();
	void build(const SimpleDirectedGraph& graph);

	void computeVectorClocks(const SimpleDirectedGraph& graph);

	int num_threads() const { return m_numThreads; }

	virtual bool areOrdered(int slice1, int slice2) const;

//	const std::vector<int>& getClockForSlice(int slice) const {
//		return m_vectorClocks[slice];
//	}

private:
	void assignNodesToThread(const SimpleDirectedGraph& graph, int startNode, int threadId);

	std::vector<int> m_nodeThread;
	int m_numThreads;

	std::vector<std::vector<short> > m_vectorClocks;
};

#endif /* THREADMAPPING_H_ */
