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

    // receives the agent's action and calculates the new environment percept
    virtual void performAction(action_t action) = 0;

    //reset the environment after episode
    virtual void envReset(void) {
        return;
    }

    // returns true if the environment cannot interact with the agent anymore
    virtual bool isFinished(void) const {
        return false;
    }

    void getPercept(symbol_list_t &symbol_list);

    percept_t getObservation(void) const {
        return m_observation;
    }

    percept_t getReward(void) const {
        return m_reward;
    }

protected:
    // visible to inherited classes
    action_t m_last_action;  // the last action performed by the agent
    percept_t m_observation; // the current observation
    percept_t m_reward;      // the current reward

};

// An experiment involving flipping a biased coin and having the agent predict
// whether it will come up heads or tails. The agent receives a reward of 1
// for a correct guess and a reward of 0 for an incorrect guess.
class CoinFlip: public Environment {
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
class CheeseMaze: public Environment {
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
    struct node_t {
        unsigned int percept;
        node_t *next[4];
    };

    std::string m_maze_conf;
    node_t *m_current_node;
    node_t *m_cheese_node;
    node_t *m_mouse_start;
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
class ExtTiger: public Environment {
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
    double m_p; //probability of listening correctly
    bool m_tiger; //position of tiger
    bool m_standing; //player is standing
};

//Tic Tac Toe environment
class TicTacToe: public Environment {
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
    int m_board[9];
    bool m_finished;
    int m_freeCells;

    //calculates the observation for the agent
    percept_t calBoardVal() {
        std::bitset<18> boardVal;
        for (int i = 0; i < 9; i++) {
            if (m_board[i] == 0) {
                boardVal.set(i * 2, 0);
                boardVal.set((i * 2) + 1, 0);
            } else if (m_board[i] == 1) {
                boardVal.set(i * 2, 0);
                boardVal.set((i * 2) + 1, 1);
            } else if (m_board[i] == 2) {
                boardVal.set(i * 2, 1);
                boardVal.set((i * 2) + 1, 0);
            }
        }
        return boardVal.to_ulong() & INT_MAX;
    }

    //some random move on the cell structure
    void env_move() {
        int move = (int) (rand01() * m_freeCells);
        int count = 0;
        for (int i = 0; i < 9; i++) {
            if (m_board[i] == 0)
                if (count++ == move) {
                    m_board[i] = 1;
                    m_freeCells--;
                    return;
                }
        }
    }

    //returns 0 if there is no winner yet else returns the player number.
    //player number is 1 for environment and 2 for agent
    int check_winner() {
        if (m_board[0] != 0) {
            if ((m_board[0] == m_board[1] && m_board[1] == m_board[2])
                    || (m_board[0] == m_board[3] && m_board[0] == m_board[6])
                    || (m_board[0] == m_board[4] && m_board[4] == m_board[8]))
                return m_board[0];
        } else if (m_board[1] != 0) {
            if (m_board[1] == m_board[4] && m_board[4] == m_board[7])
                return m_board[1];
        } else if (m_board[2] != 0) {
            if ((m_board[2] == m_board[5] && m_board[5] == m_board[8])
                    || (m_board[2] == m_board[4] && m_board[4] == m_board[6]))
                return m_board[2];
        } else if (m_board[3] != 0) {
            if (m_board[3] == m_board[4] && m_board[4] == m_board[5])
                return m_board[3];
        } else if (m_board[6] != 0) {
            if (m_board[6] == m_board[7] && m_board[7] == m_board[8])
                return m_board[6];
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
class BRockPaperScissors: public Environment {
public:
    //constructor to initialise the environment and the set the initial percepts for the agent;
    BRockPaperScissors(options_t &options);

    //actions of the agent
    virtual void performAction(action_t action);

private:
    unsigned int m_move;
};

/*Pacman environment
 *	0 : Wall
 *	1 : Empty Cell
 *	2 : Food Pellet
 *	3 : Power Pill
 */
class Pacman: public Environment {
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
    //cell represents each node of the pacman maze.
    struct cell {
        unsigned int wall; //stores the percept of the node which the agent will recieve as wall configuration.
        bool isFreeCell;
        int contents; //0: empty, 1: foot pellet, 2: power pill
    };
    cell m_maze[21][19];

    //structure of the maze which will be read by the class variables during construction
    bool maze1[21][19] = { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0 }, { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
            { 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0 }, { 0, 1,
                    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 }, { 0, 1,
                    0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0 }, { 0, 1,
                    1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0 }, { 0, 0,
                    0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0 }, { 0, 0,
                    0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0 }, { 0, 0,
                    0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0 }, { 1, 1,
                    1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1 }, { 0, 0,
                    0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0 }, { 0, 0,
                    0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0 }, { 0, 0,
                    0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0 }, { 0, 1,
                    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 }, { 0, 1,
                    0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0 }, { 0, 1,
                    1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0 }, { 0, 0,
                    1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0 }, { 0, 1,
                    1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0 }, { 0, 1,
                    0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0 }, { 0, 1,
                    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 }, { 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } };

    //a structure to maintain the position and state of various entities in the game.
    struct pos {
        int x;
        int y;
        bool state;
    };

    //represents the node of a graph which is used to find the shortest path.
    struct node {
        pos cell;
        int h_value;
        int g_value;
        node* parent;
    };

    //if the pacman is under the effect of the power pill
    bool m_pacman_powered;

    //the number of moves for which the ghost will follow pacman
    int m_ghost_follow_time = 5;

    //saves the number of moves for which the ghost has been following pacman
    //or the number of moves for which the ghost has been actively not following.
    int m_ghost_timer[4];

    //position and state of each ghost
    pos ghost[4];

    //position and the state of pacman
    pos m_pacman;

    //if the game has finished
    bool m_finished;

    //counter to check how long pacman has been under the effect of the power pill
    int m_power_pill_counter;

    //How long pacman can be under the effect of power pill. Loaded from configuration file
    int m_power_pill_time = 10;

    //returns the bits for direction of ghost which are in line of sight
    unsigned int seeGhost() {
        std::bitset<4> chk;
        //checking in each direction
        for (int i = 0; i < 4; i++) {
            chk.set(3 - i, 0);
            if (i == 0 || i == 2) {
                //we only need to check if there exists a ghost which has the same y co-ordinate as the pacman
                if (ghost[0].y == m_pacman.y || ghost[1].y == m_pacman.y
                        || ghost[2].y == m_pacman.y
                        || ghost[3].y == m_pacman.y) {
                    //either looking in up direction or down depending on value of i
                    for (int j = m_pacman.x; i == 0 ? j > 0 : j < 21;
                            i == 0 ? j-- : j++) {
                        assert(0 < j && j < 21);
                        //if the loop encounters a wall then pacman cannot see the ghost, we break out of the loop
                        if (!m_maze[j][m_pacman.y].isFreeCell)
                            break;
                        //we do not need to check further once 1 ghost has been encountered
                        else if (ghost[0].x == j || ghost[1].x == j
                                || ghost[2].x == j || ghost[3].x == j) {
                            chk.set(3 - i, 1);
                            break;
                        }
                    }
                }
            } else {
                //checking in right and left direction
                if (ghost[0].x == m_pacman.x || ghost[1].x == m_pacman.x
                        || ghost[2].x == m_pacman.x
                        || ghost[3].x == m_pacman.x) {
                    //either checking in right or left direction depending on the value of i
                    for (int j = m_pacman.y; i == 1 ? j > 0 : j < 19;
                            i == 1 ? j-- : j++) {
                        assert(0 <= j && j < 19);
                        if (!m_maze[m_pacman.x][j].isFreeCell)
                            break;
                        else if (ghost[0].y == j || ghost[1].y == j
                                || ghost[2].y == j || ghost[3].y == j) {
                            chk.set(3 - i, 1);
                            break;
                        }
                    }
                }
            }

        }
        //converting the bitset to unsigned int.
        return chk.to_ulong() & INT_MAX;
    }

    //function returns the observation indicating if there is a food pellet at a manhattan distance of 2,3 or 4 form the pacman
    unsigned int smellFood() {
        std::bitset<3> smell;
        //checking all cell at a man_dist of 1 to 4
        for (int man_dist = 1;
                man_dist <= 4 && !smell.test(std::max(man_dist - 2, 0));
                man_dist++) {
            //i here represents the possible shift in x co-ordinate we need to check.
            //For a cell to be within a manhattan distance of man_dist from the pacman its x co-ordinate has be between as a distance of man_dist from the x co-ordinate of the pacman
            //hence we only need to vary i from -|pacman.x - man_dist| to |pacman.x - man_dist|
            for (int i = std::max(m_pacman.x - man_dist, 0);
                    i <= std::min(m_pacman.x + man_dist, 20)
                            && !smell.test(std::max(man_dist - 2, 0)); i++) {
                assert(
                        m_pacman.x - man_dist <= i && 0 <= i
                                && m_pacman.x + man_dist >= i && i <= 20);
                //since |pacman.x-i| + |pacman.y-j| <= man_dist
                //the range of j changes based on the value of i, such that the manhattan distance is bounded by man_dist.
                for (int j = std::max(
                        m_pacman.y - abs(abs(m_pacman.x - i) - man_dist), 0);
                        j
                                <= std::min(
                                        m_pacman.y
                                                + abs(
                                                        abs(m_pacman.x - i)
                                                                - man_dist), 18)
                                && !smell.test(std::max(man_dist - 2, 0));
                        j++) {
                    assert(
                            0 <= j && m_pacman.y - man_dist <= j
                                    && m_pacman.y + man_dist >= j && j <= 18);
                    assert(
                            abs(m_pacman.x - i) + abs(m_pacman.y - j)
                                    <= man_dist);
                    if (m_maze[i][j].contents == 1) {
                        for (int k = std::max(man_dist - 2, 0); k <= 2; k++) {
                            smell.set(k, 1);
                        }
                    }
                }
            }
        }
        return smell.to_ulong() & INT_MAX;
    }

    //function returns if there is any food in line of sight
    unsigned int seeFood() {
        std::bitset<4> sight;
        for (int i = 0; i < 4; i++) {
            sight.set(3 - i, 0); //assuming there is no food in line of sight
            if (i == 1 || i == 3) {
                //checking both left and right depending on value of i
                for (int j = m_pacman.y; i == 3 ? j > 0 : j < 19;
                        i == 3 ? j-- : j++) {
                    assert(0 <= j && j < 19);
                    //breaks on encounter either a wall of a food pellet
                    if (!m_maze[m_pacman.x][j].isFreeCell)
                        break;
                    else if (m_maze[m_pacman.x][j].contents == 1) {
                        sight.set(3 - i, 1);
                        break;
                    }
                }
            } else if (i == 0 || i == 2) {
                //checking top and down dependin on the value of i
                for (int j = m_pacman.x; i == 0 ? j > 0 : j < 21;
                        i == 0 ? j-- : j++) {
                    assert(0 <= j && j < 21);
                    //breaks when the loop encounters either a wall or a food pellet
                    if (!m_maze[j][m_pacman.y].isFreeCell)
                        break;
                    else if (m_maze[j][m_pacman.y].contents == 1) {
                        sight.set(3 - i, 1);
                        break;
                    }
                }
            }
        }
        return sight.to_ulong() & INT_MAX;
    }

    bool isCaught() {

        for (int i = 0; i < 4; i++) {
            if ((m_pacman.x == ghost[i].x) && (m_pacman.y == ghost[i].y)) {
                //ghost is active and pacman does not have power pill
                if (ghost[i].state && !m_pacman.state)
                    return 1;
                else if (m_pacman.state) //pacman under effect of power pill
                    ghost[i].state = 0;
            }
        }
        return 0;
    }

    /*
     This method moves ghost when it is at a manhattan distance of less than 5 from the agent.
     The environment finds a path using A* algorithm
     In case the ghost cannot find any path to its destination because it is blocked by other ghosts
     it will not make a move.
     */
    void manMove(int ghostNo) {
        //goal is either pacman or home in case the ghost was eaten
        pos goal;
        pos move = ghost[ghostNo];
        node* min_node;
        node* open_list[400];
        node* closed_list[400];
        bool path_found = false;
        //size of each list since we do not need only check positions where we have a added a node
        int open_list_size = 0;
        int closed_list_size = 0;

        //fix the goal based on the state
        if (ghost[ghostNo].state) {
            goal.x = m_pacman.x;
            goal.y = m_pacman.y;
        } else {
            //ghost has been eaten and is going home
            goal.x = 8 + (int) (ghostNo / 2);
            goal.y = 9 + (int) (ghostNo % 2);
        }
        open_list[0] = new node;
        open_list[0]->cell = ghost[ghostNo];
        open_list[0]->parent = NULL;
        open_list[0]->g_value = 0;
        open_list[open_list_size++]->h_value = manhattan_dist(ghost[ghostNo],
                goal);

        //if the size of the open list becomes 0 and we have not found the path yet that means all paths are blocked
        while (open_list_size > 0) {
            min_node = open_list[0];
            int min_node_index = 0;
            //find node with best f_value
            for (int i = 1; i < open_list_size; i++) {
                if ((open_list[i]->h_value + open_list[i]->g_value)
                        < (min_node->h_value + min_node->g_value)) {
                    min_node = open_list[i];
                    min_node_index = i;
                }
            }
            //removing min_node from the open list
            for (int i = min_node_index; i < open_list_size - 1; i++) {
                open_list[i] = open_list[i + 1];
            }
            open_list_size--;

            //check if the node is the goal
            if (min_node->cell.x == goal.x && min_node->cell.y == goal.y) {
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
                if (min_node->cell.x == 9 && min_node->cell.y == 0
                        && yshift == -1) {
                    yshift = 18;
                } else if (min_node->cell.x == 9 && min_node->cell.y == 18
                        && yshift == 1) {
                    yshift = -18;
                }
                if (m_maze[min_node->cell.x + xshift][min_node->cell.y + yshift].isFreeCell) {
                    bool conflict = false;
                    //check if this position is occupied by another ghost
                    for (int i = 0; i < 4; i++) {
                        if ((ghost[i].x == min_node->cell.x + xshift)
                                && (ghost[i].y == min_node->cell.y + yshift) /*&& (!ghost[i].state || !ghost[ghostNo].state)*/) {
                            conflict = true;
                        }
                    }
                    if (!conflict) {
                        //the ghost can possibly move into this node in the future
                        pos possible_node;
                        possible_node.x = min_node->cell.x + xshift;
                        possible_node.y = min_node->cell.y + yshift;
                        //check if node is already in open list
                        bool check = true;
                        for (int k = 0; k < open_list_size; k++) {
                            if (open_list[k]->cell.x == possible_node.x
                                    && open_list[k]->cell.y
                                            == possible_node.y) {
                                check = false;
                                if (open_list[k]->g_value
                                        < (min_node->g_value + 1)) {
                                    open_list[k]->g_value = min_node->g_value
                                            + 1;
                                    open_list[k]->parent = min_node;
                                }
                            }
                        }
                        for (int k = 0; k < closed_list_size; k++) {
                            if (closed_list[k]->cell.x == possible_node.x
                                    && closed_list[k]->cell.y
                                            == possible_node.y) {
                                check = false;
                            }
                        }
                        if (check) {
                            open_list[open_list_size] = new node;
                            open_list[open_list_size]->cell = possible_node;
                            open_list[open_list_size]->g_value =
                                    min_node->g_value + 1;
                            open_list[open_list_size]->h_value = manhattan_dist(
                                    possible_node, goal);
                            open_list[open_list_size++]->parent = min_node;
                        }
                    }
                }
            }
        }
        //find the move in the path
        while (min_node->parent != NULL && path_found) {
            move = min_node->cell;
            min_node = min_node->parent;
        }
        for (int i = 0; i < open_list_size; i++) {
            delete open_list[i];
        }
        for (int i = 0; i < closed_list_size; i++) {
            delete closed_list[i];
        }

        assert(m_maze[move.x][move.y].isFreeCell);

        ghost[ghostNo].x = move.x;
        ghost[ghostNo].y = move.y;
    }

    //this method return the path for a ghost in powerless state
    void eatenGhostMove(int ghostNo) {
        manMove(ghostNo);
        if ((ghost[ghostNo].x == 8 + (int) (ghostNo / 2))
                && (ghost[ghostNo].y == 9 + (int) (ghostNo % 2)))
            ghost[ghostNo].state = 1;
        return;
    }

    int manhattan_dist(pos start, pos goal) {
        return (abs(start.x - goal.x) + abs(start.y - goal.y));
    }
};

#endif // __ENVIRONMENT_HPP__
