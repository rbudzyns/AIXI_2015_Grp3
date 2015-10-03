#ifndef __SEARCH_HPP__
#define __SEARCH_HPP__

#include "main.hpp"

class Agent;

// determine the best action by searching ahead
extern action_t search(Agent &agent, int timeout);

static reward_t playout(Agent &agent, unsigned int playout_len);

#endif // __SEARCH_HPP__
