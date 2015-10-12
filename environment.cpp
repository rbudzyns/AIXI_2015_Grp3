#include "environment.hpp"

#include <cassert>
#include <stack>
#include <string>
#include <bitset>
#include <climits>

#include "util.hpp"


class next;
class next;
CoinFlip::CoinFlip(options_t &options) {
	// Determine the probability of the coin landing on heads
	p = 1.0;
	if (options.count("coin-flip-p") > 0) {
		strExtract(options["coin-flip-p"], p);
	}
	assert(0.0 <= p);
	assert(p <= 1.0);

	// Set up the initial observation
	m_observation = rand01() < p ? 1 : 0;
	m_reward = 0;
}

// Observes 1 (heads) with probability p and 0 (tails) with probability 1 - p.
// Observations are independent of the agent's actions. Gives a reward of 1 if
// the agent correctly predicts the next observation and 0 otherwise.
void CoinFlip::performAction(action_t action) {
	m_observation = rand01() < p ? 1 : 0;
	m_reward = action == m_observation ? 1 : 0;
}

/*
 * the maze is coded as a sequence of percepts of the free cells
 * in the config file.
 * This is a depth first sequence.
 * Each node would be represented by a structure which has 4
 * pointers to represent the 4 possible next nodes from there.
 * Pointer is NULL if there is a wall in any particular direction.
 * pointer index: 0-up, 1-right, 2-down, 3-left
 */
CheeseMaze::CheeseMaze(options_t &options)
{
	int mouse_pos=1;
	int cheese_pos=1;
	//setup the environment according to the config file
	if (options.count("maze-structure") > 0){
		strExtract(options["maze-structure"], maze_conf);
	}
	
	if (options.count("mouse-pos") > 0){
		strExtract(options["mouse-pos"],mouse_pos);
	}
	
	if (options.count("cheese-pos")>0){
		strExtract(options["cheese-pos"],cheese_pos);
	}
	
	//Create the nodes of the maze.
	int i=0;
	int count=1;
	char num[3];
	int j=0;
	unsigned int per;
	node *curr_node;
	//stack of pointers of a pointer to a node. 
	/*Basic depth first search algorithm involves pushing into a stack the 
	 * neighbouring nodes. Since we do not know the graph beforehand, 
	 * we can push the addresses of the pointers in the structures which need
	 * to have values assigned, which we can find out based on the value of
	 * percept.
	*/
	std::stack <node*> nodes;
	do
	{		
		if(maze_conf[i] == ',')
		{
			num[j] = '\0';
			std::istringstream(num)>>per;
			node *new_node = new node;
			new_node->percept = per;
			if(count == mouse_pos)
				current_node = new_node;
			if(count == cheese_pos)
				cheese_node = new_node;
			for(int k=0;k<4;k++)
				new_node->next[k] = NULL;
			//if stack is empty, meaning all edges will have to be connected
			if(nodes.empty())
			{
				if((new_node->percept & 8)==0)
					nodes.push(new_node);
				if((new_node->percept & 4)==0)
					nodes.push(new_node);
				if((new_node->percept & 2)==0)
					nodes.push(new_node);
				if((new_node->percept & 1)==0)
					nodes.push(new_node);
			}
			else
			{
				curr_node = nodes.top();
				nodes.pop();
				//connecting node is below the latest node
				if(((curr_node->percept & 8)==0) && (curr_node->next[0] == NULL))
				{
					new_node->next[2] = curr_node;
					curr_node->next[0] = new_node;
				}
				//connecting node is on the left of the latest node
				else if(((curr_node->percept & 4)==0) && (curr_node->next[1] == NULL))
				{
					new_node->next[3] = curr_node;
					curr_node->next[1] = new_node;
				}
				//connecting node is above the latest node
				else if(((curr_node->percept & 2)==0) && (curr_node->next[2] == NULL))
				{
					new_node->next[0] = curr_node;
					curr_node->next[2] = new_node;
				}
				else if(((curr_node->percept & 1)==0) && (curr_node->next[3] == NULL))
				{
					new_node->next[1] = curr_node;
					curr_node->next[3] = new_node;
				}
				
				//push the node into the stack for every free node which is still unassigned.
				if(((new_node->percept & 8)==0) && (new_node->next[0] == NULL))
					nodes.push(new_node);
				if(((new_node->percept & 4)==0) && (new_node->next[1] == NULL))
					nodes.push(new_node);
				if(((new_node->percept & 2)==0) && (new_node->next[2] == NULL))
					nodes.push(new_node);
				if(((new_node->percept & 1)==0) && (new_node->next[3] == NULL))
					nodes.push(new_node);
			}
			j=0;
			count++;
		}
		else
		{
			num[j]=maze_conf[i];
			j++;
		}
		i++;
	}while(maze_conf[i] != '\0');
	num[j]='\0';
	
	//setting initial observation
	m_observation = current_node->percept;
	m_reward = 0;
}


void CheeseMaze::performAction(action_t action)
{
	//action takes agent into wall
	if(current_node->next[action] == NULL)
	{
		m_reward = -10;
		return;
	}
	//action takes agent into free cell
	else
		current_node = current_node->next[action];
	//set percept for agent
	m_observation = current_node->percept;
	m_reward = current_node == cheese_node ? 10 : -1;
}

bool CheeseMaze::isFinished() const
{
	return &current_node == &cheese_node ? 1 : 0;
}


//Creating the environment
ExtTiger::ExtTiger(options_t &options)
{
	p=1.0;
	//determine the probability of listening correctly
	if(options.count("listen-p")>0){
		strExtract(options["listen-p"],p);
	}
	assert(0.0<=p);
	assert(p>=1.0);
	
	standing = 0; //player is sitting
	tiger = rand01() < 0.5 ? 1 : 2; //tiger behind left door with 0.5 probability.
	//initial observation
	m_observation = 0;
	m_reward = 0;
}


//the observation will be correct with probability of p.
void ExtTiger::performAction(action_t action)
{
	switch(action)
	{
		case 0:
		m_reward = standing ? -10 : -1;
		standing = 1;
		break;
		
		case 2:
		if(standing){
			m_reward = tiger == 2 ? -100 : 30;
		}
		else{
			m_reward = -10;
		}
		break;
		
		case 3:
		if(standing){
			m_reward = tiger == 2 ? 30 : -100;
		}
		else{
			m_reward = -10;
		}
		break;
		
		case 1:
		if(!standing){
			if(tiger == 1)
				m_observation = rand01() < p ? 1 : 2;
			else
				m_observation = rand01() < p ? 2 : 1;
			
			m_reward = -1;
		}
		else{
			m_reward = -10;
		}
	}
}

/*Tic Tac Toe environment.*/
TicTacToe::TicTacToe(options_t &options)
{
	for (int i = 0; i < 9; i++)
		board[i] = 0;

	freeCells = 9;
	finished = 0;

	//return the initial percept,
	m_observation = calBoardVal();
	m_reward = 0;
}

void TicTacToe::performAction(action_t action)
{
	if (board[action] != 0 && freeCells != 0) //illegal move
	{
		m_reward = -3;
		return; //Obverstaion will not change so there is no need to re-calculate
	}
	else
	{
		board[action] = 2;
		if (check_winner() == 2) //agent won the game
		{
			m_reward = 2;
			m_observation = calBoardVal();
			finished = 1;
			return;
		}
		else if (--freeCells == 0) //game is a draw
		{
			m_reward = 1;
			m_observation = calBoardVal();
			finished = 1;
			return;
		}
		else
		{
			env_move();
			if (check_winner() == 1) //agent lost the game
			{
				m_reward = -2;
				m_observation = calBoardVal();
				finished = 1;
				return;
			}
			else //game has not yet ended
			{
				m_reward = 0;
				m_observation = calBoardVal();
				return;
			}
		}
	}
}

bool TicTacToe::isFinished() const
{
	return finished;
}


/*
	Biased Rock Paper Scissors environment*
	Move:
		0 : Rock
		1 : Paper
		2 : Scissors
*/
BRockPaperScissors::BRockPaperScissors(options_t &options)
{
	move = (int)(rand01() * 3);
	//return the initial percept
	m_observation = 0;
	m_reward = 0;
}

void BRockPaperScissors::performAction(action_t action)
{
	if (action != move)
	{
		switch (move)
		{
		case 0:
			m_reward = action == 1 ? 1 : -1;
			break;
		case 1:
			m_reward = action == 2 ? 1 : -1;
			break;
		case 2:
			m_reward = action == 0 ? 1 : -1;
		}
	}
	else
	{
		m_reward = 0;
	}

	m_observation = move;
	
	move = m_reward == -1 ? move : (int)(rand01() * 3);
}


/* Pacman environment
 */
Pacman::Pacman(options_t &options)
{
	maze[19][21].content = {
			{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
			{0,1,2,2,2,1,1,1,1,1,1,1,1,2,2,2,2,2,0},
			{0,2,0,0,1,0,0,0,2,0,1,0,0,0,1,0,0,2,0},
			{0,3,1,1,1,1,1,2,2,2,2,1,2,1,1,2,1,3,0},
			{0,2,0,0,1,0,1,0,0,0,0,0,2,0,1,0,0,1,0},
			{0,2,2,1,2,0,2,2,1,0,1,1,1,0,1,1,1,1,0},
			{0,0,0,0,2,0,0,0,1,0,1,0,0,0,2,0,0,0,0},
			{0,0,0,0,1,0,1,1,1,1,1,1,1,0,1,0,0,0,0},
			{0,0,0,0,1,0,1,0,1,1,1,0,1,0,1,0,0,0,0},
			{1,1,1,1,1,0,1,0,1,1,1,0,1,0,1,1,1,1,1},
			{0,0,0,0,2,0,1,0,0,0,0,0,1,0,1,1,1,1,1},
			{0,0,0,0,2,0,1,1,1,1,1,1,1,0,1,0,0,0,0},
			{0,0,0,0,1,0,1,0,0,0,0,0,2,0,1,0,0,0,0},
			{0,1,1,1,2,2,1,2,1,1,1,2,1,2,2,2,2,2,0},
			{0,1,0,0,1,0,0,0,2,0,1,0,0,0,2,0,0,1,0},
			{0,3,2,0,2,1,1,1,1,1,1,2,1,2,2,0,1,3,0},
			{0,0,2,0,2,0,1,0,0,0,0,0,1,0,2,0,1,0,0},
			{0,1,1,2,1,0,1,2,2,0,2,2,2,0,1,1,1,1,0},
			{0,1,0,0,0,0,0,0,1,0,2,0,0,0,0,0,0,1,0},
			{0,2,2,1,1,1,2,2,2,1,1,2,1,1,1,2,2,2,0},
			{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
	};
	std::bitset<4> per;
	for(int i=1;i<18; i++)
		for(int j=1; j<10; j++)
		{
			if(maze[i][j].content != 0)
			{
				maze[i-1][j].content == 0 ? per.set(3,1) : per.set(3,0);
				maze[i][j+1].content == 0 ? per.set(2,1) : per.set(2,0);
				maze[i+1][j].content == 0 ? per.set(1,1) : per.set(1,0);
				maze[i][j-1].content == 0 ? per.set(0,1) : per.set(0,0);
				maze[i][j].wall = per.to_ulong() & INT_MAX;
			}
			else
				maze[i][j].wall = 15;
		}
	maze[9][0].wall = 10;
	maze[9][18].wall = 10;

	ghost[0].x = 8;
	ghost[0].y = 9;

	ghost[1].x = ghost[0].x;
	ghost[1].y = ghost[0].y +1;

	ghost[2].x = ghost[0].x +1;
	ghost[2].y = ghost[0].y;

	ghost[3].x = ghost[2].x;
	ghost[3].y = ghost[1].y;

	pacman.x = 8;
	pacman.y = 13;
}
