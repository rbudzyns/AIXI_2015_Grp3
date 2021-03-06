#include "search.hpp"

#include <cassert>
#include <cmath>
#include <ctime>
#include <iostream>
#include <utility>
#include <vector>

#include "util.hpp"

// search options
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

DecisionNode::DecisionNode(obsrew_t obsrew) :
        SearchNode() {
    m_obsrew = obsrew;
}

DecisionNode::~DecisionNode() {
    for (chance_map_t::iterator i = m_children.begin(); i != m_children.end();
            i++) {
        delete i->second;
    }
    m_children.clear();
}

// print method for debugging purposes
void DecisionNode::print() const {
    std::cout << "Node: (" << m_obsrew.first << "," << m_obsrew.second << ")"
            << std::endl;
    std::cout << "    T(h): " << m_visits << std::endl;
    std::cout << "    Vhat(h): " << m_mean << std::endl;
    std::cout << "    Children: " << m_children.size() << std::endl;
}

// getter method for a decision node's observation/reward
obsrew_t DecisionNode::obsRew(void) const {
    return m_obsrew;
}

// add a new child chance node
bool DecisionNode::addChild(ChanceNode* child) {
    if (m_children.size() >= MaxBranchFactor) {
        return false;
    }
    std::pair<action_t, ChanceNode*> p = std::make_pair(child->action(), child);
    m_children.insert(p);

    return true;
}

// getter method for a decision node's child corresponding to a given action
ChanceNode * DecisionNode::getChild(action_t action) {
    return m_children.count(action) ? m_children[action] : 0;
}

// count the number of nodes contained with the subtree starting at
// the decision node
int DecisionNode::getDecisionNodeInfo(void) {
    int n_nodes = 0;
    for (auto it = m_children.begin(); it != m_children.end(); ++it) {
        n_nodes += (it->second)->getChanceNodeInfo() + 1;
    }
    return n_nodes;
}

// perform a sample run through this node and it's children,
// returning the accumulated reward from this sample run
reward_t DecisionNode::sample(Agent &agent, unsigned int dfr) {
    reward_t reward;
    if (dfr == agent.horizon()) { // horizon has been reached
        return 0;
    } else if (m_visits == 0) {
        reward = playout(agent, agent.horizon() - dfr);
    } else {
        action_t action = selectAction(agent);
        agent.modelUpdate(action);
        reward = m_children[action]->sample(agent, dfr);
    }
    m_mean = (1.0 / (m_visits + 1)) * (reward + m_visits * m_mean);
    m_visits++;

    return reward;
}

// determine the next action to play
action_t DecisionNode::selectAction(Agent &agent) {
    action_t a;
    if (m_children.size() != agent.numActions()) { // then U != {}
        std::vector<action_t> U;
        int N = agent.numActions() - m_children.size();

        if (m_children.size() != 0) {
            for (action_t i = 0; i < agent.numActions(); i++) {
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
        // U == {}
        double max_val = 0;
        double val;

        for (action_t action = 0; action < m_children.size(); action++) {
            ChanceNode* child = m_children[action];
            double normalization = agent.horizon()
                    * (agent.maxReward() - agent.minReward()); // m(\beta - \alpha)
            double Vha = child->expectation(); // \hat{V}(ha)
            val = Vha / normalization
                    + agent.UCBWeight()
                            * sqrt(
                                    (double) log2((double) m_visits)
                                            / child->visits()); // eqn. 14 (Veness)
            if (val > max_val) {
                max_val = val;
                a = action;
            }
        }
        return a;
    }
}

// prune all child chance nodes except the given action
void DecisionNode::pruneAllBut(action_t action) {
    auto it = m_children.begin();

    while (it != m_children.end()) {
        if ((it->second)->action() != action) {
            delete it->second;
            it = m_children.erase(it);
        } else {
            it++;
        }
    }
}

// return the best action for a decision node
action_t DecisionNode::bestAction(Agent & agent) const {
    if (m_children.size() > 0) {
        reward_t max_val = 0;
        action_t a = 1;
        for (auto it = m_children.begin(); it != m_children.end(); ++it) {
            if ((it->second)->expectation() > max_val) {
                a = it->first;
                max_val = (it->second)->expectation();
            }
        }
        return a;
    } else {
        std::cout << "Warning: generating random action in bestAction."
                << std::endl;
        return agent.genRandomAction();
    }
}

ChanceNode::ChanceNode(action_t action) :
        SearchNode() {
    m_action = action;
}

ChanceNode::~ChanceNode() {
    for (decision_map_t::iterator i = m_children.begin(); i != m_children.end();
            i++) {
        delete i->second;
    }
    m_children.clear();
}

// getter method for the action corresponding to a chance node
action_t ChanceNode::action(void) const {
    return m_action;
}

// add a new child node
bool ChanceNode::addChild(DecisionNode* child) {
    if (m_children.size() >= MaxBranchFactor) {
        return false;
    }
    std::pair<obsrew_t, DecisionNode*> p;
    p = std::make_pair(child->obsRew(), child);
    m_children.insert(p);

    return true;
}

// prune all child decision nodes except the given observation/reward
void ChanceNode::pruneAllBut(obsrew_t obsrew) {
    auto it = m_children.begin();
    while (it != m_children.end()) {
        if ((it->second)->obsRew() != obsrew) {
            delete it->second;
            it = m_children.erase(it);
        } else {
            it++;
        }
    }
}

// count the number of nodes contained with the subtree starting at
// the chance node
int ChanceNode::getChanceNodeInfo(void) {
    int n_nodes = 0;
    for (auto it = m_children.begin(); it != m_children.end(); ++it) {
        n_nodes += (it->second)->getDecisionNodeInfo() + 1;
    }
    return n_nodes;
}

// getter method for a chance node's child corresponding to a given observation/
// reward
DecisionNode * ChanceNode::getChild(obsrew_t o_r) {
    return m_children.count(o_r) ? m_children[o_r] : 0;
}

// perform a sample run through this node and it's children,
// returning the accumulated reward from this sample run
reward_t ChanceNode::sample(Agent &agent, unsigned int dfr) {
    reward_t reward;
    if (dfr == agent.horizon()) { // horizon has been reached
        return 0;
    } else {
        percept_t* percept = agent.genPerceptAndUpdate();
        obsrew_t o_r = std::make_pair(percept[0], percept[1]);
        bool found = m_children.count(o_r);

        if (!found) {
            DecisionNode* decision_node = new DecisionNode(o_r);
            found = addChild(decision_node);
            // if we have breached MaxBranchFactor, uniformly choose an existing child DecisionNode
            if (!found) {
                auto random_it = std::next(std::begin(m_children),
                        randRange(0, m_children.size()));
                o_r = random_it->first;
            }
        }

        reward = percept[1] + m_children[o_r]->sample(agent, dfr + 1);
        delete[] percept;
    }
    m_mean = (1.0 / (m_visits + 1)) * (reward + m_visits * m_mean);
    m_visits++;

    return reward;
}

// simulate a path through a hypothetical future for the agent within its
// internal model of the world, returning the accumulated reward.
reward_t playout(Agent &agent, unsigned int playout_len) {
    reward_t reward = 0;
    for (int i = 1; i <= int(playout_len); i++) {
        action_t a = agent.genRandomAction();
        agent.modelUpdate(a);
        percept_t* percept = agent.genPerceptAndUpdate();
        reward += percept[1];
        delete[] percept;
    }

    return reward;
}

// determine the best action by searching ahead using MCTS
extern action_t search(Agent &agent) {
//	obsrew_t o_r = std::make_pair(NULL, NULL);
//	DecisionNode root = DecisionNode(o_r);
    clock_t startTime = clock();
    clock_t endTime = clock();
    int iter = 0;
    do {
        ModelUndo mu = ModelUndo(agent);

        (agent.searchTree())->sample(agent, 0u);
        //root.sample(agent, 0u);
        agent.modelRevert(mu);

        endTime = clock();
        iter++;
    } while ((endTime - startTime) / (double) CLOCKS_PER_SEC < agent.timeout());

    action_t action = (agent.searchTree())->bestAction(agent);
//action_t action = root.bestAction(agent);

    return action;
}
