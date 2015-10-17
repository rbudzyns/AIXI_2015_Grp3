#ifndef __SEARCH_HPP__
#define __SEARCH_HPP__

#include <unordered_map>

#include "agent.hpp"
#include "main.hpp"

class or_hasher {

public:
	size_t operator()(const obsrew_t & p) const {
		return p.first * 100 + p.second * 10000;
	}
};
class ChanceNode;
class DecisionNode;

typedef std::unordered_map<obsrew_t, DecisionNode*, or_hasher> decision_map_t;
typedef std::unordered_map<action_t, ChanceNode*> chance_map_t;
typedef unsigned long long visits_t;

class Agent;

class SearchNode {

public:

	// constructor
	SearchNode(void);

	// determine the expected reward from this node
	reward_t expectation(void) const;

	// number of times the search node has been visited
	visits_t visits(void) const;

protected:

	double m_mean;      // the expected reward of this node
	visits_t m_visits;  // number of times the search node has been visited
};

class DecisionNode: SearchNode {

public:

	// constructor
	DecisionNode(obsrew_t obsrew);

	// print node data for debugging purposes
	void print() const;

	// get (observation,reward) label
	obsrew_t obsRew(void) const;

	// add a new child chance node
	bool addChild(ChanceNode* child);

	// perform a sample run through this node and its children,
	// returning the accumulated reward from this sample run
	reward_t sample(Agent &agent, unsigned int dfr);

	// determine the next action to play
	action_t selectAction(Agent &agent);

	// return the best action for a decision node
	action_t bestAction(Agent &agent) const;

private:
	obsrew_t m_obsrew; 		  // observation associated with decision nodes
	chance_map_t m_children;  // list of child nodes
};

class ChanceNode: public SearchNode {

public:

	// constructor
	ChanceNode(action_t action);

	// get action label
	action_t action(void) const;

	// add a new child node
	bool addChild(DecisionNode* child);

	// perform a sample run through this node and it's children,
	// returning the accumulated reward from this sample run
	reward_t sample(Agent &agent, unsigned int dfr);

private:
	action_t m_action;  // action associated with chance nodes
	decision_map_t m_children; // list of child nodes
};

// determine the best action by searching ahead
extern action_t search(Agent &agent, double timeout);

extern reward_t playout(Agent &agent, unsigned int playout_len);

#endif // __SEARCH_HPP__
