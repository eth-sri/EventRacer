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

#ifndef BITCLOCKS_H_
#define BITCLOCKS_H_

#include <vector>
#include "EventGraph.h"

// Computes happens before using vector clocks of width |num_nodes|, but with optimized storage for
// one bit per vector clock value (such vector clocks may have values only of 0 and 1).
class BitClocks : public EventGraphInterface {
public:
	BitClocks();
	void build(const SimpleDirectedGraph& graph);

	virtual bool areOrdered(int slice1, int slice2) const;

private:
	void computeBitClocks(const SimpleDirectedGraph& graph);

	std::vector<std::vector<unsigned int> > m_bitClocks;
};

#endif /* BITCLOCKS_H_ */
