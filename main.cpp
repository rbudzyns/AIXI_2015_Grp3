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

// Streams for logging
namespace aixi {
std::ofstream log;        // A verbose human-readable log
}
std::ofstream compactLog; // A compact comma-separated value log

void processOptions(std::ifstream &in, options_t &options);
// The main agent/environment interaction loop
void mainLoop(Agent &ai, Environment &env, options_t &options) {

	// Determine exploration options
	bool explore = options.count("exploration") > 0;
	double explore_rate, explore_decay;
	if (explore) {
		strExtract(options["exploration"], explore_rate);
		strExtract(options["explore-decay"], explore_decay);
		assert(0.0 <= explore_rate && explore_rate <= 1.0);
		assert(0.0 <= explore_decay && explore_decay <= 1.0);
	}

	// Determine termination lifetime
	bool terminate_check = options.count("terminate-lifetime") > 0;
	lifetime_t terminate_lifetime;
	if (terminate_check) {
		strExtract(options["terminate-lifetime"], terminate_lifetime);
		assert(0 <= terminate_lifetime);
	}

	// Agent/environment interaction loop

	action_t action = 0;
	int cycle = 1;
	while (true) {
		// check for agent termination
		if (terminate_check && ai.lifetime() > terminate_lifetime) {
			aixi::log << "info: terminating lifetiment" << std::endl;
			break;
		}

		// Get a percept from the environment
		percept_t observation = env.getObservation();
		percept_t reward = env.getReward();

		// Update agent's environment model with the new percept

		ai.modelUpdate(observation, reward);

		if (env.isFinished()) {
			break;
		}

		// Determine best exploitive action, or explore

		bool explored = false;
		if (explore && rand01() < explore_rate) {
			explored = true;
			action = ai.genRandomAction();
		} else {

			if (ai.historySize() >= ai.maxTreeDepth()) {
				action = search(ai, 0.05);

			} else {
				action = ai.genRandomAction();
				std::cout << "Generating random action: " << action
						<< std::endl;
			}
		}

		// Send an action to the environment
		env.performAction(action);

		// Update agent's environment model with the chosen action
		ai.modelUpdate(action);
		// Log this turn

		aixi::log << "cycle: " << cycle << std::endl;
		aixi::log << "observation: " << observation << std::endl;
		aixi::log << "reward: " << reward << std::endl;
		aixi::log << "action: " << action << std::endl;
		aixi::log << "explored: " << (explored ? "yes" : "no") << std::endl;
		aixi::log << "explore rate: " << explore_rate << std::endl;
		aixi::log << "total reward: " << ai.reward() << std::endl;
		aixi::log << "average reward: " << ai.averageReward() << std::endl;

		// Log the data in a more compact form
		compactLog << cycle << ", " << observation << ", " << reward << ", "
				<< action << ", " << explored << ", " << explore_rate << ", "
				<< ai.reward() << ", " << ai.averageReward() << std::endl;

		// Update exploration rate
		if (explore)
			explore_rate *= explore_decay;

		cycle++;
	}

// Print summary to standard output

	std::cout << std::endl << std::endl << "Episode finished. Summary:"
			<< std::endl;
	std::cout << "agent lifetime: " << ai.lifetime() << std::endl;
	std::cout << "average reward: " << ai.averageReward() << std::endl;

}

int main(int argc, char *argv[]) {

	if (argc < 2 || argc > 3) {
		std::cerr << "ERROR: Incorrect number of arguments" << std::endl;
		std::cerr
				<< "The first argument should indicate the location of the configuration file and the second (optional) argument should indicate the file to log to."
				<< std::endl;
		return -1;
	}

// Set up logging

	std::string log_file = argc < 3 ? "log" : argv[2];
	aixi::log.open((log_file + ".log").c_str());
	compactLog.open((log_file + ".csv").c_str());

// Print header to compactLog

	compactLog
			<< "cycle, observation, reward, action, explored, explore_rate, total reward, average reward"
			<< std::endl;

	options_t options;

// Default configuration values

	options["ct-depth"] = "3";
	options["agent-horizon"] = "16";
	options["exploration"] = "0"; // do not explore
	options["explore-decay"] = "1.0"; // exploration rate does not decay

// Read configuration options

	std::ifstream conf(argv[1]);
	if (!conf.is_open()) {
		std::cerr << "ERROR: Could not open file '" << argv[1]
				<< "' now exiting" << std::endl;
		return -1;
	}
	processOptions(conf, options);
	conf.close();

// Set up the environment
	Environment *env;

// option. For any environment you do not implement you may delete the
// corresponding if statement.
// NOTE: you may modify the options map in order to set quantities such as
// the reward-bits for each particular environment. See the coin-flip
// experiment for an example.

	std::string environment_name = options["environment"];
	if (environment_name == "coin-flip") {
		env = new CoinFlip(options);
	} else if (environment_name == "cheese-maze") {
		env = new CheeseMaze(options);
	} else if (environment_name == "extended-tiger") {
		env = new ExtTiger(options);
	} else if (environment_name == "tictactoe") {
		env = new TicTacToe(options);
	} else if (environment_name == "biased-rock-paper-scissor") {
		env = new BRockPaperScissors(options);
	} else if (environment_name == "pacman") {
		env = new Pacman(options);
	} else {
		std::cerr << "ERROR: unknown environment '" << environment_name << "'"
				<< std::endl;
		return -1;
	}

// Set up the agent
	Agent ai(options);

// Run the main agent/environment interaction loop
	int n_episodes = 100;
	for (int i = 0; i < n_episodes; i++) {
		mainLoop(ai, *env, options);
		env->envReset();
		ai.newEpisode();
	}

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
