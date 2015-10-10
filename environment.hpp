#ifndef __ENVIRONMENT_HPP__
#define __ENVIRONMENT_HPP__

#include "main.hpp"
#include <string>
#include <bitset>

class Environment {

public:

	// Constructor: set up the initial environment percept
	// TODO: implement in inherited class

	// receives the agent's action and calculates the new environment percept
	virtual void performAction(action_t action) = 0; // TODO: implement in inherited class

	// returns true if the environment cannot interact with the agent anymore
	virtual bool isFinished(void) const { return false; } // TODO: implement in inherited class (if necessary)

	void getPercept(symbol_list_t &symbol_list);

	percept_t getObservation(void) const { return m_observation; }

	percept_t getReward(void) const { return m_reward; }

protected: // visible to inherited classes
	action_t m_last_action;  // the last action performed by the agent
	percept_t m_observation; // the current observation
	percept_t m_reward;      // the current reward

};


// An experiment involving flipping a biased coin and having the agent predict
// whether it will come up heads or tails. The agent receives a reward of 1
// for a correct guess and a reward of 0 for an incorrect guess.
class CoinFlip : public Environment {
public:

	// set up the initial environment percept
	CoinFlip(options_t &options);

	// receives the agent's action and calculates the new environment percept
	virtual void performAction(action_t action);

private:
	double p; // Probability of observing 1 (heads)
};


//Basic cheese maze environment. Environment contains the position of mouse and cheese
//along with the structure of the walls. The agent recieves a reward of 10 for finding
//the cheese, a penalty of -10 for bumping into a wall and a penalty of -1 for moving 
//into a free cell.
class CheeseMaze : public Environment {
public:
	
	//setup initial environment for Cheese maze and sets up the inital percept for the agent.
	CheeseMaze(options_t &options);
	
	//actions of the agent and set up percept based on action;
	virtual void performAction(action_t action);
	
	//Check with the environment if the agent has reached the goal;
	virtual bool isFinished(void) const;
	
private:
	//structure to represent the nodes
	struct node{
		unsigned int percept;
		node *next[4];
	};
	
	std::string maze_conf;
	node *current_node;
	node *cheese_node;
};

//Extented Tiger environment.It maps the location of tiger using a integer variable tiger.
//If tiger is 2 then tiger is behind left door and 1 if tiger is behind right door.
//The gold pot is obviously behind the other door.
//Agent state is mapped using boolean variable sitting. 1 for standing, 0 for sitting.
class ExtTiger : public Environment{
public:
	//setup initial environment for Cheese maze and sets up the inital percept for the agent.
	ExtTiger(options_t &options);
	
	//actions of the agent and set up percept based on action;
	virtual void performAction(action_t action);
	
private:
	double p; //probability of listening correctly
	unsigned int tiger; //position of tiger
	bool standing; //player is standing
};

class TicTacToe : public Environment
{
public:	
	//constructor to intialise the Tic Tac Toe environment and to give the initial percepts.
	TicTacToe(options_t & options);

	//actions of the agent
	virtual void performAction(action_t action);

protected:
	std::bitset<18> board;
};

class BRockPaperScissors : public Environment
{
public:
	//constructor to initialise the environment and the set the initial percepts for the agent;
	BRockPaperScissors(options_t &options);

	//actions of the agent
	virtual void performAction(action_t action);

private:
	unsigned int move;
};

#endif // __ENVIRONMENT_HPP__
