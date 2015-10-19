#include "predict.hpp"
#include "util.hpp"

#include <cassert>
#include <cmath>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>

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
			m_log_prob_weighted = m_log_prob_est;
			assert(!isnan(m_log_prob_weighted) && !isinf(m_log_prob_weighted));
		} else {
			assert(!isnan(m_log_prob_est - m_child[1]->m_log_prob_weighted));
			assert(!isinf(m_log_prob_est - m_child[1]->m_log_prob_weighted));
			m_log_prob_weighted = log2(
					pow(2, m_log_prob_est - m_child[1]->m_log_prob_weighted)
							+ 1) + m_child[1]->m_log_prob_weighted - 1;
			assert(!isnan(m_log_prob_weighted) && !isinf(m_log_prob_weighted));
		}
	} else {
		if (m_child[1] == NULL) {
			m_log_prob_weighted = log2(
					pow(2, m_log_prob_est - m_child[0]->m_log_prob_weighted)
							+ 1) + m_child[0]->m_log_prob_weighted - 1;
			assert(!isnan(m_log_prob_weighted) && !isinf(m_log_prob_weighted));
		} else {
			double logKTSub01Weighted = m_log_prob_est
					- (m_child[0]->m_log_prob_weighted
							+ m_child[1]->m_log_prob_weighted);
			assert(!isnan(logKTSub01Weighted) && !isinf(logKTSub01Weighted));
			double TwoPow = pow(2, logKTSub01Weighted) + 1;
			if (isnan(TwoPow) || isinf(TwoPow)) {
				std::cout << "root counts: " << m_count[0] << "," << m_count[1]
						<< std::endl;
				std::cout << "child0 counts: " << m_child[0]->m_count[0] << ","
						<< m_child[0]->m_count[1] << std::endl;
				std::cout << "child1 counts: " << m_child[1]->m_count[0] << ","
						<< m_child[1]->m_count[1] << std::endl;
				std::cout << "logKTSub01Weighted is: child0weighted "
						<< m_child[0]->m_log_prob_weighted
						<< " + child1weighted"
						<< m_child[1]->m_log_prob_weighted << " - kt "
						<< m_log_prob_est << std::endl;
				std::cout << "logKTSub01Weighted is: " << logKTSub01Weighted
						<< std::endl;
				double var1 = pow(10, 300);
				double var2 = pow(10, 300) - pow(10, 30);
				double var3 = var1 - var2;
				std::cout << "floating point error is: " << var3 << std::endl;
			}
			assert(!isnan(TwoPow) && !isinf(TwoPow));
			double ccc = log2(TwoPow) + m_log_prob_est - 1;
			assert(!isnan(TwoPow) && !isinf(TwoPow));

			m_log_prob_weighted = log2(
					pow(2,
							m_log_prob_est
									- (m_child[0]->m_log_prob_weighted
											+ m_child[1]->m_log_prob_weighted))
							+ 1)
					+ (m_child[0]->m_log_prob_weighted
							+ m_child[1]->m_log_prob_weighted) - 1;
			assert(!isnan(m_log_prob_weighted) && !isinf(m_log_prob_weighted));
		}
	}
	if (isnan (m_log_prob_weighted)) {
		std::cout << "Hello nan updateWeightedProbability" << std::endl;
	}
	assert(!isnan(m_log_prob_weighted) && !isinf(m_log_prob_weighted));

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

void CTNode::revert(const symbol_t symbol) {
	m_count[symbol]--;
	if (m_count[0] == 0 && m_count[1] == 0) {
		m_log_prob_est = 0.0;
		m_log_prob_weighted = 0.0;
	} else {
		m_log_prob_est -= logKTMul(symbol);
		updateLogProbability();
	}
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

void ContextTree::update(const symbol_t sym) {
	std::vector<CTNode*> context_path;
	CTNode* current = m_root;

	walkAndGeneratePath(0, context_path, &current);

	while (context_path.empty() != true) {
		// Update the nodes along the context path bottom up
		current->update(sym);
		current = context_path.back();
		context_path.pop_back();
	}
	current->update(sym);
	updateHistory(sym);
}

void ContextTree::update(const symbol_list_t &symbol_list) {
	for (size_t i = 0; i < symbol_list.size(); i++) {
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

void ContextTree::walkAndGeneratePath(int bit_fix,
		std::vector<CTNode*> &context_path, CTNode **current) {

	int traverse_depth = 0;

	int cur_history_sym;
// Update the (0,1) count of each context node upto min(depth,history)
// and remember the path
	while (traverse_depth < m_depth) {
		cur_history_sym = m_history.at(
				bit_fix + (m_history.size() - 1) - traverse_depth);
		// If sym is 0, then move right
		// If sym is 1, then move left
		if ((*current)->m_child[cur_history_sym] == NULL) {
			CTNode* node = new CTNode();
			(*current)->m_child[cur_history_sym] = node;

		}
		// Store the current node on the context path, used when updating the CT bottom up
		context_path.push_back((*current));

		(*current) = (*current)->m_child[cur_history_sym];
		traverse_depth++;
	}
}

// Revert the CT to its state prior to 
// the most recently observed symbol
void ContextTree::revert(void) {
	std::vector<CTNode*> context_path;
	CTNode* current = m_root;

	walkAndGeneratePath(-1, context_path, &current);

	while (context_path.empty() != true) {
		// Update the nodes along the context path bottom up
		current->revert(m_history.at(m_history.size() - 1));
		current = context_path.back();
		context_path.pop_back();
	}
	current->revert(m_history.at(m_history.size() - 1));
}

// shrinks the history down to a former size
void ContextTree::revertHistory(size_t newsize) {

	assert(newsize <= m_history.size());
	while (m_history.size() > newsize)
		m_history.pop_back();
}

// Note: It does not revert the history
// generate a specified number of random symbols
// distributed according to the context tree statistics
void ContextTree::genRandomSymbols(symbol_list_t &symbols, size_t bits) {

	genRandomSymbolsAndUpdate(symbols, bits);

	// restore the context tree to it's original state
	for (size_t i = 0; i < bits; i++)
		revert();
}

// P(x[i]=sym|h)
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

// generate a specified number of random symbols distributed according to
// the context tree statistics and update the context tree with the newly
// generated bits
void ContextTree::genRandomSymbolsAndUpdate(symbol_list_t &symbols,
		size_t bits) {
	double prob_next_bit;
	symbol_t sym;

	for (int i = 0; i < bits; i++) {
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

double * treePre;
double * treeIn;
int num_of_nodes_pre;
int num_of_nodes_in;

int count;

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

void ContextTree::debugTree1() {
	std::cout << "History : " << "C0 = " << m_root->m_count[0] << " C1 = "
			<< m_root->m_count[1] << std::endl;
	for (int i = 0; i < m_history.size(); i++) {
		std::cout << m_history.at(i);
	}
	count = 0;
	std::cout << std::endl;

	std::vector<CTNode*> node_list;
	node_list.push_back(m_root);
	std::cout << "Weighted..." << std::endl;
	printTree1(node_list, 0, 0);
	std::cout << std::endl;
	std::cout << "Est..." << std::endl;
	printTree1(node_list, 0, 1);
	std::cout << std::endl;
	std::cout << "Zeros..." << std::endl;
	printTree1(node_list, 0, 2);
	std::cout << std::endl;
	std::cout << "Ones..." << std::endl;
	printTree1(node_list, 0, 3);
	std::cout << std::endl;

}

void ContextTree::printTree1(std::vector<CTNode*> node_list, int cur_depth,
		int type) {
	int i = 0;

	int n_next_level = pow(2, cur_depth + 1);
	int n_cur_level = pow(2, cur_depth);
	CTNode * node;
	std::vector<CTNode*> next_list(n_next_level);
	std::vector<double> data(n_next_level);

	if (cur_depth > m_depth) {
		return;
	}

	while (i < n_cur_level) {
		node = node_list[i];
		if (node != NULL) {
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
			data[i] = -1.234;
			next_list[i * 2] = NULL;
			next_list[i * 2 + 1] = NULL;
		}
		i++;
	}
	i = 0;
	int n_pad = 80 / (n_cur_level + 1);
	char pad[n_pad + 1];

	while (i < n_pad - 1) {
		pad[i] = 0x20;
		i++;
	}
	pad[i] = 0;

	i = 0;
	while (i < n_cur_level) {
		std::cout << pad << data[i];
		i++;
	}
	std::cout << std::endl;
	printTree1(next_list, cur_depth + 1, type);
}

void ContextTree::printTree(CTNode *node) {
	std::cout << "Count " << ++count << " Node Weighted probability "
			<< node->m_log_prob_weighted << std::endl;

	if (node->m_child[1] != NULL)
		printTree(node->m_child[1]);

	if (node->m_child[0] != NULL)
		printTree(node->m_child[0]);
}

void ContextTree::printInTree(CTNode *node) {
	if (node->m_child[1] != NULL)
		printInTree(node->m_child[1]);

	treeIn[num_of_nodes_in] = node->m_log_prob_weighted;
	num_of_nodes_in++;
	if (node->m_child[0] != NULL)
		printInTree(node->m_child[0]);
}

void ContextTree::printPreTree(CTNode *node) {

	treePre[num_of_nodes_pre] = node->m_log_prob_weighted;
	num_of_nodes_pre++;
	if (node->m_child[1] != NULL)
		printPreTree(node->m_child[1]);
	if (node->m_child[0] != NULL)
		printPreTree(node->m_child[0]);
}
