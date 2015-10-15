#include "agent.hpp"
#include <cassert>

#include "predict.hpp"
#include "search.hpp"
#include "util.hpp"

// construct a learning agent from the command line arguments
Agent::Agent(options_t & options) {
    std::string s;

    strExtract(options["agent-actions"], m_actions);
    strExtract(options["agent-horizon"], m_horizon);
    strExtract(options["observation-bits"], m_obs_bits);
    strExtract(options["ct-depth"], m_max_tree_depth);
    strExtract<unsigned int>(options["reward-bits"], m_rew_bits);

    // calculate the number of bits needed to represent the action
    for (unsigned int i = 1, c = 1; i < m_actions; i *= 2, c++) {
        m_actions_bits = c;
    }

    // calculate the number of bits needed to represent the action
    for (unsigned int i = 1, c = 1; i < m_actions; i *= 2, c++) {
        m_actions_bits = c;
    }
    
    m_ct = new ContextTree(strExtract<unsigned int>(options["ct-depth"]));
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

// the total accumulated reward across an agents lifespan
reward_t Agent::reward(void) const {
    return m_total_reward;
}

// the average reward received by the agent at each time step
reward_t Agent::averageReward(void) const {
    return lifetime() > 0 ? reward() / reward_t(lifetime()) : 0.0;
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

size_t Agent::maxTreeDepth(void) {
    return m_max_tree_depth;
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
action_t Agent::genRandomAction(void) const {
    return randRange(m_actions);
}

// generate a percept distributed according
// to our history statistics
percept_t* Agent::genPercept(void) const {
    percept_t *percept = new percept_t[2];
    symbol_list_t symbol_list;

    m_ct->genRandomSymbols(symbol_list, m_obs_bits + m_rew_bits);
    m_ct->revertHistory(m_obs_bits + m_rew_bits);

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

    m_ct->genRandomSymbolsAndUpdate(symbol_list, m_obs_bits + m_rew_bits);
    //std::cout << __FILE__ << " " <<  __LINE__ << " " << __func__ << " Before Decoding" << std::endl;

    percept[0] = decode(symbol_list, m_obs_bits);
    for (int i = 0; i < m_rew_bits; i++) {
        symbol_list[i] = symbol_list[i + m_obs_bits];
    }
    percept[1] = decode(symbol_list, m_rew_bits);
    //std::cout << __FILE__ << " " <<  __LINE__ << " " << __func__ << " After Decoding" << std::endl;

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

    m_ct->update(percept);
    m_ct->updateHistory(percept);

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
    m_ct->update(action_syms);
    m_ct->updateHistory(action_syms);

    m_time_cycle++;
    m_last_update_percept = false;
}

// revert the agent's internal model of the world
// to that of a previous time cycle, false on failure
bool Agent::modelRevert(const ModelUndo &mu) {
    int size_of_ora_pairs = (m_time_cycle - mu.lifetime())*(m_obs_bits + m_rew_bits + m_actions_bits);
    
    std::cout << "Size of ora = " << size_of_ora_pairs << std::endl;

    for(int i = 0; i < size_of_ora_pairs; i++) {
        m_ct->revert();
        m_ct->revertHistory(m_ct->historySize()-1);
    }
    
    m_time_cycle = mu.lifetime();
    m_total_reward = mu.reward();
    return true;
}

void Agent::reset(void) {
    m_ct->clear();

    m_time_cycle = 0;
    m_total_reward = 0.0;
}

void Agent::newEpisode(void) {
    m_time_cycle = 0;
    m_total_reward = 0.0;
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

    for (int i = 0; i < m_rew_bits; i++) {
        log_probability += m_ct->getLogProbNextSymbolGivenHWithUpdate(
                1 & observation);
        observation /= 2;
    }

    for (int i = 0; i < m_rew_bits; i++) {
        log_probability += m_ct->getLogProbNextSymbolGivenHWithUpdate(
                1 & reward);
        reward /= 2;
    }

    return pow(2, log_probability);
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

ContextTree* Agent::getContextTree() {
    return m_ct;
}
// used to revert an agent to a previous state
ModelUndo::ModelUndo(const Agent &agent) {

    m_lifetime = agent.lifetime();
    m_reward = agent.reward();
    m_history_size = agent.historySize();

}

