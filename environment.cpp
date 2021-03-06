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
CheeseMaze::CheeseMaze(options_t &options) {
    int mouse_pos = 1;
    int cheese_pos = 1;
    //setup the environment according to the config file
    if (options.count("maze-structure") > 0) {
        strExtract(options["maze-structure"], m_maze_conf);
    }

    if (options.count("mouse-pos") > 0) {
        strExtract(options["mouse-pos"], mouse_pos);
    }

    if (options.count("cheese-pos") > 0) {
        strExtract(options["cheese-pos"], cheese_pos);
    }

    //Create the nodes of the maze.
    int i = 0;
    int count = 1;
    char num[3];
    int j = 0;
    unsigned int per;
    node_t *curr_node;
    //stack of pointers of a pointer to a node.
    /*Basic depth first search algorithm involves pushing into a stack the
     * neighbouring nodes. Since we do not know the graph beforehand,
     * we can push the addresses of the pointers in the structures which need
     * to have values assigned, which we can find out based on the value of
     * percept.
     */
    std::stack<node_t*> nodes;
    do {
        if (m_maze_conf[i] == ',' || m_maze_conf[i] == '\0') {
            num[j] = '\0';
            std::istringstream(num) >> per;
            node_t *new_node = new node_t;
            new_node->percept = per;
            if (count == mouse_pos)
                m_mouse_start = new_node;
            if (count == cheese_pos)
                m_cheese_node = new_node;
            for (int k = 0; k < 4; k++)
                new_node->next[k] = NULL;
            //if stack is empty, meaning all edges will have to be connected
            if (nodes.empty()) {
                if ((new_node->percept & 8) == 0)
                    nodes.push(new_node);
                if ((new_node->percept & 4) == 0)
                    nodes.push(new_node);
                if ((new_node->percept & 2) == 0)
                    nodes.push(new_node);
                if ((new_node->percept & 1) == 0)
                    nodes.push(new_node);
            } else {
                curr_node = nodes.top();
                nodes.pop();
                //connecting node is below the latest node
                if (((curr_node->percept & 8) == 0)
                        && (curr_node->next[0] == NULL)) {
                    new_node->next[2] = curr_node;
                    curr_node->next[0] = new_node;
                }
                //connecting node is on the left of the latest node
                else if (((curr_node->percept & 4) == 0)
                        && (curr_node->next[1] == NULL)) {
                    new_node->next[3] = curr_node;
                    curr_node->next[1] = new_node;
                }
                //connecting node is above the latest node
                else if (((curr_node->percept & 2) == 0)
                        && (curr_node->next[2] == NULL)) {
                    new_node->next[0] = curr_node;
                    curr_node->next[2] = new_node;
                } else if (((curr_node->percept & 1) == 0)
                        && (curr_node->next[3] == NULL)) {
                    new_node->next[1] = curr_node;
                    curr_node->next[3] = new_node;
                }

                //push the node into the stack for every free node which is still unassigned.
                if (((new_node->percept & 8) == 0)
                        && (new_node->next[0] == NULL))
                    nodes.push(new_node);
                if (((new_node->percept & 4) == 0)
                        && (new_node->next[1] == NULL))
                    nodes.push(new_node);
                if (((new_node->percept & 2) == 0)
                        && (new_node->next[2] == NULL))
                    nodes.push(new_node);
                if (((new_node->percept & 1) == 0)
                        && (new_node->next[3] == NULL))
                    nodes.push(new_node);
            }
            j = 0;
            count++;
        } else {
            num[j] = m_maze_conf[i];
            j++;
        }
    } while (m_maze_conf[i++] != '\0');

    m_current_node = m_mouse_start;
    //setting initial observation
    m_observation = m_current_node->percept;
    m_reward = 10;
}

void CheeseMaze::performAction(action_t action) {
    //action takes agent into wall
    if (m_current_node->next[action] == NULL) {
        m_reward = 0;
        return;
    }
    //action takes agent into free cell
    else
        m_current_node = m_current_node->next[action];
    //set percept for agent
    m_observation = m_current_node->percept;
    m_reward = m_current_node == m_cheese_node ? 20 : 9;
}

void CheeseMaze::envReset() {
    m_current_node = m_mouse_start;
    m_reward = 10;
    m_observation = m_current_node->percept;
}

bool CheeseMaze::isFinished() const {
    return m_current_node == m_cheese_node ? 1 : 0;
}

//Creating the environment
ExtTiger::ExtTiger(options_t &options) {
    m_p = 1.0;
    //determine the probability of listening correctly
    if (options.count("listen-p") > 0) {
        strExtract(options["listen-p"], m_p);
    }
    assert(0.0 <= m_p);
    assert(m_p <= 1.0);

    m_standing = 0; //player is sitting
    m_tiger = rand01() < 0.5 ? 0 : 1; //tiger behind left door with 0.5 probability.
    //initial observation
    m_observation = 0;
    m_reward = 100;
}

//the observation will be correct with probability of p.
void ExtTiger::performAction(action_t action) {
    switch (action) {
    /*
     * Agent tries to stand
     * STATE	REWARD
     * sitting	-1
     * standing	-10
     */
    case 0:
        m_reward = m_standing ? -10 : -1;
        m_standing = 1;
        break;

        /*
         * Agent ties to open left door
         * STATE								REWARD
         * sitting								-10
         * standing(tiger behind left door) 	-100
         * standing(tiger behind right door)	 30
         */
    case 2:
        if (m_standing) {
            m_reward = m_tiger ? -100 : 30;
        } else {
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
        if (m_standing) {
            m_reward = m_tiger ? 30 : -100;
        } else {
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
        if (!m_standing) {
            if (m_tiger == 1)
                m_observation = rand01() < m_p ? 1 : 2;
            else
                m_observation = rand01() < m_p ? 2 : 1;

            m_reward = -1;
        } else {
            m_reward = -10;
        }
    }
    m_reward = (int) m_reward + 100;
}

//check if the environment is finished
bool ExtTiger::isFinished() const {
    if (m_reward == 130 || m_reward == 0)
        return 1;
    else
        return 0;
}

//reset the enviroment
void ExtTiger::envReset() {
    m_standing = 0; //player is sitting
    m_tiger = rand01() < 0.5 ? 0 : 1; //tiger behind left door with 0.5 probability.
    //initial observation
    m_observation = 0;
    m_reward = 100;
}

/*Tic Tac Toe environment.*/
TicTacToe::TicTacToe(options_t &options) {
    for (int i = 0; i < 9; i++)
        m_board[i] = 0;

    m_freeCells = 9;
    m_finished = 0;

    //return the initial percept,
    m_observation = calBoardVal();
    m_reward = 3;
}

void TicTacToe::performAction(action_t action) {
    if (m_board[action] != 0 && m_freeCells != 0) //illegal move
            {
        m_reward = 0;
        return; //Obverstaion will not change so there is no need to re-calculate
    } else {
        m_board[action] = 2;
        m_freeCells--;
        if (check_winner() == 2) //agent won the game
                {
            m_reward = 5;
            m_observation = calBoardVal();
            m_finished = 1;
            return;
        } else if (m_freeCells == 0) //game is a draw
                {
            m_reward = 4;
            m_observation = calBoardVal();
            m_finished = 1;
            return;
        } else {
            env_move();
            if (check_winner() == 1) //agent lost the game
                    {
                m_reward = 1;
                m_observation = calBoardVal();
                m_finished = 1;
                return;
            } else if (m_freeCells != 0) //game has not yet ended
                    {
                m_reward = 3;
                m_observation = calBoardVal();
                return;
            } else {
                m_reward = 4;
                m_observation = calBoardVal();
                m_finished = 1;
                return;
            }
        }
    }
}

//reset the environment
void TicTacToe::envReset() {
    for (int i = 0; i < 9; i++)
        m_board[i] = 0;

    m_freeCells = 9;
    m_finished = 0;

    //return the initial percept,
    m_observation = calBoardVal();
    m_reward = 3;
}

bool TicTacToe::isFinished() const {
    return m_finished;
}

/*
 Biased Rock Paper Scissors environment*
 Move:
 0 : Rock
 1 : Paper
 2 : Scissors
 */
BRockPaperScissors::BRockPaperScissors(options_t &options) {
    m_move = (int) (rand01() * 3);
    //return the initial precept
    m_observation = m_move;
    m_reward = 1;
}

void BRockPaperScissors::performAction(action_t action) {
    m_move = m_reward == 0 ? m_move : (int) (rand01() * 3);
    if (action != m_move) {
        switch (m_move) {
        case 0:
            m_reward = action == 1 ? 2 : 0;
            break;
        case 1:
            m_reward = action == 2 ? 2 : 0;
            break;
        case 2:
            m_reward = action == 0 ? 2 : 0;
        }
    } else {
        m_reward = 1;
    }

    m_observation = m_move;
}

/* Pacman environment
 */
Pacman::Pacman(options_t &options) {
    if (options.count("power-pill-time") > 0) {
        strExtract(options["power-pill-time"], m_power_pill_time);
    }

    if (options.count("ghost-follow-time") > 0) {
        strExtract(options["ghost-follow-time"], m_ghost_follow_time);
    }

    //encoding the structure of the maze
    for (int i = 0; i < 21; i++)
        for (int j = 0; j < 19; j++)
            m_maze[i][j].isFreeCell = maze1[i][j];
    std::bitset<4> per;
    for (int i = 0; i < 21; i++)
        for (int j = 0; j < 19; j++) {
            if (m_maze[i][j].isFreeCell) {
                !m_maze[i - 1][j].isFreeCell ? per.set(3, 1) : per.set(3, 0);
                !m_maze[i][j + 1].isFreeCell ? per.set(2, 1) : per.set(2, 0);
                !m_maze[i + 1][j].isFreeCell ? per.set(1, 1) : per.set(1, 0);
                !m_maze[i][j - 1].isFreeCell ? per.set(0, 1) : per.set(0, 0);
                m_maze[i][j].wall = per.to_ulong() & INT_MAX;
                if (rand01() < 0.5)
                    m_maze[i][j].contents = 1;
                else
                    m_maze[i][j].contents = 0;
            } else {
                m_maze[i][j].wall = 15;
                m_maze[i][j].contents = -1;
            }
        }
    m_maze[9][0].wall = 10;
    m_maze[9][18].wall = 10;
    if (rand01() < 0.5)
        m_maze[9][0].contents = 1;
    else
        m_maze[9][0].contents = 0;
    if (rand01() < 0.5)
        m_maze[9][18].contents = 1;
    else
        m_maze[9][18].contents = 0;

    m_maze[1][3].contents = 2;
    m_maze[17][3].contents = 2;

    m_maze[1][15].contents = 2;
    m_maze[17][15].contents = 2;

    //initialising the ghosts
    for (int i = 0; i < 4; i++) {
        ghost[i].x = 8 + (int) (i / 2);
        ghost[i].y = 9 + (int) (i % 2);
        ghost[i].state = 1;
        m_ghost_timer[i] = -1 * m_ghost_follow_time;
        assert(m_maze[ghost[i].x][ghost[i].y].isFreeCell);
    }
    //initialising pacman
    m_pacman.x = 9;
    m_pacman.y = 0;
    m_pacman.state = 0;
    m_maze[13][9].contents = 0;

    m_power_pill_counter = 0;

    m_finished = false;

    m_observation = ((m_maze[m_pacman.x][m_pacman.y].wall & 15) << 12)
            | ((seeGhost() & 15) << 8) | ((smellFood() & 7) << 5)
            | ((seeFood() & 15) << 1) | m_pacman.state;
    m_reward = 60;
}

void Pacman::performAction(action_t action) {
    m_reward = 60;
    int pac_x_shift = (2 - (int) action) % 2;
    int pac_y_shift = ((int) action - 1) % 2;
    assert(action == 0 ? pac_x_shift == 0 : true);
    assert(pac_x_shift == 0 || pac_x_shift == -1 || pac_x_shift == 1);
    assert(pac_y_shift == 0 || pac_y_shift == -1 || pac_y_shift == 1);
    assert(pac_x_shift == 0 ? pac_y_shift != 0 : pac_y_shift == 0);

    //adding condition for loop back in row 9
    if (m_pacman.x == 9 && m_pacman.y == 0 && pac_y_shift == -1) {
        pac_y_shift = 18;
    } else if (m_pacman.x == 9 && m_pacman.y == 18 && pac_y_shift == 1) {
        pac_y_shift = -18;
    }

    //power pill effect fades when counter reaches 0
    if (m_power_pill_counter-- > 1)
        m_pacman.state = 0;

    if (m_maze[m_pacman.x + pac_x_shift][m_pacman.y + pac_y_shift].isFreeCell) {
        m_pacman.x = m_pacman.x + pac_x_shift;
        m_pacman.y = m_pacman.y + pac_y_shift;
        assert(m_maze[m_pacman.x][m_pacman.y].isFreeCell);
        if (!isCaught()) {
            if (m_maze[m_pacman.x][m_pacman.y].contents == 1) //pacman moved to a cell with food
                    {
                m_maze[m_pacman.x][m_pacman.y].contents = 0;
                m_reward += 10;
            } else if (m_maze[m_pacman.x][m_pacman.y].contents == 2) //POWER PILL!!!
                    {
                m_pacman.state = 1;
                m_power_pill_counter = m_power_pill_time;
            } else
                //empty cell
                m_reward += -1;
        } else //pacman ran into the ghost
        {
            m_reward += -50;
            m_finished = true;
            m_observation = ((m_maze[m_pacman.x][m_pacman.y].wall & 15) << 12)
                    | ((seeGhost() & 15) << 8) | ((smellFood() & 7) << 5)
                    | ((seeFood() & 15) << 1) | m_pacman.state;
            return;
        }
    } else
        //pacman ran into the wall
        m_reward += -10;

    for (int i = 0; i < 4; i++) {
        pos curr_ghost = ghost[i];
        if (ghost[i].state) {
            if (manhattan_dist(ghost[i], m_pacman) < 5) {
                if (m_ghost_timer[i]-- > 0
                        || m_ghost_timer[i] <= -1 * m_ghost_follow_time)
                        //ghost follows the pacman for the follow time set in the configuration file and then takes random moves for the same time
                                {
                    if (m_ghost_timer[i] < 0) {
                        m_ghost_timer[i] = m_ghost_follow_time;
                    }
                    manMove(i);
                }
            } else {
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

                    //adding condition for loop back in row 9
                    if (ghost[i].x == 9 && ghost[i].y == 0 && yshift == -1) {
                        yshift = 18;
                    } else if (ghost[i].x == 9 && ghost[i].y == 18
                            && yshift == 1) {
                        yshift = -18;
                    }

                    if (m_maze[ghost[i].x + xshift][ghost[i].y + yshift].isFreeCell) {
                        //check for collisions with other ghosts
                        bool ghost_collision = false;
                        for (int k = 0; k < 4; k++) {
                            if (ghost[k].x == (ghost[i].x + xshift)
                                    && ghost[k].y == (ghost[i].y + yshift)) {
                                ghost_collision = true;
                            }
                        }
                        if (!ghost_collision) {
                            movableCells[count][0] = ghost[i].x + xshift;
                            movableCells[count++][1] = ghost[i].y + yshift;
                        }
                    }
                }
                if (count != 0) {
                    int move = (int) (rand01() * count);
                    ghost[i].x = movableCells[move][0];
                    ghost[i].y = movableCells[move][1];
                }
            }
            assert(ghost[i].x >= 0 && ghost[i].x < 21);
            assert(ghost[i].y >= 0 && ghost[i].y < 19);

            //Confirm ghost is moving into a free cell
            assert(m_maze[ghost[i].x][ghost[i].y].isFreeCell);

            //confirm ghost is not moving into the same cell as another ghost or the pacman.
            for (int i_1 = 0; i_1 < 4; i_1++) {
                for (int i_2 = 0; i_2 < 4; i_2++) {
                    assert(
                            (ghost[i_1].x != ghost[i_2].x)
                                    || (ghost[i_1].y != ghost[i_2].y)
                                    || (i_1 == i_2));
                }
            }

        } else {
            eatenGhostMove(i);
        }
    }

    if (isCaught()) {
        m_reward += -50;
        m_finished = true;
    }

    m_observation = ((m_maze[m_pacman.x][m_pacman.y].wall & 15) << 12)
            | ((seeGhost() & 15) << 8) | ((smellFood() & 7) << 5)
            | ((seeFood() & 15) << 1) | m_pacman.state;

    //check if all food is eaten
    for (int x = 0; x < 21; x++)
        for (int y = 0; y < 19; y++)
            if (m_maze[x][y].contents == 1)
                return;

    //agent wins
    m_finished = true;
    m_reward += 100;
}

bool Pacman::isFinished(void) const {
    return m_finished;
}

void Pacman::envReset(void) {
    for (int i = 0; i < 21; i++)
        for (int j = 0; j < 19; j++) {
            if (m_maze[i][j].isFreeCell) {
                if (rand01() < 0.5)
                    m_maze[i][j].contents = 1;
                else
                    m_maze[i][j].contents = 0;
            }
        }

    m_maze[1][3].contents = 2;
    m_maze[17][3].contents = 2;

    m_maze[1][15].contents = 2;
    m_maze[17][15].contents = 2;

    //initialising the ghosts
    for (int i = 0; i < 4; i++) {
        ghost[i].x = 8 + (int) (i / 2);
        ghost[i].y = 9 + (int) (i % 2);
        ghost[i].state = 1;
        m_ghost_timer[i] = 0;
    }
    //initialising pacman
    m_pacman.x = 13;
    m_pacman.y = 9;
    m_pacman.state = 0;
    m_maze[13][9].contents = 0;

    m_finished = false;

    m_observation = ((m_maze[m_pacman.x][m_pacman.y].wall & 15) << 12)
            | ((seeGhost() & 15) << 8) | ((smellFood() & 7) << 5)
            | ((seeFood() & 15) << 1) | m_pacman.state;
    m_reward = 60;
}
