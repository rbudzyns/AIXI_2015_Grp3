#include "search.hpp"

//#include <sys/_types/_clock_t.h>
//#include <sys/_types/_size_t.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <ctime>
#include <vector>

#include "agent.hpp"

typedef unsigned long long visits_t;

// search options
static const visits_t MinVisitsBeforeExpansion = 1;
static const unsigned int MaxDistanceFromRoot = 100;
static size_t MaxSearchNodes;
static const int MaxBranchFactor = 100;

// contains information about a single "state"
class SearchNode {

public:

	// constructor
	SearchNode(bool is_chance_node) {
		m_visits = 0llu;
		m_chance_node = is_chance_node;
		m_mean = 0;
	}

	// print method for debugging purposes
    void print() const {
    	std::cout << "Node state:" << std::endl;
    	std::cout << "    Node type: " << (m_chance_node ? "chance" : "decision") << std::endl;
    	std::cout << "    T(h): " << m_visits << std::endl;
    	std::cout << "    Vhat(h): " << m_mean << std::endl;
    	std::cout << "    Children: " << m_children.size() << std::endl;
    }

	// determine the next action to play
	action_t selectAction(Agent &agent) const {
		// TODO: implement
		action_t a;
		// John: should we flick this if stmt around to be consistent with the psuedo code in Veness?
		if (m_children.size() == agent.numActions()) {
			// then U == {}
			double max_val = 0;
			double val;

			for (int i = 0; i < m_children.size(); i++) {
				SearchNode child = getChild(i);
				double normalization = agent.horizon() * (agent.maxReward() - agent.minReward()); // m(\beta - \alpha)
				double Vha = child.expectation(); // \hat{V}(ha)
				// John: just a note to check with you re: C from 14 in Veness.
				val = Vha/normalization+ sqrt((double) log2((double) m_visits)/child.visits()); // eqn. 14 (Veness)
				if (val > max_val) {
					max_val = val;
					a = i;
				}
			}
		} else { // U != {}
			a = 1; // TODO randRange(m_num_children, agent.numActions());
			// TODO create chance node
		}
		return a;
	}

	// determine the expected reward from this node
	reward_t expectation(void) const {
		return m_mean;
	}

	// perform a sample run through this node and it's children,
	// returning the accumulated reward from this sample run
	reward_t sample(Agent &agent, unsigned int dfr) {
		reward_t reward;
		if (dfr == MaxDistanceFromRoot) { // horizon has been reached
			return 0;
		} else if (m_chance_node) {
			percept_t o = 0; // Zero for now to get to compile. agent.genObsAndUpdate(); // TODO fix these up
			percept_t r = 0; // Zero for now to get to compile. agent.genRewardAndUpdate();
			SearchNode decision_node = new SearchNode(false);
			addChild(decision_node);
			reward = r + decision_node.sample(agent, dfr + 1); // do we increment here or on line 91?
		} else if (m_visits == 0) {
			std::cout << "Sample: Child node: T(n) = 0" << std::endl;
			reward = playout(agent, agent.horizon() - dfr);
		} else {
			std::cout << "Sample: Child node: T(n) > 0" << std::endl;
			action_t a = selectAction(agent);
			SearchNode chance_node = new SearchNode(true);
			addChild(chance_node);
			// reward = 0; // just for now... //
			reward = chance_node.sample(agent, dfr);
		}
		m_mean = (1.0/(m_visits+1))*(reward + m_visits * m_mean);
		m_visits++;

		print(); // print the node state for debugging purposes

		return reward;
	}

	// number of times the search node has been visited
	visits_t visits(void) const {
		return m_visits;
	}

	double getValueEstimate(void) const {
		return m_mean;
	}

	SearchNode getChild(int i) const {
		return m_children[i];
	}

	action_t getAction(void) const {
        assert(m_chance_node);
		return m_action;
	}

	// add a new child node
	bool addChild(SearchNode child) {
		if (m_children.size() >= MaxBranchFactor) {
			return false;
		}
		// m_children[m_num_children] = &child;
		m_children.push_back(child);
		// m_num_children++;

		return true;
	}

	// return the best action for a decision node
	action_t bestAction() const {
		assert(!m_chance_node);
		assert(m_children.size() > 0);
		reward_t max_val = 0;
		action_t a = 1;
		for (std::vector<SearchNode>::const_iterator it = m_children.begin(); it != m_children.end(); ++it) {
		    if ((*it).getValueEstimate() > max_val) {
			    a = (*it).getAction();
		    }
		}
		return a;
	}

private:

	bool m_chance_node; // true if this node is a chance node, false otherwise
	double m_mean;      // the expected reward of this node
	visits_t m_visits;  // number of times the search node has been visited
	action_t m_action;  // action associated with chance nodes
	std::vector<SearchNode> m_children; // list of child nodes

	// SearchNode *m_children[MaxBranchFactor]; // Array of children
	// unsigned int m_num_children; // number of children

};

// simulate a path through a hypothetical future for the agent within it's
// internal model of the world, returning the accumulated reward.
static reward_t playout(Agent &agent, unsigned int playout_len) {
	std::cout << "Playout:" << std::endl;
	reward_t reward = 0;
	for (int i = 1; i <= int(playout_len); i++) {
		int a = 1; // TODO Generate a from \Pi(h)
		int o = 1;
		int r = 1; // TODO Generate (o,r) from \rho(or|ha)
		reward += r;
	    // TODO h <-- haor
	}
	return reward;
}

//action_t bestAction(Agent &agent, SearchNode node) {
//	unsigned int N = agent.numActions();
//	double max_val = 0;
//	double val;
//	unsigned int j = N + 1; // error handle
//
//	// max action
//	for (int i = 0; i < int(N); i++) {
//		val = node.getChild(i).getValueEstimate();
//		max_val = std::max(max_val, val);
//		j = i;
//	}
//	assert(j <= N);
//	return j;
//}

// determine the best action by searching ahead using MCTS
extern action_t search(Agent &agent, double timeout) {
	// initialise search tree
	// TODO cache subtree between searches for efficiency
	// TODO make a copy of the agent model so we can update during search
	std::cout << "search: timeout value: " << timeout << std::endl;
	SearchNode root = SearchNode(false);
	clock_t startTime = clock();
	clock_t endTime = clock();
	do {
		root.sample(agent, 0u);
		endTime = clock();
		std::cout << "search: time since start: " << ((endTime - startTime) / (double) CLOCKS_PER_SEC) << std::endl;
	} while ((endTime - startTime) / (double) CLOCKS_PER_SEC < timeout);
	return root.bestAction();
}

