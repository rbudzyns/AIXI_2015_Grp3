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
		if(maze_conf[i] == ',' || maze_conf[i] == '\0')
		{
			num[j] = '\0';
			std::istringstream(num)>>per;
			node *new_node = new node;
			new_node->percept = per;
			if(count == mouse_pos)
				mouse_start = new_node;
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
	}while(maze_conf[i++] != '\0');
	
	current_node = mouse_start;
	//setting initial observation
	m_observation = current_node->percept;
	m_reward = 10;
}


void CheeseMaze::performAction(action_t action)
{
	//action takes agent into wall
	if(current_node->next[action] == NULL)
	{
		m_reward = 0;
		return;
	}
	//action takes agent into free cell
	else
		current_node = current_node->next[action];
	//set percept for agent
	m_observation = current_node->percept;
	m_reward = current_node == cheese_node ? 20 : 9;
}

void CheeseMaze::envReset()
{
	current_node = mouse_start;
	m_reward = 10;
	m_observation = current_node->percept;
}

bool CheeseMaze::isFinished() const
{
	return current_node == cheese_node ? 1 : 0;
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
	assert(p<=1.0);
	
	standing = 0; //player is sitting
	tiger = rand01() < 0.5 ? 0 : 1; //tiger behind left door with 0.5 probability.
	//initial observation
	m_observation = 0;
	m_reward = 100;
}


//the observation will be correct with probability of p.
void ExtTiger::performAction(action_t action)
{
	switch(action)
	{
		/*
		 * Agent tries to stand
		 * STATE	REWARD
		 * sitting	-1
		 * standing	-10
		 */
		case 0:
		m_reward = standing ? -10 : -1;
		standing = 1;
		break;
		
		/*
		 * Agent ties to open left door
		 * STATE								REWARD
		 * sitting								-10
		 * standing(tiger behind left door) 	-100
		 * standing(tiger behind right door)	 30
		 */
		case 2:
		if(standing){
			m_reward = tiger ? -100 : 30;
		}
		else{
			m_reward = -10;
		}
		break;
		
		/*
		 * Agent tries to open right door
		 * STATE								REWARD
		 * sitting								-10
		 * Standing(tiger behind left door)		 30
		 * Standing(tiger behind right door)	-100
		 */
		case 3:
		if(standing){
			m_reward = tiger ? 30 : -100;
		}
		else{
			m_reward = -10;
		}
		break;
		
		/*
		 * Agent tries to listen
		 * STATE									REWARD
		 * sitting									-1
		 * standing									-10
		 */
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
	m_reward = (int)m_reward + 100;
}

//check if the environment is finished
bool ExtTiger::isFinished() const{
	if(m_reward == 130 ||m_reward == 0)
		return 1;
	else
		return 0;
}

//reset the enviroment
void ExtTiger::envReset(){
	standing = 0; //player is sitting
	tiger = rand01() < 0.5 ? 0 : 1; //tiger behind left door with 0.5 probability.
	//initial observation
	m_observation = 0;
	m_reward = 100;
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
	m_reward =3;
}

void TicTacToe::performAction(action_t action)
{
	/*
	for(int x = 0; x<3; x++)
	{
		for(int y=0; y<3; y++)
		{
			std::cout << board[x*y] << " ";
		}
		std::cout << std::endl;
	}
	*/
	if (board[action] != 0 && freeCells != 0) //illegal move
	{
		m_reward = 0;
		return; //Obverstaion will not change so there is no need to re-calculate
	}
	else
	{
		board[action] = 2;
		freeCells--;
		if (check_winner() == 2) //agent won the game
		{
			m_reward = 5;
			m_observation = calBoardVal();
			finished = 1;
			return;
		}
		else if (freeCells == 0) //game is a draw
		{
			m_reward = 4;
			m_observation = calBoardVal();
			finished = 1;
			return;
		}
		else
		{
			env_move();
			if (check_winner() == 1) //agent lost the game
			{
				m_reward = 1;
				m_observation = calBoardVal();
				finished = 1;
				return;
			}
			else if (freeCells != 0) //game has not yet ended
			{
				m_reward = 3;
				m_observation = calBoardVal();
				return;
			}
			else
			{
				m_reward = 4;
				m_observation = calBoardVal();
				finished = 1;
				return;
			}
		}
	}
}

//reset the environment
void TicTacToe::envReset()
{
	for (int i = 0; i < 9; i++)
		board[i] = 0;

	freeCells = 9;
	finished = 0;

	//return the initial percept,
	m_observation = calBoardVal();
	m_reward = 3;
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
	//return the initial precept
	m_observation = move;
	m_reward = 1;
}

void BRockPaperScissors::performAction(action_t action)
{
	move = m_reward == 0 ? move : (int)(rand01() * 3);
	if (action != move)
	{
		switch (move)
		{
		case 0:
			m_reward = action == 1 ? 2 : 0;
			break;
		case 1:
			m_reward = action == 2 ? 2 : 0;
			break;
		case 2:
			m_reward = action == 0 ? 2 : 0;
		}
	}
	else
	{
		m_reward = 1;
	}

	m_observation = move;
}


/* Pacman environment
 */
Pacman::Pacman(options_t &options)
{
	if (options.count("power-pill-time")>0) {
		strExtract(options["power-pill-time"], power_pill_time);
	}

	if(options.count("ghost-follow-time")>0) {
		strExtract(options["ghost-follow-time"], ghost_follow_time);
	}

	//encoding the structure of the maze
	bool maze1[21][19] = {
			{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
			{0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
			{0,1,0,0,1,0,0,0,1,0,1,0,0,0,1,0,0,1,0},
			{0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
			{0,1,0,0,1,0,1,0,0,0,0,0,1,0,1,0,0,1,0},
			{0,1,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,1,0},
			{0,0,0,0,1,0,0,0,1,0,1,0,0,0,1,0,0,0,0},
			{0,0,0,0,1,0,1,1,1,1,1,1,1,0,1,0,0,0,0},
			{0,0,0,0,1,0,1,0,1,1,1,0,1,0,1,0,0,0,0},
			{1,1,1,1,1,0,1,0,1,1,1,0,1,0,1,1,1,1,1},
			{0,0,0,0,1,0,1,0,0,0,0,0,1,0,1,0,0,0,0},
			{0,0,0,0,1,0,1,1,1,1,1,1,1,0,1,0,0,0,0},
			{0,0,0,0,1,0,1,0,0,0,0,0,1,0,1,0,0,0,0},
			{0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
			{0,1,0,0,1,0,0,0,1,0,1,0,0,0,1,0,0,1,0},
			{0,1,1,0,1,1,1,1,1,1,1,1,1,1,1,0,1,1,0},
			{0,0,1,0,1,0,1,0,0,0,0,0,1,0,1,0,1,0,0},
			{0,1,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,1,0},
			{0,1,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,1,0},
			{0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
			{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
	};
	for (int i = 0; i < 21; i++)
		for (int j = 0; j < 19; j++)
			maze[i][j].isFreeCell = maze1[i][j];
	std::bitset<4> per;
	for(int i=1;i<20; i++)
		for(int j=1; j<18; j++)
		{
			if(maze[i][j].isFreeCell)
			{
				!maze[i-1][j].isFreeCell ? per.set(3,1) : per.set(3,0);
				!maze[i][j+1].isFreeCell ? per.set(2,1) : per.set(2,0);
				!maze[i+1][j].isFreeCell ? per.set(1,1) : per.set(1,0);
				!maze[i][j-1].isFreeCell ? per.set(0,1) : per.set(0,0);
				maze[i][j].wall = per.to_ulong() & INT_MAX;
				if(rand01() < 0.5)
					maze[i][j].contents = 1;
				else
					maze[i][j].contents = 0;
		}
			else
				maze[i][j].wall = 15;
	}
	maze[9][0].wall = 10;
	maze[9][18].wall = 10;
	if (rand01() < 0.5)
		maze[9][0].contents = 1;
	else
		maze[9][0].contents = 0;
	if (rand01() < 0.5)
			maze[9][18].contents = 1;
	else
			maze[9][18].contents = 0;

	maze[1][3].contents = 2;
	maze[17][3].contents = 2;

	maze[1][15].contents = 2;
	maze[17][15].contents = 2;

	//initialising the ghosts
	for (int i = 0; i < 4; i++)
	{
		ghost[i].x = 8 + (int)(i / 2);
		ghost[i].y = 9 + (int)(i % 2);
		ghost[i].state = 1;
		ghost_timer[i] = 0;
		assert(maze[ghost[i].x][ghost[i].y].isFreeCell);
	}
	//initialising pacman
	pacman.x = 13;
	pacman.y = 9;
	pacman.state = 0;
	maze[13][9].contents = 0;

	power_pill_counter = 0;

	finished = false;

	m_observation = ((maze[pacman.x][pacman.y].wall & 15) << 12) | ((seeGhost() & 15) << 8) | ((smellFood() & 7) << 5) | ((seeFood() & 15) << 1) | pacman.state;
	m_reward = 60;
}

void Pacman::performAction(action_t action)
{
	m_reward = 60;
	int pac_x_shift = (2 - (int)action) % 2;
	int pac_y_shift = ((int)action - 1) % 2;
	assert(action == 0 ? pac_x_shift == 0 : true);
	assert(pac_x_shift == 0 || pac_x_shift == -1 || pac_x_shift == 1);
	assert(pac_y_shift == 0 || pac_y_shift == -1 || pac_y_shift == 1);
	assert(pac_x_shift == 0 ? pac_y_shift != 0 : pac_y_shift == 0);

	//power pill effect fades when counter reaches 0
	if (power_pill_counter-- > 1)
		pacman.state = 0;

	if(maze[pacman.x + pac_x_shift][pacman.y + pac_y_shift].isFreeCell)
	{
		pacman.x = pacman.x + pac_x_shift;
		pacman.y = pacman.y + pac_y_shift;
		assert(maze[pacman.x][pacman.y].isFreeCell);
		if (!isCaught())
		{
			if (maze[pacman.x][pacman.y].contents == 1) //pacman moved to a cell with food
			{
				maze[pacman.x][pacman.y].contents = 0;
				m_reward += 10;
			}
			else if (maze[pacman.x][pacman.y].contents == 2) //POWER PILL!!!
			{
				pacman.state = 1;
				power_pill_counter = power_pill_time;
			} 
			else //empty cell
				m_reward += -1;
		}
		else //pacman ran into the ghost
		{
			m_reward += -50;
			finished = true;
			m_observation = ((maze[pacman.x][pacman.y].wall & 15) << 12) | ((seeGhost() & 15) << 8) | ((smellFood() & 7) << 5) | ((seeFood() & 15) << 1) | pacman.state;
			return;
		}
	}
	else //pacman ran into the wall
		m_reward += -10;
		
	for(int i=0; i<4; i++)
	{
		if(ghost[i].state)
		{
			if(manhattan_dist(ghost[i], pacman) < 5 && ghost_timer[i]-- !=1)
			{
				if(ghost_timer[i] < 0)
				{
					ghost_timer[i] = ghost_follow_time;
				}
				manMove(i);
			}
			else
			{
				int movableCells[4][2];
				int count = 0;
				for (int j = 3; j >= 0; j--) //check each direction for empty cell
				{
					int xshift = (2 - j) % 2;
					assert(j == 0 ? xshift == 0 : true);
					assert(xshift == 0 || xshift == -1 || xshift == 1);
					int yshift = (j - 1) % 2;
					assert(yshift == 0 || yshift == -1 || yshift == 1);
					assert(xshift == 0 ? yshift != 0 : yshift == 0);
					if (maze[ghost[i].x + xshift][ghost[i].y + yshift].isFreeCell)
					{
						//check for collisions with other ghosts
						bool ghost_collision = false;
						for (int k = 0; k < 4; k++)
						{
							if ((ghost[k].x == (ghost[i].x + xshift) && ghost[k].y == (ghost[i].y + yshift)) || !ghost[k].state)
							{
								ghost_collision = true;
							}
						}
						if(!ghost_collision)
						{
							movableCells[count][0] = ghost[i].x + xshift;
							movableCells[count++][1] = ghost[i].y + yshift;
						}
					}
				}
				int move = (int)(rand01() * count);
				ghost[i].x = movableCells[move][0];
				ghost[i].y = movableCells[move][1];
			}
			assert(maze[ghost[i].x][ghost[i].y].isFreeCell);
			/*
			for (int i_1 = 0; i_1 < 4; i_1++)
			{
				assert((ghost[i_1].x != pacman.x) || (ghost[i_1].y != pacman.y));
				for (int i_2 = 0; i_2 < 4; i_2++)
				{
					if(!((ghost[i_1].x != ghost[i_2].x) || (ghost[i_1].y != ghost[i_2].y) || (i_1 == i_2)))
					{
						std::cout <<i_1<<i_2<<std::endl;
					}
				}
			}
			*/
		}
		else
		{
			eatenGhostMove(i);
		}
	}

	if (isCaught())
	{
		m_reward += -50;
		finished = true;
	}

	m_observation = ((maze[pacman.x][pacman.y].wall & 15) << 12) | ((seeGhost() & 15) << 8) | ((smellFood() & 7) << 5) | ((seeFood() & 15) << 1) | pacman.state;

	//check if all food is eaten
	for (int x = 0; x < 21; x++)
		for (int y = 0; y < 19; y++)
			if (maze[x][y].contents == 1)
				return;
	
	//agent wins
	finished = true;
	m_reward += 100;
}
	
bool Pacman::isFinished(void) const
{
	return finished;
}

void Pacman::envReset(void)
{
	for(int i=0;i<21; i++)
		for(int j=0; j<19; j++)
		{
			if(maze[i][j].isFreeCell)
			{
				if(rand01() < 0.5)
					maze[i][j].contents = 1;
				else
					maze[i][j].contents = 0;
			}
		}

	maze[1][3].contents = 2;
	maze[17][3].contents = 2;

	maze[1][15].contents = 2;
	maze[17][15].contents = 2;

	//initialising the ghosts
	for (int i = 0; i < 4; i++)
	{
		ghost[i].x = 8 + (int)(i / 2);
		ghost[i].y = 9 + (int)(i % 2);
		ghost[i].state = 1;
		ghost_timer[i] = 0;
	}
	//initialising pacman
	pacman.x = 13;
	pacman.y = 9;
	pacman.state = 0;
	maze[13][9].contents = 0;

	finished = false;

	m_observation = ((maze[pacman.x][pacman.y].wall & 15) << 12) | ((seeGhost() & 15) << 8) | ((smellFood() & 7) << 5) | ((seeFood() & 15) << 1) | pacman.state;
	m_reward = 60;
}
