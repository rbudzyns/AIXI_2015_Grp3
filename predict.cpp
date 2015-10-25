#include "predict.hpp"
#include "util.hpp"

#include <cassert>
#include <cmath>
#include <stdio.h>

CTNode::CTNode(void) :
		m_log_prob_est(0.0), m_log_prob_weighted(0.0) {
	m_count[0] = 0;
	m_count[1] = 0;
	m_child[0] = NULL;
	m_child[1] = NULL;
}

CTNode::~CTNode(void) {
	if (m_child[0])
		delete m_child[0];
	if (m_child[1])
		delete m_child[1];
}

void CTNode::print(void) const {
	std::cout << "Printing node..." << std::endl;
}

// number of descendants of a node in the context tree
size_t CTNode::size(void) const {

	size_t rval = 1;
	rval += child(false) ? child(false)->size() : 0;
	rval += child(true) ? child(true)->size() : 0;
	return rval;
}

// compute the logarithm of the KT-estimator update multiplier
double CTNode::logKTMul(symbol_t sym) const {
	return log2((m_count[sym] + 0.5) / (m_count[0] + m_count[1] + 1));;
}

// Calculate the logarithm of the weighted block probability.
void CTNode::updateLogProbability(void) {

	if (m_child[0] == NULL) {
		if (m_child[1] == NULL) {
			// Calculate weighted log probability when the node is leaf
			m_log_prob_weighted = m_log_prob_est;
		} else {
			// Calculate weighted log probability when the node has no right child(sym 0)
			m_log_prob_weighted = log2(
					pow(2, m_log_prob_est - m_child[1]->m_log_prob_weighted)
							+ 1) + m_child[1]->m_log_prob_weighted - 1;
		}
	} else {
		if (m_child[1] == NULL) {
			// Calculate weighted log probability when the node has no left child(sym 1)
			m_log_prob_weighted = log2(
					pow(2, m_log_prob_est - m_child[0]->m_log_prob_weighted)
							+ 1) + m_child[0]->m_log_prob_weighted - 1;
		} else {
			// Calculate weighted log probability with both children(sym 1 and sym 2)
			m_log_prob_weighted = log2(
					pow(2,
							m_log_prob_est
									- (m_child[0]->m_log_prob_weighted
											+ m_child[1]->m_log_prob_weighted))
							+ 1)
					+ (m_child[0]->m_log_prob_weighted
							+ m_child[1]->m_log_prob_weighted) - 1;
		}
	}

}

// Update the node after having observed a new symbol.
void CTNode::update(const symbol_t symbol) {
	// Update the KT estimate for this node
	m_log_prob_est += logKTMul(symbol);

	// Update 0 or 1 counter for this node
	m_count[symbol]++;
	// Update the weighted probability of this node
	updateLogProbability();
}

// Return the node to its state immediately prior to the last update.
void CTNode::revert(const symbol_t symbol) {
	// Decrement the count for the symbol
	m_count[symbol]--;
	if (m_count[0] == 0 && m_count[1] == 0) {
		return;
	}
	// Reset the KT estimate on the node and then weighted log probability
	m_log_prob_est -= logKTMul(symbol);
	updateLogProbability();
}

// create a context tree of specified maximum depth
ContextTree::ContextTree(size_t depth) :
		m_root(new CTNode()), m_depth(depth) {
	return;
}

ContextTree::~ContextTree(void) {
	if (m_root)
		delete m_root;
}

// reset the history to the last ct-depth size of history
void ContextTree::resetHistory(void) {
	m_history.erase(m_history.begin(), m_history.begin() + (m_history.size() - m_depth) );
}

// clear the entire context tree
void ContextTree::clear(void) {
	m_history.clear();
	if (m_root)
		delete m_root;
	m_root = new CTNode();
}

void ContextTree::print(void) {
	std::cout << "Printing tree..." << std::endl;
	m_root->print();
}

// updates the context tree with a new binary symbol
void ContextTree::update(const symbol_t sym) {
	std::vector<CTNode*> context_path;
	CTNode* current = m_root;

	// Create a list of the path tranversed
	// bitfix=0, as the last history symbol is also used
	walkAndGeneratePath(0, context_path, &current);

	while (context_path.empty() != true) {
		// Update the nodes along the context path bottom up
		current->update(sym);
		// Move one level up, along the context path
		current = context_path.back();
		context_path.pop_back();
	}
	// Update the root node
	current->update(sym);
	updateHistory(sym);
}

// update the context tree with a list of symbols
void ContextTree::update(const symbol_list_t &symbol_list) {
	for (size_t i = 0; i < symbol_list.size(); i++) {
		// Update one symbol at a time in the block of symbols
		update(symbol_list[i]);
	}
}

// updates the history statistics, without touching the context tree
void ContextTree::updateHistory(const symbol_list_t &symbol_list) {
	for (size_t i = 0; i < symbol_list.size(); i++) {
		m_history.push_back(symbol_list[i]);
	}
}

void ContextTree::updateHistory(const symbol_t sym) {
	m_history.push_back(sym);
}

// Create a path list from root node to one level above the leaf node
// along the context, used for updating and reverting the Context tree
// from bottom up
void ContextTree::walkAndGeneratePath(int bit_fix,
		std::vector<CTNode*> &context_path, CTNode **current) {
	int traverse_depth = 0;
	int cur_history_sym;

	// Store the path of current context in the traverse list
	while (traverse_depth < m_depth) {
		cur_history_sym = m_history.at(
				bit_fix + (m_history.size() - 1) - traverse_depth);

		// Add a new context node, if it is a new context
		if ((*current)->m_child[cur_history_sym] == NULL) {
			CTNode* node = new CTNode();
			(*current)->m_child[cur_history_sym] = node;

		}
		// Store the current node on the context path,
		// used when updating and reverting the Context tree bottom up
		context_path.push_back((*current));

		(*current) = (*current)->m_child[cur_history_sym];
		traverse_depth++;
	}
}

// Revert the CT to its state prior to the most recently observed symbol
void ContextTree::revert(void) {
	std::vector<CTNode*> context_path;
	CTNode* current = m_root;
	int cur_depth = m_depth;

	// Create a list of the path tranversed
	// bitfix=-1, as the last history symbol is not
	walkAndGeneratePath(-1, context_path, &current);

	while (context_path.empty() != true) {
		// Update the nodes along the context path bottom up
		current->revert(m_history.at(m_history.size() - 1));

		if (current->m_count[0] == 0 && current->m_count[1] == 0) {
			// Delete the context node when there is no context
			delete current;
			// Reset the parent's child node pointer for the symbol
			current = context_path.back();
			context_path.pop_back();
			current->m_child[m_history.at(m_history.size() - cur_depth -1)] = NULL;
		} else {
			// Update the nodes one level up
			current = context_path.back();
			context_path.pop_back();
		}
		cur_depth--;
	}
	// Revert the root node
	current->revert(m_history.at(m_history.size() - 1));
}

// shrinks the history down to a former size
void ContextTree::revertHistory(size_t newsize) {

	assert(newsize <= m_history.size());
	while (m_history.size() > newsize)
		m_history.pop_back();
}

// Calculate the probability of the next symbol given the next history P(x[i]=sym|h)
double ContextTree::getLogProbNextSymbolGivenH(symbol_t sym) {
	double prob_log_next_bit, new_log_block_prob, last_log_block_prob;

	last_log_block_prob = logBlockProbability();
	// To calculate the root probability as if the next symbol was 0
	update(sym);
	new_log_block_prob = logBlockProbability();

	prob_log_next_bit = new_log_block_prob - last_log_block_prob;
	// Remove the recently added 0, which was used for calculating the root prob
	revert();
	revertHistory(m_history.size() - 1);

	return prob_log_next_bit;
}

// Calculate the probability of the next symbol given the next history
// P(x[i]=sym|h) and update
double ContextTree::getLogProbNextSymbolGivenHWithUpdate(symbol_t sym) {
	double prob_log_next_bit, new_log_block_prob, last_log_block_prob;

	last_log_block_prob = logBlockProbability();
	// To calculate the root probability as if the next symbol was 0
	update(sym);
	new_log_block_prob = logBlockProbability();
	prob_log_next_bit = new_log_block_prob - last_log_block_prob;
	// Remove the recently added 0, which was used for calculating the root prob

	return prob_log_next_bit;
}

// generate a specified number of random symbols
// distributed according to the context tree statistics
// Note: It does not revert the history
void ContextTree::genRandomSymbols(symbol_list_t &symbols, size_t bits) {

	genRandomSymbolsAndUpdate(symbols, bits);

	// restore the context tree to it's original state
	for (size_t i = 0; i < bits; i++)
		revert();
}

// Generate a specified number of random symbols distributed according to
// the context tree statistics and update the context tree with the newly
// generated bits
void ContextTree::genRandomSymbolsAndUpdate(symbol_list_t &symbols,
		size_t bits) {
	double prob_next_bit;
	symbol_t sym;

	for (int i = 0; i < bits; i++) {
		// Calculate the probability of the next symbol to be 0, given history
		prob_next_bit = pow(2, getLogProbNextSymbolGivenH(0));

		// Sample the next bit
		sym = (rand01() > prob_next_bit);
		update(sym);
		symbols[i] = sym;
	}
}

// the logarithm of the block probability of the whole sequence
double ContextTree::logBlockProbability(void) {
	return m_root->logProbWeighted();
}

// get the n'th most recent history symbol, NULL if doesn't exist
const symbol_t *ContextTree::nthHistorySymbol(size_t n) const {
	return n < m_history.size() ? &m_history[n] : NULL;
}


int count;

// Debug tree, print history symbols and the context tree in Pre order
void ContextTree::debugTree() {

	std::cout << "History : " << "C0 = " << m_root->m_count[0] << " C1 = "
			<< m_root->m_count[1] << std::endl;
	for (int i = 0; i < m_history.size(); i++) {
		std::cout << m_history.at(i);
	}
	count = 0;
	std::cout << std::endl;
	printTree (m_root);
	std::cout << std::endl;

}

// Print history symbols, and print the structure of the context tree
void ContextTree::debugTreeStructure() {

	// Print the history
	std::cout << "History : " << "C0 = " << m_root->m_count[0] << " C1 = "
			<< m_root->m_count[1] << std::endl;
	for (int i = 0; i < m_history.size(); i++) {
		std::cout << m_history.at(i);
	}
	count = 0;
	std::cout << std::endl;

	// Print the nodes in separate trees
	std::vector<CTNode*> node_list;
	node_list.push_back(m_root);
	std::cout << "Weighted..." << std::endl;
	printTreeStructure(node_list, 0, 0);
	std::cout << std::endl;
	std::cout << "Est..." << std::endl;
	printTreeStructure(node_list, 0, 1);
	std::cout << std::endl;
	std::cout << "Zeros..." << std::endl;
	printTreeStructure(node_list, 0, 2);
	std::cout << std::endl;
	std::cout << "Ones..." << std::endl;
	printTreeStructure(node_list, 0, 3);
	std::cout << std::endl;

}

// Print the Context Tree structure
void ContextTree::printTreeStructure(std::vector<CTNode*> node_list, int cur_depth,
		int type) {
	int i = 0;

	int n_next_level = pow(2, cur_depth + 1);
	int n_cur_level = pow(2, cur_depth);
	CTNode * node;
	std::vector<CTNode*> next_list(n_next_level);
	std::vector<double> data(n_next_level);

	// Return when all the levels have been printed
	if (cur_depth > m_depth) {
		return;
	}

	while (i < n_cur_level) {
		node = node_list[i];
		if (node != NULL) {
			// Create a list for different data in the nodes
			if (type == 0) {
				data[i] = node->m_log_prob_weighted;
			} else if (type == 1) {
				data[i] = node->m_log_prob_est;
			} else if (type == 2) {
				data[i] = node->m_count[0];
			} else {
				data[i] = node->m_count[1];
			}

			next_list[i * 2] = node->m_child[1];
			next_list[i * 2 + 1] = node->m_child[0];
		} else {
			// The node is does not exist
			data[i] = -1.234;
			next_list[i * 2] = NULL;
			next_list[i * 2 + 1] = NULL;
		}
		i++;
	}
	i = 0;
	int n_pad = 80 / (n_cur_level + 1);
	char pad[80 + 1];

	//Adjust the white space characters for this level
	while (i < n_pad - 1) {
		pad[i] = 0x20;
		i++;
	}
	pad[i] = 0;

	// Print the white space characters, along with the data at the current level
	i = 0;
	while (i < n_cur_level) {
		std::cout << pad << data[i];
		i++;
	}
	std::cout << std::endl;

	// Go one level down and print the Context tree at the level
	printTreeStructure(next_list, cur_depth + 1, type);
}

// Print the Pre order traversal of the context tree
void ContextTree::printTree(CTNode *node) {

	// Print the log weighted probability
	std::cout << "Count " << ++count << " Node Weighted probability "
		<< node->m_log_prob_weighted << std::endl;

	if (node->m_child[1] != NULL)
		printTree(node->m_child[1]);

	if (node->m_child[0] != NULL)
		printTree(node->m_child[0]);
}

