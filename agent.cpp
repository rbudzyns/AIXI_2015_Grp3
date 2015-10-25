#include "agent.hpp"
#include <cassert>
#include <cmath>

#include "predict.hpp"
#include "search.hpp"
#include "util.hpp"

// construct a learning agent from the command line arguments

void Agent::setOptions(options_t & options) {
    std::string s;

    strExtract(options["agent-actions"], m_actions);
    strExtract(options["agent-horizon"], m_horizon);
    strExtract(options["observation-bits"], m_obs_bits);
    strExtract(options["ct-depth"], m_max_tree_depth);
    strExtract<unsigned int>(options["reward-bits"], m_rew_bits);
    strExtract(options["timeout"], m_timeout);
    strExtract(options["UCB-weight"], m_UCBWeight);

    for (unsigned int i = 1, c = 1; i < m_actions; i *= 2, c++) {
        m_actions_bits = c;
    }
    m_time_cycle = 0;
    m_total_reward = 0.0;
    obsrew_t o_r = std::make_pair(NULL, NULL);
    m_st = new DecisionNode(o_r);
}

Agent::Agent(options_t & options) {
    std::string s;

    strExtract(options["agent-actions"], m_actions);
    strExtract(options["agent-horizon"], m_horizon);
    strExtract(options["observation-bits"], m_obs_bits);
    strExtract(options["ct-depth"], m_max_tree_depth);
    strExtract<unsigned int>(options["reward-bits"], m_rew_bits);
    strExtract(options["timeout"], m_timeout);
    strExtract(options["UCB-weight"], m_UCBWeight);

    // calculate the number of bits needed to represent the action
    for (unsigned int i = 1, c = 1; i < m_actions; i *= 2, c++) {
        m_actions_bits = c;
    }

    m_ct = new ContextTree(strExtract<unsigned int>(options["ct-depth"]));

    // build a new uct
    obsrew_t o_r = std::make_pair(NULL, NULL);
    m_st = new DecisionNode(o_r);

    reset();

}

// destruct the agent and the corresponding context tree
Agent::~Agent(void) {
    if (m_ct)
        delete m_ct;
}

// current lifetime of the agent in cycles
lifetime_t Agent::lifetime(void) const {
    return m_time_cycle;
}

double Agent::UCBWeight(void) const {
    return m_UCBWeight;
}

// the total accumulated reward across an agent's lifespan
reward_t Agent::reward(void) const {
    return m_total_reward;
}

// the average reward received by the agent at each time step
reward_t Agent::averageReward(void) const {
    return lifetime() > 0 ? reward() / reward_t(lifetime() + 1) : 0.0;
}

// maximum reward in a single time instant
reward_t Agent::maxReward(void) const {
    return reward_t((1 << m_rew_bits) - 1);
}

// minimum reward in a single time instant
reward_t Agent::minReward(void) const {
    return 0.0;
}

// number of distinct actions
unsigned int Agent::numActions(void) const {
    return m_actions;
}

// Get the Context tree depth
size_t Agent::maxTreeDepth(void) {
    return m_max_tree_depth;
}

// Calculate the probability of next symbol
double Agent::getProbNextSymbol(void) {
    return pow(2, m_ct->getLogProbNextSymbolGivenH(1));
}

// the length of the stored history for an agent
size_t Agent::historySize(void) const {
    return m_ct->historySize();
}

// length of the search horizon used by the agent
size_t Agent::horizon(void) const {
    return m_horizon;
}

// generate an action uniformly at random
action_t Agent::genRandomAction(void) {

    return randRange(m_actions);
}

// Generate a percept distributed according
// to our history statistics
percept_t* Agent::genPercept(void) const {
    percept_t *percept = new percept_t[2];
    symbol_list_t symbol_list;

    // Generate the observation and reward block
    m_ct->genRandomSymbols(symbol_list, m_obs_bits + m_rew_bits);
    m_ct->revertHistory(m_obs_bits + m_rew_bits);

    // Decode the (observation, reward) percept from symbol list
    percept[0] = decode(symbol_list, m_obs_bits);
    for (int i = 0; i < m_rew_bits; i++) {
        symbol_list[i] = symbol_list[i + m_obs_bits];
    }
    percept[1] = decode(symbol_list, m_rew_bits);

    return percept;
}

// generate a percept distributed to our history statistics, and
// update our mixture environment model with it
percept_t* Agent::genPerceptAndUpdate(void) {
    percept_t* percept = new percept_t[2];
    symbol_list_t symbol_list(m_obs_bits + m_rew_bits);

    // Generate the observation and reward block and update the Context tree
    m_ct->genRandomSymbolsAndUpdate(symbol_list, m_obs_bits + m_rew_bits);

    // Decode the (observation, reward) percept from symbol list
    percept[0] = decode(symbol_list, m_obs_bits);
    for (int i = 0; i < m_rew_bits; i++) {
        symbol_list[i] = symbol_list[i + m_obs_bits];
    }
    percept[1] = decode(symbol_list, m_rew_bits);

    // Update other properties
    m_total_reward += percept[1];
    m_last_update_percept = true;
    return percept;
}

// Update the agent's internal model of the world after receiving a percept
void Agent::modelUpdate(percept_t observation, percept_t reward) {
    // Update internal model
    symbol_list_t percept;
    encodePercept(percept, observation, reward);

    if (m_ct->historySize() >= m_ct->depth()) {
        // Update the context tree with the percept
        m_ct->update(percept);
    } else {
        // Populate the history for initial context
        m_ct->updateHistory(percept);
    }

    // Update other properties
    m_total_reward += reward;
    m_last_update_percept = true;
}

// Update the agent's internal model of the world after performing an action
void Agent::modelUpdate(action_t action) {
    assert(isActionOk(action));
    assert(m_last_update_percept == true);

    // Update internal model
    symbol_list_t action_syms;
    encodeAction(action_syms, action);
    m_ct->updateHistory(action_syms);

    m_time_cycle++;
    m_last_update_percept = false;
}

// revert the agent's internal model of the world
// to that of a previous time cycle, false on failure
bool Agent::modelRevert(const ModelUndo &mu) {

    int n_cycles = m_time_cycle - mu.lifetime();

    // Revert the context tree to the restoration point
    for (int i = 0; i < n_cycles; i++) {
        for (int j = 0; j < m_obs_bits + m_rew_bits; j++) {
            // Revert the perpcept for each cycle
            m_ct->revert();
            m_ct->revertHistory(m_ct->historySize() - 1);
        }
        m_ct->revertHistory(m_ct->historySize() - m_actions_bits);

    }

    // Revert the time cycle and total reward
    m_time_cycle = mu.lifetime();
    m_total_reward = mu.reward();
    return true;
}

// Reset the agent
void Agent::reset(void) {
    m_ct->clear();

    m_time_cycle = 0;
    m_total_reward = 0.0;
}

// Start a new episode
void Agent::newEpisode(void) {
    m_time_cycle = 0;
    m_total_reward = 0.0;
    m_ct->resetHistory();
}

// Get the time out
double Agent::timeout(void) {
    return m_timeout;
}

// probability of selecting an action according to the
// agent's internal model of it's own behaviour
double Agent::getPredictedActionProb(action_t action) {
    double log_probability = 0.0;

    for (int i = 0; i < m_actions_bits; i++) {
        log_probability += m_ct->getLogProbNextSymbolGivenHWithUpdate(
                1 & action);
        action /= 2;
    }

    return pow(2, log_probability);
}

// get the agent's probability of receiving a particular percept
double Agent::perceptProbability(percept_t observation,
        percept_t reward) const {
    double log_probability = 0.0;

    for (int i = 0; i < m_obs_bits; i++) {
        // Calculate the log probability of seeing the observation bits
        log_probability += m_ct->getLogProbNextSymbolGivenHWithUpdate(
                1 & observation);
        observation /= 2;
    }

    for (int i = 0; i < m_rew_bits; i++) {
        // Calculate the log probability of seeing the reward bits
        log_probability += m_ct->getLogProbNextSymbolGivenHWithUpdate(
                1 & reward);
        reward /= 2;
    }

    return pow(2, log_probability);
}

// Return context tree
ContextTree * Agent::contextTree() {
    return m_ct;
}

// action sanity check
bool Agent::isActionOk(action_t action) const {
    return action < m_actions;
}

// reward sanity check
bool Agent::isRewardOk(reward_t reward) const {
    return reward >= minReward() && reward <= maxReward();
}

// Encodes an action as a list of symbols
void Agent::encodeAction(symbol_list_t &symlist, action_t action) const {
    symlist.clear();

    encode(symlist, action, m_actions_bits);
}

// Encodes a percept (observation, reward) as a list of symbols
void Agent::encodePercept(symbol_list_t &symlist, percept_t observation,
        percept_t reward) const {
    symlist.clear();

    encode(symlist, observation, m_obs_bits);
    encode(symlist, reward, m_rew_bits);
}

// Decodes the observation from a list of symbols
action_t Agent::decodeAction(const symbol_list_t &symlist) const {
    return decode(symlist, m_actions_bits);
}

// Decodes the reward from a list of symbols
percept_t Agent::decodeReward(const symbol_list_t &symlist) const {
    return decode(symlist, m_rew_bits);
}

// return the search tree
DecisionNode * Agent::searchTree() {
    return m_st;
}

// reset the search tree to a new root node
void Agent::searchTreeReset() {
    delete m_st;
    obsrew_t o_r = std::make_pair(NULL, NULL);
    m_st = new DecisionNode(o_r);
}

// prune the tree to the subtree of the root corresponding to
// the given action
void Agent::searchTreePrune(action_t action, obsrew_t obsrew) {
    ChanceNode * chance_node = m_st->getChild(action);
    if (chance_node != 0) {
        searchTree()->pruneAllBut(action);
        DecisionNode * new_root = chance_node->getChild(obsrew);
        if (new_root != 0) {
            chance_node->pruneAllBut(obsrew);
            m_st = new_root;
        }
    }
}

// used to revert an agent to a previous state
ModelUndo::ModelUndo(const Agent &agent) {
    m_lifetime = agent.lifetime();
    m_reward = agent.reward();
    m_history_size = agent.historySize();
}

