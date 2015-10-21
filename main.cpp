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

	//double total_head_prob = 0.0;

	while (true) {
		//std::cout << "GlobalCycles: " << global_cycles << std::endl;
		//std::cout << "HistSize: " << ai.historySize() << " MaxTreeDepth: " << ai.maxTreeDepth() << std::endl;

		// check for agent termination
		if (terminate_check && ai.lifetime() > terminate_lifetime) {
			aixi::log << "info: terminating lifetime" << std::endl;
			break;
		}

		// Get a percept from the environment
		percept_t observation = env.getObservation();
		percept_t reward = env.getReward();

		aixi::log << "cycle: " << global_cycles_g << std::endl;
		aixi::log << "observation: " << observation << std::endl;
		aixi::log << "reward: " << reward << std::endl;

		// Update agent's environment model with the new percept

		ai.modelUpdate(observation, reward);

		if (env.isFinished()) {
			break;
		}

		// Determine best exploitive action, or explore_g
		if (global_cycles_g == next_explore_switch_g) {
			if (explore_g) {
				std::cout << "Starting evaluation phase: " << global_cycles_g
						<< std::endl;
				explore_g = false;
				next_explore_switch_g += def_total_cycles_g
						* total_cycles_mult_g * 2 / 5;
			} else {
				std::cout << "Starting training phase: " << global_cycles_g
						<< std::endl;
				explore_g = true;
				next_explore_switch_g += def_total_cycles_g
						* total_cycles_mult_g / 5;
			}
		}
		explored = false;
		if (explore_g && rand01() < explore_rate_g) {
			explored = true;
			action = ai.genRandomAction();
		} else {
			if (ai.historySize() >= ai.maxTreeDepth()) {
				action = search(ai);
			} else {
				action = ai.genRandomAction();
				// std::cout << "Generating random action: " << action
				//		<< std::endl;
			}
		}
		env.performAction(action);

		// Update agent's environment model with the chosen action
		ai.modelUpdate(action);

		// Log this turn

		aixi::log << "action: " << action << std::endl;
		aixi::log << "explored: " << (explored ? "yes" : "no") << std::endl;
		aixi::log << "explore_rate_g: " << explore_rate_g << std::endl;
		aixi::log << "total reward: " << ai.reward() << std::endl;
		aixi::log << "average reward: " << ai.averageReward() << std::endl;

		// Log the data in a more compact form
		compactLog << global_cycles_g << ", " << observation << ", " << reward << ", "
				<< action << ", " << explored << ", " << explore_rate_g << ", "
				<< ai.reward() << ", " << ai.averageReward() << std::endl;

		// Update exploration rate
		if (explore_g)
			explore_rate_g *= explore_decay_g;

//		std::cout << "agent lifetime: " << ai.lifetime() + 1 << std::endl;
//		std::cout << "average reward: " << ai.averageReward() << std::endl;
//		int cyclebits = 16;
//		int buffer = 300*cyclebits;
//		if(cycle > buffer) {
//			total_head_prob += ai.getProbNextSymbol();
//			std::cout<< total_head_prob/(cycle-buffer)<<std::endl;
//		}
		cycle++;
		global_cycles_g++;
	}

// Print summary to standard output

//	std::cout << std::endl << std::endl << "Episode finished. Summary:"
//			<< std::endl;
//	std::cout << "agent lifetime: " << ai.lifetime() + 1 << std::endl;
//	std::cout << "average reward: " << ai.averageReward() << std::endl;

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
			<< "cycle, observation, reward, action, explored, explore_rate_g, total reward, average reward"
			<< std::endl;

	options_t options;

// Default configuration values

	options["ct-depth"] = "3";				// max context tree depth
	options["agent-horizon"] = "16";		// agent max search horizon
	options["exploration"] = "0";			// do not explore_g
	options["explore_g-decay"] = "1.0";		// exploration rate does not decay
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

//	mainLoop(ai, *env, options);

// Run the main agent/environment interaction loop
	int n_episodes = 10000;
	strExtract(options["total-cycles-mult"], total_cycles_mult_g);
	strExtract(options["def-total-cycles"], def_total_cycles_g);
	next_explore_switch_g = global_cycles_g
			+ (def_total_cycles_g * total_cycles_mult_g / 5);

	// Determine exploration options
	explore_g = options.count("exploration") > 0;
	if (explore_g) {
		strExtract(options["exploration"], explore_rate_g);
		strExtract(options["explore_g-decay"], explore_decay_g);
		assert(0.0 <= explore_rate_g && explore_rate_g <= 1.0);
		assert(0.0 <= explore_decay_g && explore_decay_g <= 1.0);
	}

	for (int i = 0; i < def_total_cycles_g * total_cycles_mult_g; i++) {
		mainLoop(ai, *env, options);
		env->envReset();
		//ai.contextTree()->debugTree();
		ai.newEpisode();
		//ai.contextTree()->debugTree();
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
