#include "environment.hpp"

#include <cassert>
#include <stack>

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
	//setup the environment according to the config file
	if (options.count("maze-structure") > 0){
		strExtract(options["maze-structure"], maze_conf);
	}
	
	//Create the nodes of the maze.
	int i=0;
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
		}
		else
		{
			num[j]=maze_conf[i];
			j++;
		}
		i++;
	}while(maze_conf[i] != '\0');
	num[j]='\0';
}


void CheeseMaze::performAction(action_t action)
{
	//action takes agent into wall
	if(current_node.next[action] == NULL)
		m_reward = -10;
	//action takes agent into free cell
	else
		current_node = *current_node.next[action];
	//set percept for agent
	m_observation = current_node.percept;
	m_reward = &current_node == &cheese_node ? 10 : -1;
}

bool CheeseMaze::isFinished() const
{
	return &current_node == &cheese_node ? 1 : 0;
}

