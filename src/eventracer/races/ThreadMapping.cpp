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

#include "ThreadMapping.h"

#include "base.h"

#include <stdio.h>

#include <emmintrin.h>

ThreadMapping::ThreadMapping() : m_numThreads(0) {
}

void ThreadMapping::build(const SimpleDirectedGraph& graph) {
	printf("ThreadMapping: Computing threads...\n");
	int64 start_time = GetCurrentTimeMicros();
	// Greedy algorithm for mapping nodes to threads.
	m_nodeThread.assign(graph.numNodes(), -1);
	m_numThreads = 0;
	for (int i = 0; i < graph.numNodes(); ++i) {
		if (m_nodeThread[i] == -1 && !graph.isNodeDeleted(i)) {
			assignNodesToThread(graph, i, m_numThreads);
			++m_numThreads;
		}
	}
	printf("ThreadMapping: Found %d threads for %lld ms\n", m_numThreads, (GetCurrentTimeMicros() - start_time) / 1000);
}

void ThreadMapping::assignNodesToThread(const SimpleDirectedGraph& graph, int startNode, int threadId) {
	int nodeId = startNode;
	int num_nodes_in_chain = 0;
	for (;;) {
		if (m_nodeThread[nodeId] == -1) {
			m_nodeThread[nodeId] = threadId;
		}
		int nextNode = -1;
		const std::vector<int>& next = graph.nodeSuccessors(nodeId);
		for (size_t i = 0; i < next.size(); ++i) {
			if (m_nodeThread[next[i]] == -1) {
				nextNode = next[i];
				break;
			}
		}
		if (nextNode == -1 && next.size() > 0) {
			nextNode = next[0];
		}
		if (nextNode == -1) break;
		nodeId = nextNode;
		++num_nodes_in_chain;
		if (num_nodes_in_chain == 32766) break;
	}
/*
	// Note: For evaluating no chain cover.
	m_nodeThread[startNode] = threadId;
*/
}

namespace {
void maxVector(std::vector<short>* outv, const std::vector<short>& inv) {
/*	for (size_t i = 0; i < inv.size(); ++i) {
		//(*outv)[i] = std::max((*outv)[i], inv[i]);
		if (inv[i] > (*outv)[i]) {
			(*outv)[i] = inv[i];
		}
	}*/

	int invsize = inv.size();
	int is8 = invsize / 8;

	__m128i* a = (__m128i*) outv->data();
	__m128i* b = (__m128i*) inv.data();
	for (int i = 0; i < is8; ++i) {
		*a = _mm_max_epi16(*a, *b);
		++a;
		++b;
	}
	for (int i = is8 * 8; i < invsize; ++i) {
		if (inv[i] > (*outv)[i]) {
			(*outv)[i] = inv[i];
		}
	}
}
}  // namespace

void ThreadMapping::computeVectorClocks(const SimpleDirectedGraph& graph) {
	printf("ThreadMapping: Computing vector clocks...\n");
	int64 start_time = GetCurrentTimeMicros();
	m_vectorClocks.assign(graph.numNodes(), std::vector<short>());
	for (int node_id = 0; node_id < graph.numNodes(); ++node_id) {
		if (m_nodeThread[node_id] == -1) continue;
		m_vectorClocks[node_id].assign(m_numThreads, 0);
		const std::vector<int>& pred = graph.nodePredecessors(node_id);
		for (size_t j = 0; j < pred.size(); ++j) {
			maxVector(&m_vectorClocks[node_id], m_vectorClocks[pred[j]]);
		}
		m_vectorClocks[node_id][m_nodeThread[node_id]]++;
	}
	printf("ThreadMapping: Vector clocks done... (%lld ms)\n", (GetCurrentTimeMicros() - start_time) / 1000);
}

bool ThreadMapping::areOrdered(int slice1, int slice2) const {
	if (slice1 == slice2) return true;
	if (slice2 < slice1) return false;
	return m_vectorClocks[slice1][m_nodeThread[slice1]] <= m_vectorClocks[slice2][m_nodeThread[slice1]];
}
