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

#include "BitClocks.h"

#include <stdio.h>

#include "base.h"


BitClocks::BitClocks() {
}

void BitClocks::build(const SimpleDirectedGraph& graph) {
	int nodes = graph.numNodes();
	std::vector<unsigned int> empty((nodes + 31) / 32, 0);
	m_bitClocks.assign(nodes, empty);
	computeBitClocks(graph);
}

void BitClocks::computeBitClocks(const SimpleDirectedGraph& graph) {
	printf("Computing BitClocks...\n");
	int64 start_time = GetCurrentTimeMicros();

	for (int node_id = 0; node_id < graph.numNodes(); ++node_id) {
		std::vector<unsigned int>& cl = m_bitClocks[node_id];

		const std::vector<int>& pred = graph.nodePredecessors(node_id);
		for (size_t j = 0; j < pred.size(); ++j) {
			for (size_t i = 0; i < cl.size(); ++i) {
				cl[i] |= m_bitClocks[pred[j]][i];
			}
		}
		cl[node_id / 32] |= 1u << (node_id % 32);
	}
	printf("Computing BitClocks done... (%lld ms)\n", (GetCurrentTimeMicros() - start_time) / 1000);
}

bool BitClocks::areOrdered(int slice1, int slice2) const {
	if (slice1 < 0 ||
		slice2 < 0 ||
		slice1 >= static_cast<int>(m_bitClocks.size()) ||
		slice2 >= static_cast<int>(m_bitClocks.size())) return false;

	if (slice1 == slice2) return true;

	int a1 = (m_bitClocks[slice1][slice1 / 32] >> (slice1 % 32)) & 1;
	int a2 = (m_bitClocks[slice2][slice1 / 32] >> (slice1 % 32)) & 1;
	return a1 <= a2;
}


