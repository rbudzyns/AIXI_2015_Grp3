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
    SearchNode(action_t action) {
        m_chance_node = true;
        m_visits = 0llu;
        m_mean = 0;
        m_observation = NULL;
        m_reward = NULL;
        m_action = action;
    }

    SearchNode(percept_t observation, percept_t reward) {
        m_chance_node = false;
        m_visits = 0llu;
        m_mean = 0;
        m_observation = observation;
        m_reward = reward;
        m_action = NULL;
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
            std::vector < action_t > U;
            int N = agent.numActions() - m_children.size();
            bool found;
            for (action_t i = 0; i < agent.numActions(); i++) {
                found = false;
                for (int j = 0; j < int(agent.numActions()); j++) {
                    if (i == getChild(j)->getAction()) {
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
            SearchNode* chance_node = new SearchNode(a);
            addChild(chance_node);
            return a;
        } else {
            // U == {}
            double max_val = 0;
            double val;

            for (int i = 0; i < m_children.size(); i++) {
                SearchNode* child = getChild(i);
                double normalization = agent.horizon()
                        * (agent.maxReward() - agent.minReward()); // m(\beta - \alpha)
                double Vha = child->expectation(); // \hat{V}(ha)
                // John: just a note to check with you re: C from 14 in Veness.
                val = Vha / normalization
                        + sqrt(
                                (double) log2((double) m_visits)
                                        / child->visits()); // eqn. 14 (Veness)
                if (val > max_val) {
                    max_val = val;
                    a = child->getAction();
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
            percept_t* percept = agent.genPerceptAndUpdate();
            SearchNode* decision_node = childWithObsRew(percept[0], percept[1]);
            if (decision_node == NULL) {
                decision_node = new SearchNode(percept[0], percept[1]);
                addChild(decision_node);
            }
            reward = percept[1] + decision_node->sample(agent, dfr + 1); // do we increment here or on line 91?
        } else if (m_visits == 0) {
            std::cout << "Sample: Child node: T(n) = 0" << std::endl;
            reward = playout(agent, agent.horizon() - dfr);
        } else {
            std::cout << "Sample: Child node: T(n) > 0" << std::endl;
            action_t a = selectAction(agent);

            // this is ugly, but necessary to keep the chance node creation inside selectAction, i think.
            for (int i = 0; i < m_children.size(); i++) {
                if (a == m_children[i]->getAction()) {
                    reward = m_children[i]->sample(agent, dfr);
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

    SearchNode* getChild(int i) const {
        return m_children[i];
    }

    action_t getAction(void) const {
        assert(m_chance_node);
        return m_action;
    }

    percept_t getObs(void) const {
        assert(!m_chance_node);
        return m_observation;
    }

    percept_t getRew(void) const {
        assert(!m_chance_node);
        return m_reward;
    }

    // returns true if this chance node has a child decision node
    // which resulted from (obs, rew)
    SearchNode* childWithObsRew(percept_t obs, percept_t rew) {
        for (int i = 0; i < int(m_children.size()); i++) {
            if (m_children[i]->getObs() == obs
                    && m_children[i]->getRew() == rew) {
                return m_children[i];
            }
        }
        return NULL;
    }

    // add a new child node
    bool addChild(SearchNode* child) {
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
        for (std::vector<SearchNode*>::const_iterator it = m_children.begin();
                it != m_children.end(); ++it) {
            if ((*it)->getValueEstimate() > max_val) {
                a = (*it)->getAction();
            }
        }
        return a;
    }

private:

    bool m_chance_node; // true if this node is a chance node, false otherwise
    double m_mean;      // the expected reward of this node
    visits_t m_visits;  // number of times the search node has been visited
    action_t m_action;  // action associated with chance nodes
    percept_t m_observation; // observation associated with decision nodes
    percept_t m_reward; // reward associated with decision nodes
    std::vector<SearchNode*> m_children; // list of child nodes
};

// return a random action according to the agent's model for its own
// behavior.

action_t genModelledAction(Agent &agent) {
    double p = 0.0;
    double pr = rand01();
    for (action_t i = 0; i < agent.numActions(); i++) {
        std::cout << "genModelledAction: before getPredictedActionProb" << i << std::endl;
        p += agent.getPredictedActionProb(i);
        std::cout << "genModelledAction: after getPredictedActionProb" << std::endl;
        if (p > pr) {
            return i;
        }
    }
    return 0;
}

action_t rollout_policy(Agent &agent) {
	// return agent.genRandomAction();
	return genModelledAction(agent);
}

// simulate a path through a hypothetical future for the agent within its
// internal model of the world, returning the accumulated reward.
reward_t playout(Agent &agent, unsigned int playout_len) {
	std::cout << "Playout:" << std::endl;
	reward_t reward = 0;
	for (int i = 1; i <= int(playout_len); i++) {
		std::cout << "Playout: before rollout_policy" << std::endl;
		action_t a = rollout_policy(agent);
		std::cout << "Playout: after rollout_policy" << std::endl;
		std::cout << "Playout: before modelUpdate(" << a << ")" << std::endl;
		agent.modelUpdate(a);
		std::cout << "Playout: after modelUpdate" << std::endl;
		percept_t* percept = agent.genPerceptAndUpdate();
		std::cout << "Playout: after genPerceptAndUpdate" << std::endl;
		reward += percept[1];
	}
	std::cout << "Playout: leaving" << std::endl;
	return reward;
}

// determine the best action by searching ahead using MCTS
extern action_t search(Agent &agent, double timeout) {
// initialise search tree
// TODO cache subtree between searches for efficiency
// TODO make a copy of the agent model so we can update during search
    std::cout << "search: timeout value: " << timeout << std::endl;
    SearchNode root = SearchNode(NULL, NULL);
    clock_t startTime = clock();
    clock_t endTime = clock();
    do {
        ModelUndo mu = ModelUndo(agent);
        root.sample(agent, 0u);
        agent.modelRevert(mu);
        endTime = clock();
        std::cout << "search: time since start: "
                << ((endTime - startTime) / (double) CLOCKS_PER_SEC)
                << std::endl;
    } while ((endTime - startTime) / (double) CLOCKS_PER_SEC < timeout);
    return root.bestAction();
}
