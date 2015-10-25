#include "main.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <cmath>

#include "agent.hpp"
#include "environment.hpp"
#include "search.hpp"
#include "util.hpp"
#include "predict.hpp"

// Globals for controlling experiments
static int total_cycles_mult_g;
static int def_total_cycles_g;
static int global_cycles_g = 0;
static int next_explore_switch_g;
static double explore_rate_g, explore_decay_g;
static bool explore_g;

// Streams for logging
namespace aixi {
std::ofstream log;        // A verbose human-readable log
}
std::ofstream compactLog; // A compact comma-separated value log

void processOptions(std::ifstream &in, options_t &options);
Environment * getEnvFromOptions(options_t options);
void setGlobalOptions(options_t options);

// The main agent/environment interaction loop
void mainLoop(Agent &ai, Environment &env, options_t &options) {

    // Determine termination lifetime
    bool terminate_check = options.count("terminate-lifetime") > 0;
    lifetime_t terminate_lifetime;
    bool explored = false;
    if (terminate_check) {
        strExtract(options["terminate-lifetime"], terminate_lifetime);
        assert(0 <= terminate_lifetime);
    }

    // Agent/environment interaction loop
    action_t action = 0;
    int cycle = 1;
    bool dobreak = false;

    while (true) {
        // check for agent termination
        if (terminate_check && ai.lifetime() > terminate_lifetime) {
            aixi::log << "info: terminating lifetime" << std::endl;
            break;
        }

        // Get a percept from the environment
        percept_t observation = env.getObservation();
        percept_t reward = env.getReward();

        // Reset the UCT
        ai.searchTreeReset();

        // Update agent's environment model with the new percept
        ai.modelUpdate(observation, reward);

        // If the episode is finished, still do a modelUpdate with action
        if (env.isFinished()) {
            dobreak = true;
        }

        // Determine best exploitive action, or explore_g
        if (global_cycles_g == next_explore_switch_g) {
            if (explore_g) {
                std::cout << "Starting evaluation phase: " << global_cycles_g
                        << std::endl;
                explore_g = false;
                next_explore_switch_g += def_total_cycles_g
                        * total_cycles_mult_g / 10;
            } else {
                std::cout << "Starting training phase: " << global_cycles_g
                        << std::endl;
                explore_g = true;
                next_explore_switch_g += def_total_cycles_g
                        * total_cycles_mult_g / 5;
            }
        }

        explored = false;
        // Either explore or search
        if (explore_g && rand01() < explore_rate_g) {
            explored = true;
            action = ai.genRandomAction();
        } else {
            // We need to accumulate some history before calling search
            if (ai.historySize() >= ai.maxTreeDepth()) {
                action = search(ai);
            } else {
                action = ai.genRandomAction();
            }
        }

        // Update agent's environment model with the chosen action
        ai.modelUpdate(action);

        // Log this turn
        aixi::log << "action: " << action << std::endl;
        aixi::log << "explored: " << (explored ? "yes" : "no") << std::endl;
        aixi::log << "explore_rate_g: " << explore_rate_g << std::endl;
        aixi::log << "cycle: " << cycle << std::endl;
        aixi::log << "global_cycle: " << global_cycles_g << std::endl;
        aixi::log << "observation: " << observation << std::endl;
        aixi::log << "reward: " << reward << std::endl;
        aixi::log << "total reward: " << ai.reward() << std::endl;
        aixi::log << "average reward: " << ai.averageReward() << std::endl;
        aixi::log << "Search tree size: "
                << ai.searchTree()->getDecisionNodeInfo() << std::endl;
        aixi::log << "Global cycle number: " << global_cycles_g << std::endl;

        // Log the data in a more compact form
        compactLog << global_cycles_g << ", " << cycle << ", " << observation
                << ", " << reward << ", " << action << ", " << explore_g << ", "
                << explored << ", " << explore_rate_g << ", " << ai.reward()
                << ", " << ai.averageReward() << ", " << env.isFinished()
                << std::endl;

        // Break out before performing another action, since the environment is finished.
        if (dobreak) {
            break;
        }

        // Do the action!
        env.performAction(action);

        if (explore_g)
            explore_rate_g *= explore_decay_g;

        cycle++;
        global_cycles_g++;
    }
}

int main(int argc, char *argv[]) {

    if (argc < 2 || argc > 3) {
        std::cerr << "ERROR: Incorrect number of arguments" << std::endl;
        std::cerr
                << "The first argument should indicate the location of the configuration file and the second (optional) argument should indicate the location of the configuration file of the second game to run."
                << std::endl;
        return -1;
    }

    // Set up logging
    std::string log_file = "log";
    aixi::log.open((log_file + ".log").c_str());
    compactLog.open((log_file + ".csv").c_str());

    // Print header to compactLog
    compactLog
            << "global_cycle, cycle, observation, reward, action, explore_on, explored, explore_rate_g, total reward, average reward, end of game"
            << std::endl;

    options_t options;

    // Default configuration values
    options["ct-depth"] = "3";				// max context tree depth
    options["agent-horizon"] = "16";		// agent max search horizon
    options["exploration"] = "0";			// do not explore_g
    options["explore-decay"] = "1.0";		// exploration rate does not decay
    options["timeout"] = "0.5"; 			// timeout
    options["UCB-weight"] = "1.41"; 		// UCB weight
    options["def-total-cycles"] = "1000"; // total number of cycles for 1 experiment

    // Read configuration options
    std::ifstream conf(argv[1]);
    if (!conf.is_open()) {
        std::cerr << "ERROR: Could not open file '" << argv[1]
                << "' now exiting" << std::endl;
        return -1;
    }
    processOptions(conf, options);
    conf.close();

    // Read the optional second config, if required
    options_t options1;
    if (argc == 3) {
        std::ifstream conf1(argv[2]);
        if (!conf1.is_open()) {
            std::cerr << "ERROR: Could not open file '" << argv[2]
                    << "' now exiting" << std::endl;
            return -1;
        }
        processOptions(conf1, options1);
        conf1.close();
    }

    // Set up the environment
    Environment * env = getEnvFromOptions(options);

    // Set up the agent
    Agent ai(options);

    // Set the global experiment options
    setGlobalOptions(options);

    // Run the main agent/environment interaction loop
    while (global_cycles_g < 2 * def_total_cycles_g * total_cycles_mult_g) {
        mainLoop(ai, *env, options);
        env->envReset();
        ai.searchTreeReset();
    }
    // Run on game 2, then again on game 1.
    if (argc == 3) {
        std::cout << "Finished game 1, starting game 2" << std::endl;
        aixi::log << "-----------------" << std::endl;
        compactLog << "-----------------" << std::endl;

        global_cycles_g = 0;
        ai.setOptions(options1);
        setGlobalOptions(options1);

        env = getEnvFromOptions(options1);

        while (global_cycles_g < 2 * def_total_cycles_g * total_cycles_mult_g) {
            mainLoop(ai, *env, options1);
            env->envReset();
            ai.searchTreeReset();
        }

        global_cycles_g = 0;
        ai.setOptions(options);

        std::cout << "Finished game 2, returning to game 1" << std::endl;
        aixi::log << "-----------------" << std::endl;
        compactLog << "-----------------" << std::endl;

        setGlobalOptions(options);
        env = getEnvFromOptions(options);

        while (global_cycles_g < 2 * def_total_cycles_g * total_cycles_mult_g) {
            mainLoop(ai, *env, options);
            env->envReset();
            ai.searchTreeReset();
        }
    }

    std::cout << "Done!" << std::endl;
    aixi::log.close();
    compactLog.close();

    return 0;
}

// Populate the 'options' map based on 'key=value' pairs from an input stream
void processOptions(std::ifstream &in, options_t &options) {
    std::string line;
    size_t pos;

    for (int lineno = 1; in.good(); lineno++) {
        std::getline(in, line);

        // Ignore # comments
        if ((pos = line.find('#')) != std::string::npos) {
            line = line.substr(0, pos);
        }

        // Remove whitespace
        while ((pos = line.find(" ")) != std::string::npos)
            line.erase(line.begin() + pos);
        while ((pos = line.find("\t")) != std::string::npos)
            line.erase(line.begin() + pos);

        // Split into key/value pair at the first '='
        pos = line.find('=');
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        // Check that we have parsed a valid key/value pair. Warn on failure or
        // set the appropriate option on success.
        if (pos == std::string::npos) {
            std::cerr << "WARNING: processOptions skipping line " << lineno
                    << " (no '=')" << std::endl;
        } else if (key.size() == 0) {
            std::cerr << "WARNING: processOptions skipping line " << lineno
                    << " (no key)" << std::endl;
        } else if (value.size() == 0) {
            std::cerr << "WARNING: processOptions skipping line " << lineno
                    << " (no value)" << std::endl;
        } else {
            options[key] = value; // Success!
            std::cout << "OPTION: '" << key << "' = '" << value << "'"
                    << std::endl;
        }
    }
}

// Construct the environmnet required by the options
Environment * getEnvFromOptions(options_t options) {
    std::string environment_name = options["environment"];
    if (environment_name == "coin-flip") {
        return new CoinFlip(options);
    } else if (environment_name == "cheese-maze") {
        return new CheeseMaze(options);
    } else if (environment_name == "extended-tiger") {
        return new ExtTiger(options);
    } else if (environment_name == "tictactoe") {
        return new TicTacToe(options);
    } else if (environment_name == "biased-rock-paper-scissor") {
        return new BRockPaperScissors(options);
    } else if (environment_name == "pacman") {
        return new Pacman(options);
    } else {
        std::cerr << "ERROR: unknown environment '" << environment_name << "'"
                << std::endl;
        return NULL;
    }
}

// Helper function to extract global experiment variables from options
void setGlobalOptions(options_t options) {
    // Determine experiment length options
    strExtract(options["total-cycles-mult"], total_cycles_mult_g);
    strExtract(options["def-total-cycles"], def_total_cycles_g);
    next_explore_switch_g = global_cycles_g
            + (def_total_cycles_g * total_cycles_mult_g / 5);

    // Determine exploration options
    explore_g = options.count("exploration") > 0;
    if (explore_g) {
        strExtract(options["exploration"], explore_rate_g);
        strExtract(options["explore-decay"], explore_decay_g);
        assert(0.0 <= explore_rate_g && explore_rate_g <= 1.0);
        assert(0.0 <= explore_decay_g && explore_decay_g <= 1.0);
    }
}
