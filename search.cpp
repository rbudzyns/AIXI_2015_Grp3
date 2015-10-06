#include "search.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <ctime>
#include <vector>

#include "agent.hpp"
#include "util.hpp"

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
	SearchNode(bool is_chance_node, action_t action) {
		m_visits = 0llu;
		m_chance_node = is_chance_node;
		m_mean = 0;
		if (is_chance_node) {
			m_action = action;
		} else {
			m_action = NULL;
		}
	}

	// print method for debugging purposes
	void print() const {
		std::cout << "Node state:" << std::endl;
		std::cout << "    Node type: "
				<< (m_chance_node ? "chance" : "decision") << std::endl;
		std::cout << "    T(h): " << m_visits << std::endl;
		std::cout << "    Vhat(h): " << m_mean << std::endl;
		std::cout << "    Children: " << m_children.size() << std::endl;
	}

	// determine the next action to play
	action_t selectAction(Agent &agent) {
		action_t a;
		if (m_children.size() != agent.numActions()) {
			// then U != {}
			std::vector<action_t> U;
			int N = agent.numActions() - m_children.size();
			bool found;
			for (action_t i = 0; i < agent.numActions(); i++) {
				found = false;
				for (int j = 0; j < agent.numActions(); j++) {
					if (i == getChild(j).getAction()) {
						found = true;
						break;
					}
				}
				if (!found) {
					U.push_back(i);
				}
			}
			assert(U.size() == N);
			a = U[randRange(N)];
			SearchNode chance_node = SearchNode(true, a);
			addChild(chance_node);
			return a;
		} else {
			// U == {}
			double max_val = 0;
			double val;

			for (int i = 0; i < m_children.size(); i++) {
				SearchNode child = getChild(i);
				double normalization = agent.horizon()
						* (agent.maxReward() - agent.minReward()); // m(\beta - \alpha)
				double Vha = child.expectation(); // \hat{V}(ha)
				// John: just a note to check with you re: C from 14 in Veness.
				val = Vha / normalization
						+ sqrt(
								(double) log2((double) m_visits)
										/ child.visits()); // eqn. 14 (Veness)
				if (val > max_val) {
					max_val = val;
					a = child.getAction();
				}
			}
			return a;
		}
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
			SearchNode decision_node = SearchNode(false, NULL);
			addChild(decision_node);
			reward = r + decision_node.sample(agent, dfr + 1); // do we increment here or on line 91?
		} else if (m_visits == 0) {
			std::cout << "Sample: Child node: T(n) = 0" << std::endl;
			reward = playout(agent, agent.horizon() - dfr);
		} else {
			std::cout << "Sample: Child node: T(n) > 0" << std::endl;
			action_t a = selectAction(agent);

			// this is ugly, but necessary to keep the chance node creation inside selectAction, i think.
			for (int i = 0; i < m_children.size(); i++) {
				if (a == m_children[i].getAction()) {
					reward = m_children[i].sample(agent, dfr);
				}
			}
		}
		m_mean = (1.0 / (m_visits + 1)) * (reward + m_visits * m_mean);
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
		m_children.push_back(child);

		return true;
	}

	// return the best action for a decision node
	action_t bestAction() const {
		assert(!m_chance_node);
		assert(m_children.size() > 0);
		reward_t max_val = 0;
		action_t a = 1;
		for (std::vector<SearchNode>::const_iterator it = m_children.begin();
				it != m_children.end(); ++it) {
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
};

// simulate a path through a hypothetical future for the agent within its
// internal model of the world, returning the accumulated reward.
reward_t playout(Agent &agent, unsigned int playout_len) {
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

// determine the best action by searching ahead using MCTS
extern action_t search(Agent &agent, double timeout) {
	// initialise search tree
	// TODO cache subtree between searches for efficiency
	// TODO make a copy of the agent model so we can update during search
	std::cout << "search: timeout value: " << timeout << std::endl;
	SearchNode root = SearchNode(false, NULL);
	clock_t startTime = clock();
	clock_t endTime = clock();
	do {
		root.sample(agent, 0u);
		endTime = clock();
		std::cout << "search: time since start: "
				<< ((endTime - startTime) / (double) CLOCKS_PER_SEC)
				<< std::endl;
	} while ((endTime - startTime) / (double) CLOCKS_PER_SEC < timeout);
	return root.bestAction();
}

