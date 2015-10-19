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
	//for (unsigned int cycle = 1; !env.isFinished(); cycle++) {
	action_t action = 0;
	for (unsigned int cycle = 1; cycle <= 100000; cycle++) {

		// check for agent termination
		if (terminate_check && ai.lifetime() > terminate_lifetime) {
			aixi::log << "info: terminating lifetiment" << std::endl;
			break;
		}

		// Get a percept from the environment
		percept_t observation = env.getObservation();
		percept_t reward = env.getReward();
		int maxeroo = 1000;

		if (cycle == maxeroo) {
			return;
		}

		if (cycle < maxeroo - 1) {
			observation = 1;
			reward = (action == 1 ? 1 : 0);
		} else {
			observation = 0;
			reward = 0;
		}

		//std::cout << "Reward = " << reward << std::endl;

		// Update agent's environment model with the new percept
		int foo = maxeroo - 10;
//		if (cycle > foo) {
//			double bar = ai.getProbNextSymbol();
//			if (isnan(bar)) {
//				return;
//			}
//			std::cout << "before model update with o_r head prob: " << ai.getProbNextSymbol() << std::endl;
//		}

		ai.modelUpdate(observation, reward); // TODO: implement in agent.cpp
//		if (cycle > foo) {
//			std::cout << "after model update with o_r head prob: " << ai.getProbNextSymbol() << std::endl;
//		}

		//std::cout << "Hello" << std::endl;

		//std::cout << "Model update with real OR bits ^^^^^^^^^^^^^^^^^^^^^"<<std::endl;

		// Determine best exploitive action, or explore

		if (cycle > foo) {
			//ai.getContextTree()->print();
			ai.getContextTree()->debugTree1();
			std::cout << "action: (" << action << "), observation: ("
					<< observation << "), reward = (" << reward
					<< "), head prob: " << " " << pow(2, ai.getProbNextSymbol())
					<< std::endl;
		}

		bool explored = false;
		if (explore && rand01() < explore_rate) {
			explored = true;
			action = ai.genRandomAction();
		} else {
			if (ai.historySize() >= ai.maxTreeDepth()) {
				//action = search(ai, 0.01);
				action = ai.genRandomAction();
			} else {
				std::cout << "Generating random action" << std::endl;
				action = ai.genRandomAction();
			}
		}
		//action = 1;
		//std::cout << "Agent performed action: " << action << std::endl;

		// Send an action to the environment
		env.performAction(action); // TODO: implement for each environment
//		if (cycle > foo) {
//			std::cout << "after action performed head prob: " << ai.getProbNextSymbol() << std::endl;
//		}
				//std::cout << "Action performed======================"<<std::endl;

		// Update agent's environment model with the chosen action
		ai.modelUpdate(action); // TODO: implement in agent.cpp

		//ai.getContextTree()->debugTree();
		//ai.getContextTree()->printRootKTAndWeight();
		//std::cout << "Model update with performed action *******************"<<std::endl;
		//ai.getContextTree()->debugTree();
		// Log this turn
		aixi::log << "-----" << std::endl;
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

		//std::cout << "Current cycle : " << cycle << std::endl;
		// Print to standard output when cycle == 2^n
		// if ((cycle & (cycle - 1)) == 0) {
		if (cycle % 1 == 0) {

			std::cout << "cycle: " << cycle << std::endl;
//			std::cout << "head prob: " << ai.getProbNextSymbol() << std::endl;
			//std::cout << "average reward: " << ai.averageReward() << std::endl;

			if (explore) {
				//std::cout << "explore rate: " << explore_rate << std::endl;
			}
		}

		// Update exploration rate
		if (explore)
			explore_rate *= explore_decay;

	}

// Print summary to standard output
	std::cout << std::endl << std::endl << "SUMMARY" << std::endl;
	std::cout << "agent lifetime: " << ai.lifetime() << std::endl;
	std::cout << "average reward: " << ai.averageReward() << std::endl;

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

void testPredict() {
	ContextTree* ct = new ContextTree(30);
	symbol_list_t sym_list;

	encode(sym_list, 1031234, 30);
	ct->update(sym_list);
	ct->updateHistory(sym_list);
	ct->debugTree();

	ct->revert();
	ct->updateHistory(sym_list);
	ct->debugTree();
}

void testAgentAndPredict(Agent *agent) {
	symbol_list_t sym_list;
	/*
	 action_t action = 0x5;
	 percept_t obs = 0x0795;
	 percept_t rew = 0xE;
	 */
	action_t action = 0x1;
	percept_t obs = 0x0795;
	percept_t rew = 0xE;
	ModelUndo *mu = new ModelUndo(*agent);

	agent->modelUpdate(obs, rew);
//agent->getContextTree()->debugTree();

	agent->modelUpdate(action);
	agent->getContextTree()->debugTree();

	agent->modelUpdate(obs, rew);
// agent->getContextTree()->debugTree();

	agent->modelUpdate(action);
//agent->getContextTree()->debugTree();

	agent->modelRevert(*mu);
	agent->getContextTree()->debugTree();
}

#ifdef __DEBUG__

/* 
 * Main() for Debuging 
 * Usage: g++ -o aixi *.cpp -D__DEBUG__
 */

int main(int argc, char *argv[]) {

// Load configuration options
	options_t options;

// Default configuration values
	options["environment"] = "test";
	options["ct-depth"] = "3";
	options["agent-horizon"] = "16";
	options["exploration"] = "0";// do not explore
	options["explore-decay"] = "1.0";// exploration rate does not decay

// Set up the environment
	Environment *env;

// TODO: instantiate the environment based on the "environment-name"
// option. For any environment you do not implement you may delete the
// corresponding if statement.
// NOTE: you may modify the options map in order to set quantities such as
// the reward-bits for each particular environment. See the coin-flip
// experiment for an example.
	std::string environment_name = options["environment"];
	if (environment_name == "coin-flip") {
		env = new CoinFlip(options);
		options["agent-actions"] = "2";
		options["observation-bits"] = "1";
		options["reward-bits"] = "1";
	}
	else if (environment_name == "test") {
		//env = new CoinFlip(options);
		options["ct-depth"] = "3";
		options["agent-actions"] = "2";
		options["observation-bits"] = "1";
		options["reward-bits"] = "1";
		options["action-bits"] = "1";
	}
	else if (environment_name == "1d-maze") {
		// TODO: instantiate "env" (if appropriate)
	}
	else if (environment_name == "cheese-maze") {
		// TODO: instantiate "env" (if appropriate)
	}
	else if (environment_name == "tiger") {
		// TODO: instantiate "env" (if appropriate)
	}
	else if (environment_name == "extended-tiger") {
		// TODO: instantiate "env" (if appropriate)
	}
	else if (environment_name == "4x4-grid") {
		// TODO: instantiate "env" (if appropriate)
	}
	else if (environment_name == "tictactoe") {
		// TODO: instantiate "env" (if appropriate)
	}
	else if (environment_name == "biased-rock-paper-scissor") {
		// TODO: instantiate "env" (if appropriate)
	}
	else if (environment_name == "kuhn-poker") {
		// TODO: instantiate "env" (if appropriate)
	}
	else if (environment_name == "pacman") {
		// TODO: instantiate "env" (if appropriate)
	}
	else {
		std::cerr << "ERROR: unknown environment '" << environment_name << "'" << std::endl;
		return -1;
	}

// Set up the agent
	Agent ai(options);

	testAgentAndPredict(&ai);
	/*
	 // Run the main agent/environment interaction loop
	 mainLoop(ai, *env, options);
	 aixi::log.close();
	 compactLog.close();
	 */
	return 0;
}

#else 

/* 
 * Main function
 */
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
	options["exploration"] = "0";     // do not explore
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

// TODO: instantiate the environment based on the "environment-name"
// option. For any environment you do not implement you may delete the
// corresponding if statement.
// NOTE: you may modify the options map in order to set quantities such as
// the reward-bits for each particular environment. See the coin-flip
// experiment for an example.
	std::string environment_name = options["environment"];
	if (environment_name == "coin-flip") {
		env = new CoinFlip(options);
		options["agent-actions"] = "2";
		options["observation-bits"] = "1";
		options["reward-bits"] = "1";
	} else if (environment_name == "1d-maze") {
		// TODO: instantiate "env" (if appropriate)
	} else if (environment_name == "cheese-maze") {
		env = new CheeseMaze(options);
		options["agent-actions"] = "4";
		options["observation-bits"] = "4";
		options["reward-bits"] = "5";
	} else if (environment_name == "tiger") {
		// TODO: instantiate "env" (if appropriate)
	}
	/*actions:          0 = stand
	 *                  1 = listen
	 *                  2 = open left door
	 *                  3 = open right door
	 * 
	 * observations:    0 = nothing known
	 *                  1 = tiger behind right door
	 *                  2 = tiger behind left door
	 */
	else if (environment_name == "extended-tiger") {
		env = new ExtTiger(options);
		options["agent-actions"] = "4";
		options["observation-bits"] = "2";
		options["reward-bits"] = "8";
	} else if (environment_name == "4x4-grid") {
		// TODO: instantiate "env" (if appropriate)
	} else if (environment_name == "tictactoe") {
		env = new TicTacToe(options);
		options["agent-actions"] = "9";
		options["observation-bits"] = "18";
		options["reward-bits"] = "3";
	}
	/*
	 Action 
	 0 : rock
	 1 : paper
	 2 : scissors
	 */
	else if (environment_name == "biased-rock-paper-scissor") {
		env = new BRockPaperScissors(options);
		options["agent-actions"] = "3";
		options["observation-bits"] = "3";
		options["reward-bits"] = "3";
	} else if (environment_name == "kuhn-poker") {
		// TODO: instantiate "env" (if appropriate)
	} else if (environment_name == "pacman") {
		// TODO: instantiate "env" (if appropriate)
	} else {
		std::cerr << "ERROR: unknown environment '" << environment_name << "'"
				<< std::endl;
		return -1;
	}

// Set up the agent
	Agent ai(options);

// Run the main agent/environment interaction loop
	mainLoop(ai, *env, options);

	aixi::log.close();
	compactLog.close();

	return 0;
}

#endif

