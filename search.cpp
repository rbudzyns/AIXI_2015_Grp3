#include "search.hpp"

#include "agent.hpp"
#include <ctime>
#include <cassert>
#include <algorithm>


typedef unsigned long long visits_t;


// search options
static const visits_t     MinVisitsBeforeExpansion = 1;
static const unsigned int MaxDistanceFromRoot  = 100;
static size_t             MaxSearchNodes;

// contains information about a single "state"
class SearchNode {

public:

	SearchNode(bool is_chance_node) {
		m_visits = 0;
		m_chance_node = is_chance_node;
		m_mean = 0;
	}

	// determine the next action to play
	action_t selectAction(Agent &agent) const; // TODO: implement

	// determine the expected reward from this node
	reward_t expectation(void) const { return m_mean; }

	// perform a sample run through this node and it's children,
	// returning the accumulated reward from this sample run
	reward_t sample(Agent &agent, unsigned int dfr); // TODO: implement

	// number of times the search node has been visited
	visits_t visits(void) const { return m_visits; }

	double getValueEstimate(void) const {
		return m_mean;
	}

	double getNumVisits(void) const {
		return m_visits;
	}

	SearchNode getChild(int i) {
		return m_children[i];
	}

private:

	bool m_chance_node; // true if this node is a chance node, false otherwise
	double m_mean;      // the expected reward of this node
	visits_t m_visits;  // number of times the search node has been visited
	SearchNode *m_children[100]; // Array of children

	// TODO: decide how to reference child nodes
	//  e.g. a fixed-size array
};

// simulate a path through a hypothetical future for the agent within it's
// internal model of the world, returning the accumulated reward.
static reward_t playout(Agent &agent, unsigned int playout_len) {
	return 0; // TODO: implement
}

// determine the best action by searching ahead using MCTS
extern action_t search(Agent &agent,int timeout) {
	// initialise search tree
	// TODO cache subtree between searches for efficiency

	SearchNode root = SearchNode(false);
	clock_t startTime = clock();
	do {
		root.sample(agent, 0u);
	} while ((clock() - startTime) / (double) CLOCKS_PER_SEC  < timeout);
	int N = agent.numActions();
	double max_val = 0;
	double val;
	unsigned int j = N +1; // error handle
	for (int i = 0; i < N; i++) {

		val = root.getChild(i).getValueEstimate();
		max_val = std::max(max_val,val);
		j = i;
	}
	assert(j <= N);
	return j; // TODO: implement
}

