#ifndef __ENVIRONMENT_HPP__
#define __ENVIRONMENT_HPP__

#include "main.hpp"
#include <string>
#include <bitset>
#include <climits>
#include <iostream>
#include <algorithm>
#include <cassert>
#include <queue>
#include "util.hpp"

class Environment {

public:

	// Constructor: set up the initial environment percept
	// TODO: implement in inherited class

	// receives the agent's action and calculates the new environment percept
	virtual void performAction(action_t action) = 0; // TODO: implement in inherited class

	//reset the environment after episode
	virtual void envReset(void) { return; }

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
	
	//reset the environment
	virtual void envReset(void);
	
private:
	//structure to represent the nodes
	struct node{
		unsigned int percept;
		node *next[4];
	};
	
	std::string maze_conf;
	node *current_node;
	node *cheese_node;
	node *mouse_start;
};

//Extented Tiger environment.It maps the location of tiger using a integer variable tiger.
//If tiger is true then tiger is behind left door and false if tiger is behind right door.
//The gold pot is obviously behind the other door.
//Agent state is mapped using boolean variable sitting. 1 for standing, 0 for sitting.

/*actions: 			0 = stand
 * 					1 = listen
 * 					2 = open left door
 * 					3 = open right door
 *
 * observations:	0 = nothing known
 * 					1 = tiger behind right door
 * 					2 = tiger behind left door
 */
class ExtTiger : public Environment{
public:
	//setup initial environment for Cheese maze and sets up the inital percept for the agent.
	ExtTiger(options_t &options);
	
	//actions of the agent and set up percept based on action;
	virtual void performAction(action_t action);
	
	//Check if the environment is finished
	virtual bool isFinished(void) const;

	//reset the environment
	virtual void envReset(void);


private:
	double p; //probability of listening correctly
	bool tiger; //position of tiger
	bool standing; //player is standing
};

//Tic Tac Toe environment
class TicTacToe : public Environment
{
public:	
	//constructor to intialise the Tic Tac Toe environment and to give the initial percepts.
	TicTacToe(options_t & options);

	//actions of the agent
	virtual void performAction(action_t action);

	//reset the environemnt
	virtual void envReset(void);

	//check if the game is finished
	virtual bool isFinished() const;

private:
	int board[9];
	bool finished;
	int freeCells;

	percept_t calBoardVal()
	{
		std::bitset<18> boardVal;
		for (int i = 0; i < 9; i++)
		{
			if (board[i] == 0)
			{
				boardVal.set(i * 2, 0);
				boardVal.set((i * 2) + 1, 0);
			}
			else if (board[i] == 1)
			{
				boardVal.set(i * 2, 0);
				boardVal.set((i * 2) + 1, 1);
			}
			else if (board[i] == 2)
			{
				boardVal.set(i * 2, 1);
				boardVal.set((i * 2) + 1, 0);
			}
		}
		return boardVal.to_ulong() & INT_MAX;
	}

	void env_move()
	{
		int move = (int)(rand01() * freeCells);
		int count = 0;
		for (int i = 0; i < 9; i++)
		{
			if (board[i] == 0)
				if (count++ == move) {
					board[i] = 1;
					freeCells--;
					return;
				}
		}
	}

	int check_winner()
	{
			if (board[0] != 0)
			{
				if ((board[0] == board[1] && board[1] == board[2]) || (board[0] == board[3] && board[0] == board[6])
					|| (board[0] == board[4] && board[4] == board[8]))
					return board[0];
			}
			else if (board[1] != 0)
			{
				if (board[1] == board[4] && board[4] == board[7])
					return board[1];
			}
			else if (board[2] != 0)
			{
				if ((board[2] == board[5] && board[5] == board[8]) || (board[2] == board[4] && board[4] == board[6]))
					return board[2];
			}
			else if (board[3] != 0)
			{
				if (board[3] == board[4] && board[4] == board[5])
					return board[3];
			}
			else if (board[6] != 0)
			{
				if (board[6] == board[7] && board[7] == board[8])
					return board[6];
			}
		return 0;
	}
};

/*
Action
		0 : rock
		1 : paper
		2 : scissors
*/
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

/*Pacman environment
 *	0 : Wall
 *	1 : Empty Cell
 *	2 : Food Pellet
 *	3 : Power Pill
 */
class Pacman : public Environment
{
public:
	//constructor to initialise the environment and set the initial percepts for the agent
	Pacman(options_t &options);

	//actions of the agent
	virtual void performAction(action_t action);

	//check if the game is finished
	virtual bool isFinished() const;

	//reset the environment
	virtual void envReset(void);

private:
	struct cell
	{
		unsigned int wall;
		bool isFreeCell;
		int contents;
	};
	cell maze[21][19];
	bool maze1[21][19];
	struct pos
	{
		int x;
		int y;
		bool state;
	};

	struct node
	{
		pos cell;
		int h_value;
		int g_value;
		node* parent;
	};

	bool pacmanPowered;

	int ghost_follow_time;

	int ghost_timer[4];

	pos ghost[4];
	pos pacman;
	bool finished;

	int power_pill_counter;
	int power_pill_time;


	//returns the bits for direction of ghost which are in line of sight
	unsigned int seeGhost()
	{
		std::bitset<4> chk;
		for(int i=0; i<4; i++)
		{
			chk.set(3-i, 0);
			if(i == 0 || i == 2)
			{
				if(ghost[0].y == pacman.y || ghost[1].y == pacman.y || ghost[2].y == pacman.y || ghost[3].y == pacman.y)
				{
					for(int j = pacman.x; i==0?j>0:j<21; i==0?j--:j++)
					{
						assert(0 < j && j < 21);
						if(!maze[j][pacman.y].isFreeCell)
							break;
						else if(ghost[0].x == j || ghost[1].x == j || ghost[2].x == j || ghost[3].x == j)
						{
							chk.set(3-i,1);
							break;
						}
					}
				}
			}
			else
			{
				if(ghost[0].x == pacman.x || ghost[1].x == pacman.x || ghost[2].x == pacman.x || ghost[3].x == pacman.x)
				{
					for(int j = pacman.y; i==1?j>0:j<19; i==1?j--:j++)
					{
						assert(0 <= j && j < 19);
						if(!maze[pacman.x][j].isFreeCell)
							break;
						else if(ghost[0].y == j || ghost[1].y == j || ghost[2].y == j || ghost[3].y == j)
						{
							chk.set(3-i, 1);
							break;
						}
					}
				}
			}

		}

		return chk.to_ulong() & INT_MAX;
	}

	unsigned int smellFood()
	{
		std::bitset<3> smell;
		//checking all cell at a man_dist of 1 to 4
		for(int man_dist = 1; man_dist<=4 && !smell.test(std::max(man_dist-2, 0)); man_dist++)
		{
			for (int i = std::max(pacman.x - man_dist, 0); i <= std::min(pacman.x + man_dist, 20) && !smell.test(std::max(man_dist-2, 0)); i++)
			{
				assert(pacman.x - man_dist <= i && 0 <= i && pacman.x+ man_dist >= i &&  i <= 20);
				//the range of j changes based on the value of i, such that the manhattan distance is bounded by man_dist.
				for (int j = std::max(pacman.y - abs(abs(pacman.x - i) - man_dist), 0); j <= std::min(pacman.y + abs(abs(pacman.x - i) - man_dist), 18) &&
				!smell.test(std::max(man_dist-2,0)); j++)
				{
					assert(0 <= j && pacman.y - man_dist <= j && pacman.y + man_dist >= j && j <= 18);
					assert(abs(pacman.x - i) + abs(pacman.y - j) <= man_dist);
					if (maze[i][j].contents == 1)
					{
						for(int k = std::max(man_dist-2, 0); k<=2; k++)
						{
							smell.set(k,1);
						}
					}
				}
			}
		}
		return smell.to_ulong() & INT_MAX;
	}

	unsigned int seeFood()
	{
		std::bitset<4> sight;
		for (int i = 0; i < 4; i++)
		{
			sight.set(3 - i, 0); //assuming there is no food in line of sight
			if (i == 1 || i == 3)
			{
				for (int j = pacman.y; i == 3 ? j > 0:j < 19; i == 3 ? j-- : j++)
				{
					if (0 > j || j >= 19)
					{
						std::cout << "Error in seeFood()" << std::endl;
						std::cout << "direction: " << i << " j = " << j << std::endl;
						std::cout << "pacman positon= " << pacman.x << pacman.y << std::endl;
						break;
					}
					if (!maze[pacman.x][j].isFreeCell)
						break;
					else if (maze[pacman.x][j].contents == 1)
					{
						sight.set(3 - i, 1);
						break;
					}
				}
			}
			else if (i == 0 || i == 2)
			{
				for (int j = pacman.x; i == 0 ? j > 0:j < 21; i == 0 ? j-- : j++)
				{
					if (0 > j || j >= 21)
					{
						std::cout << "Error in seeFood()" << std::endl;
						std::cout << "direction: " << i << " j = " << j << std::endl;
						std::cout << "pacman positon= " << pacman.x << pacman.y << std::endl;
						break;
					}
					if (!maze[j][pacman.y].isFreeCell)
						break;
					else if (maze[j][pacman.y].contents == 1)
					{
						sight.set(3 - i, 1);
						break;
					}
				}
			}
		}
		return sight.to_ulong() & INT_MAX;
	}

	bool isCaught()
	{

		for(int i = 0; i<4; i++)
		{
			if((pacman.x == ghost[i].x) && (pacman.y == ghost[i].y))
			{
				//ghost is active and pacman does not have power pill
				if(ghost[i].state && !pacman.state)
					return 1;
				else if(pacman.state) //pacman under effect of power pill
					ghost[i].state = 0;
			}
		}
		return 0;
	}


	//this method moves ghost when it is at a manhattan distance of less than 5 from the agent.
	void manMove(int ghostNo)
	{
		pos goal;
		pos move= ghost[ghostNo];
		node* min_node;
		node* open_list[400];
		node* closed_list[400];
		bool path_found = false;
		int open_list_size = 0;
		int closed_list_size = 0;
		if (ghost[ghostNo].state)
		{
			goal.x = pacman.x;
			goal.y = pacman.y;
		}
		else
		{
			goal.x = 8 + (int)(ghostNo / 2);
			goal.y = 9 + (int)(ghostNo % 2);
		}
		open_list[0] = new node;
		open_list[0]->cell = ghost[ghostNo];
		open_list[0]->parent = NULL;
		open_list[0]->g_value = 0;
		open_list[open_list_size++]->h_value = manhattan_dist(ghost[ghostNo], goal);
		while (open_list_size > 0)
		{
			min_node = open_list[0];
			int min_node_index = 0;
			//find node with best f_value
			for (int i = 1; i < open_list_size; i++)
			{
				if ((open_list[i]->h_value + open_list[i]->g_value) < (min_node->h_value + min_node->g_value))
				{
					min_node = open_list[i];
					min_node_index = i;
				}
			}
			//removing min_node from the open list
			for (int i = min_node_index; i < open_list_size - 1; i++)
			{
				open_list[i] = open_list[i + 1];
			}
			open_list_size--;

			//check if the node is the goal
			if (min_node->cell.x == goal.x && min_node->cell.y == goal.y)
			{
				path_found = true;
				break;
			}

			//adding node to closed list
			closed_list[closed_list_size++] = min_node;

			for (int j = 3; j >= 0; j--) //iterate through all possible neighbours
			{
				int xshift = (2 - j) % 2;
				assert(j == 0 ? xshift == 0 : true);
				assert(xshift == 0 || xshift == -1 || xshift == 1);
				int yshift = (j - 1) % 2;
				assert(yshift == 0 || yshift == -1 || yshift == 1);
				assert(xshift == 0 ? yshift != 0 : yshift == 0);
				//adding condition for loop back in row 9
				if(min_node->cell.x == 9 && min_node->cell.y == 0 && yshift == -1)
				{
					yshift = 18;
				}
				else if(min_node->cell.x == 9 && min_node->cell.y == 18 && yshift == 1)
				{
					yshift = -18;
				}
				if (maze[min_node->cell.x + xshift][min_node->cell.y + yshift].isFreeCell)
				{
					bool conflict = false;
					//check if this position is occupied by another ghost
					for (int i = 0; i < 4; i++)
					{
						if ((ghost[i].x == min_node->cell.x + xshift) && (ghost[i].y == min_node->cell.y + yshift) /*&& (!ghost[i].state || !ghost[ghostNo].state)*/)
						{
							conflict = true;
						}
					}
					if(!conflict)
					{
						pos possible_node;
						possible_node.x = min_node->cell.x + xshift;
						possible_node.y = min_node->cell.y + yshift;
						//check if node is already in open list
						bool check = true;
						for (int k = 0; k < open_list_size; k++)
						{
							if (open_list[k]->cell.x == possible_node.x  && open_list[k]->cell.y == possible_node.y)
							{
								check = false;
								if (open_list[k]->g_value < (min_node->g_value + 1))
								{
									open_list[k]->g_value = min_node->g_value + 1;
									open_list[k]->parent = min_node;
								}
							}
						}
						for (int k = 0; k < closed_list_size; k++)
						{
							if (closed_list[k]->cell.x == possible_node.x && closed_list[k]->cell.y == possible_node.y)
							{
								check = false;
							}
						}
						if(check)
						{
							open_list[open_list_size] = new node;
							open_list[open_list_size]->cell = possible_node;
							open_list[open_list_size]->g_value = min_node->g_value + 1;
							open_list[open_list_size]->h_value = manhattan_dist(possible_node, goal);
							open_list[open_list_size++]->parent = min_node;
						}
					}
				}
			}
		}
		//find the move in the path
		while (min_node->parent != NULL && path_found)
		{
			move = min_node->cell;
			min_node = min_node->parent;
		}
		for (int i = 0; i < open_list_size; i++)
		{
			delete open_list[i];
		}
		for(int i =0; i<closed_list_size; i++)
		{
			delete closed_list[i];
		}

		assert(maze[move.x][move.y].isFreeCell);

		ghost[ghostNo].x = move.x;
		ghost[ghostNo].y = move.y;
	}

	//this method return the path for a ghost in powerless state
	void eatenGhostMove(int ghostNo)
	{
		manMove(ghostNo);
		if ((ghost[ghostNo].x == 8 + (int)(ghostNo / 2)) && (ghost[ghostNo].y == 9 + (int)(ghostNo % 2)))
			ghost[ghostNo].state = 1;
		return;
	}

	int manhattan_dist(pos start, pos goal)
	{
		return (abs(start.x -goal.x) + abs(start.y - goal.y));
	}
};

#endif // __ENVIRONMENT_HPP__
