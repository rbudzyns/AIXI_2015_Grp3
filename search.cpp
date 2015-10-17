#include "search.hpp"

#include <stddef.h>
#include <sys/time.h>
#include <cassert>
#include <cmath>
#include <ctime>
#include <iostream>
#include <iterator>
#include <vector>

#include "agent.hpp"
#include "main.hpp"
#include "util.hpp"

// search options
static const visits_t MinVisitsBeforeExpansion = 1;
static const unsigned int MaxDistanceFromRoot = 100;
static size_t MaxSearchNodes;
static const int MaxBranchFactor = 100;

// constructor
SearchNode::SearchNode(void) {
	m_visits = 0llu;
	m_mean = 0;
}

// determine the expected reward from this node
reward_t SearchNode::expectation(void) const {
	return m_mean;
}

// number of times the search node has been visited
visits_t SearchNode::visits(void) const {
	return m_visits;
}

// constructor
DecisionNode::DecisionNode(obsrew_t obsrew) :
		SearchNode() {
	m_obsrew = obsrew;
	//m_children = std::unordered_map<action_t,ChanceNode*>;
}

// print method for debugging purposes
void DecisionNode::print() const {
	std::cout << "Node state:" << std::endl;
	std::cout << "    T(h): " << m_visits << std::endl;
	std::cout << "    Vhat(h): " << m_mean << std::endl;
	std::cout << "    Children: " << m_children.size() << std::endl;
}

obsrew_t DecisionNode::getObsRew(void) const {
	return m_obsrew;
}

// add a new child node
bool DecisionNode::addChild(ChanceNode* child) {
	if (m_children.size() >= MaxBranchFactor) {
		return false;
	}
	std::pair<action_t, ChanceNode*> p = std::make_pair(child->getAction(),
			child);
	m_children.insert(p);

	return true;
}

// perform a sample run through this node and it's children,
// returning the accumulated reward from this sample run
reward_t DecisionNode::sample(Agent &agent, unsigned int dfr) {
	reward_t reward;
	if (dfr == MaxDistanceFromRoot) { // horizon has been reached
		return 0;
	} else if (m_visits == 0) {
		//std::cout << "Sample: Child node: T(n) = 0" << std::endl;
		reward = playout(agent, agent.horizon() - dfr);
	} else {
		//std::cout << "Sample: Child node: T(n) = " << m_visits << std::endl;
		action_t action = selectAction(agent);
		agent.modelUpdate(action);
		//std::cout << "Sample: after selectAction" << std::endl;
		reward = m_children[action]->sample(agent, dfr);
	}
	m_mean = (1.0 / (m_visits + 1)) * (reward + m_visits * m_mean);
	m_visits++;

	//print(); // print the node state for debugging purposes

	return reward;
}

// determine the next action to play
action_t DecisionNode::selectAction(Agent &agent) {
	action_t a;
	if (m_children.size() != agent.numActions()) {
		//std::cout << "selectAction: if " << m_children.size() << std::endl;
		// then U != {}
		std::vector<action_t> U;
		int N = agent.numActions() - m_children.size();
		bool found;

		/*
		 * attempt refactor construction/modification of U set
		 for (action_t i = 0; i < agent.numActions(); i++) {
		 U.push_back(i);
		 }
		 for (int i = 0; i < m_children.size(); i++) {
		 action_t tmp = getChild(i)->getAction();
		 U.erase(int(tmp));
		 }
		 */
		if (m_children.size() != 0) {
			for (action_t i = 0; i < agent.numActions(); i++) {
				//std::cout << "selectAction: for " << i << std::endl;
//					found = false;
//					for (int j = 0; j < int(m_children.size()); j++) {
//						//std::cout << "selectAction: before if " << std::endl;
//						if (i == getChild(j)->getAction()) {
//							//std::cout << "selectAction: found " << std::endl;
//							found = true;
//							break;
//						}
//					}
				//std::cout << "selectAction: after for " << std::endl;
				bool found = m_children.count(i);
				if (!found) {
					U.push_back(i);
				}
			}
			assert(U.size() == N);
		} else {
			for (action_t i = 0; i < agent.numActions(); i++) {
				U.push_back(i);
			}
		}

		a = U[randRange(N)];
		ChanceNode* chance_node = new ChanceNode(a);
		addChild(chance_node);
		return a;
	} else {
		//std::cout << "selectAction: else " << m_children.size()
		//<< std::endl;
		// U == {}
		double max_val = 0;
		double val;

		for (action_t action = 0; action < m_children.size(); action++) {
			ChanceNode* child = m_children[action];
			double normalization = agent.horizon()
					* (agent.maxReward() - agent.minReward()); // m(\beta - \alpha)
			double Vha = child->expectation(); // \hat{V}(ha)
			// John: just a note to check with you re: C from 14 in Veness.
			val = Vha / normalization
					+ agent.UCBWeight()
							* sqrt(
									(double) log2((double) m_visits)
											/ child->visits()); // eqn. 14 (Veness)
			if (val > max_val) {
				//std::cout << "getAction in UCB" << std::endl;
				max_val = val;
				a = action;
			}
		}
		return a;
	}
}

// return the best action for a decision node
action_t DecisionNode::bestAction(Agent &agent) const {
	if (m_children.size() > 0) {
		reward_t max_val = 0;
		action_t a = 1;
		//std::cout << "BestAction" << std::endl;
		for (auto it = m_children.begin(); it != m_children.end(); ++it) {
			if ((it->second)->expectation() > max_val) {
				a = it->first;
			}
		}
		return a;
	} else {
		std::cout << "Generating random action in bestAction." << std::endl;
		return agent.genRandomAction();
	}
}

ChanceNode::ChanceNode(action_t action) :
		SearchNode() {
	m_action = action;
	//m_children = std::unordered_map<obsrew_t,DecisionNode*>();
}

action_t ChanceNode::getAction(void) const {
	return m_action;
}

// add a new child node
bool ChanceNode::addChild(DecisionNode* child) {
	if (m_children.size() >= MaxBranchFactor) {
		return false;
	}
	std::pair<obsrew_t, DecisionNode*> p = std::make_pair(child->getObsRew(),
			child);
	m_children.insert(p);
	//std::cout << "addChild: " << m_children.size() << std::endl;

	return true;
}

// perform a sample run through this node and it's children,
// returning the accumulated reward from this sample run
reward_t ChanceNode::sample(Agent &agent, unsigned int dfr) {
	reward_t reward;
	if (dfr == MaxDistanceFromRoot) { // horizon has been reached
		return 0;
	} else {
		percept_t* percept = agent.genPerceptAndUpdate();
		obsrew_t o_r = std::make_pair(percept[0], percept[1]);
		bool found = m_children.count(o_r);
		if (!found) {
			DecisionNode* decision_node = new DecisionNode(o_r);
			addChild(decision_node);
		}
		reward = percept[1] + m_children[o_r]->sample(agent, dfr + 1); // do we increment here or on line 91?
	}
	m_mean = (1.0 / (m_visits + 1)) * (reward + m_visits * m_mean);
	m_visits++;

	//print(); // print the node state for debugging purposes

	return reward;
}

// return a random action according to the agent's model for its own
// behavior.

// possibly to be removed...
action_t genModelledAction(Agent &agent) {
	double p = 0.0;
	double pr = rand01();
	for (action_t i = 0; i < agent.numActions(); i++) {
		//std::cout << "genModelledAction: before getPredictedActionProb" << i << std::endl;
		p += agent.getPredictedActionProb(i);
		//std::cout << "genModelledAction: after getPredictedActionProb" << std::endl;
		if (p > pr) {
			return i;
		}
	}
	return 0;
}

// possibly to be removed...
action_t rollout_policy(Agent &agent) {
	// return agent.genRandomAction();
	return genModelledAction(agent);
}

// simulate a path through a hypothetical future for the agent within its
// internal model of the world, returning the accumulated reward.
reward_t playout(Agent &agent, unsigned int playout_len) {
	//std::cout << "Playout:" << std::endl;
	reward_t reward = 0;
	for (int i = 1; i <= int(playout_len); i++) {
		//std::cout << "Playout: before rollout_policy" << std::endl;
		action_t a = agent.genRandomAction();
		//std::cout << "Playout: after rollout_policy" << std::endl;
		//std::cout << "Playout: before modelUpdate(" << a << ")" << std::endl;
		agent.modelUpdate(a);
		//std::cout << "Playout: after modelUpdate" << std::endl;
		percept_t* percept = agent.genPerceptAndUpdate();
		//std::cout << "Playout: after genPerceptAndUpdate" << std::endl;
		reward += percept[1];
	}
	//std::cout << "Playout: leaving" << std::endl;
	return reward;
}

// determine the best action by searching ahead using MCTS
extern action_t search(Agent &agent, double timeout) {
// initialise search tree
// TODO cache subtree between searches for efficiency
// TODO make a copy of the agent model so we can update during search
	//std::cout << "search: timeout value: " << timeout << std::endl;
	obsrew_t o_r = std::make_pair(NULL, NULL);
	DecisionNode root = DecisionNode(o_r);
	clock_t startTime = clock();
	clock_t endTime = clock();
	int iter = 0;
	do {
		//std::cout << "search: in main loop, iter = " << iter << std::endl;
		ModelUndo mu = ModelUndo(agent);
		root.sample(agent, 0u);
		agent.modelRevert(mu);
		//std::cout << "After FULL Model Revert++++++++++++++++++++" << std::endl;
    	//agent.getContextTree()->debugTree();

		endTime = clock();
		/*
		 std::cout << "search: time since start: "
		 << ((endTime - startTime) / (double) CLOCKS_PER_SEC)
		 << std::endl;
		 */
		iter++;
	} while ((endTime - startTime) / (double) CLOCKS_PER_SEC < timeout);
	//std::cout << "Done searching" << std::endl;
	return root.bestAction(agent);
}
