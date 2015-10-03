#include "search.hpp"

#include <sys/_types/_clock_t.h>
#include <sys/_types/_size_t.h>
#include <algorithm>
#include <cassert>
#include <ctime>

#include "agent.hpp"
#include "main.hpp"

typedef unsigned long long visits_t;

// search options
static const visits_t MinVisitsBeforeExpansion = 1;
static const unsigned int MaxDistanceFromRoot = 100;
static size_t MaxSearchNodes;
static const int MaxBranchFactor = 100;

// contains information about a single "state"
class SearchNode {

public:

	SearchNode(bool is_chance_node) {
		m_visits = 0;
		m_chance_node = is_chance_node;
		m_mean = 0;
	}

	// determine the next action to play
	action_t selectAction(Agent &agent) const {
		// TODO: implement

	}

	// determine the expected reward from this node
	reward_t expectation(void) const {
		return m_mean;
	}

	// perform a sample run through this node and it's children,
	// returning the accumulated reward from this sample run
	reward_t sample(Agent &agent, unsigned int dfr) {
		reward_t reward;
		if (dfr == agent.m_horizon) {
			return 0;
		} else if (m_chance_node) {
			percept_t o = agent.genObsAndUpdate(); // TODO fix these up
			percept_t r = agent.genRewardAndUpdate();
			SearchNode decision_node = SearchNode(false);
			reward = r + decision_node.sample(agent, dfr + 1);
		} else if (m_visits == 0) {
			reward = playout(agent, agent.horizon() - dfr);
		} else {
			action_t a = selectAction(agent);
			SearchNode chance_node = SearchNode(true);
			reward = chance_node.sample(agent,dfr);
		}
		m_mean = (1.0/(m_visits+1))*(reward + m_visits * m_mean);
		m_visits++;

		return reward;
	}

	// number of times the search node has been visited
	visits_t visits(void) const {
		return m_visits;
	}

	double getValueEstimate(void) const {
		return m_mean;
	}

	double getNumVisits(void) const {
		return m_visits;
	}

	SearchNode getChild(int i) {
		return m_children[i];
	}
	// actions
	bool addChild(SearchNode child) {
		if (m_num_children >= MaxBranchFactor) {
			return false;
		}
		m_children[m_num_children] = *child;
		m_num_children++;

		return true;
	}

private:

	bool m_chance_node; // true if this node is a chance node, false otherwise
	double m_mean;      // the expected reward of this node
	visits_t m_visits;  // number of times the search node has been visited
	SearchNode *m_children[MaxBranchFactor]; // Array of children
	int m_num_children; // number of children

};

// simulate a path through a hypothetical future for the agent within it's
// internal model of the world, returning the accumulated reward.
static reward_t playout(Agent &agent, unsigned int playout_len) {
	return 0; // TODO: implement
}

action_t bestAction(Agent &agent, SearchNode node) {
	int N = agent.numActions();
	double max_val = 0;
	double val;
	unsigned int j = N + 1; // error handle

	// max action
	for (int i = 0; i < N; i++) {
		val = node.getChild(i).getValueEstimate();
		max_val = std::max(max_val, val);
		j = i;
	}
	assert(j <= N);
	return j;
}

// determine the best action by searching ahead using MCTS
extern action_t search(Agent &agent, int timeout) {
	// initialise search tree
	// TODO cache subtree between searches for efficiency
	// TODO make a copy of the agent model so we can update during search
	SearchNode root = SearchNode(false);
	clock_t startTime = clock();
	do {
		root.sample(agent, 0u);
	} while ((clock() - startTime) / (double) CLOCKS_PER_SEC < timeout);
	// TODO: implement

	return bestAction(agent, root);
}

