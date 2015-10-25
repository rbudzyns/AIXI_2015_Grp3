#ifndef __PREDICT_HPP__
#define __PREDICT_HPP__

#include <deque>
#include <cmath>

#include "main.hpp"

// stores symbol occurrence counts
typedef unsigned int count_t;

// holds context weights
typedef double weight_t;

// stores the agent's history in terms of primitive symbols
typedef std::deque<symbol_t> history_t;

class CTNode {
	friend class ContextTree; // i.e. ContextTree can access private members of CTNode

public:

	void print(void) const;

	// log weighted blocked probability
	weight_t logProbWeighted(void) const {
		return m_log_prob_weighted;
	}

	// log KT estimated probability
	weight_t logProbEstimated(void) const {
		return m_log_prob_est;
	}

	// the number of times this context has been visited
	count_t visits(void) const {
		return m_count[false] + m_count[true];
	}

	// child corresponding to a particular symbol
	const CTNode *child(symbol_t sym) const {
		return m_child[sym];
	}

	// number of descendants
	size_t size(void) const;

private:
	CTNode(void);

	~CTNode(void);

	// compute the logarithm of the KT-estimator update multiplier   
	double logKTMul(symbol_t sym) const;

	// Calculate the logarithm of the weighted block probability.
	// be careful of numerical issues, use an identity for log(a+b)
	//  so that you can work in logspace instead.
	void updateLogProbability(void);

	// Update the node after having observed a new symbol.
	void update(const symbol_t symbol);

	// Return the node to its state immediately prior to the last update.
	void revert(const symbol_t symbol);

	weight_t m_log_prob_est;      // log KT estimated probability
	weight_t m_log_prob_weighted; // log weighted block probability

	// one slot for each symbol
	count_t m_count[2];  // a,b in CTW literature
	CTNode *m_child[2];

};

class ContextTree {
public:

	// create a context tree of specified maximum depth
	ContextTree(size_t depth);

	~ContextTree(void);

	void print(void);

	// reset the history to the last ct-depth size of history
	void resetHistory(void);

	// clear the entire context tree
	void clear(void);

	// updates the context tree with a new binary symbol
	void update(const symbol_t sym); // TODO: implement in predict.cpp
	// update the context tree with a list of symbols.
	void update(const symbol_list_t &symbol_list); // TODO: implement in predict.cpp
	// add a symbol to the history without updating the context tree.
	void updateHistory(const symbol_list_t &symbol_list);
	void updateHistory(const symbol_t sym);

	//Recalculate the log weighted probability for this node.
	void updateLogProbability(void);

	// removes the most recently observed symbol from the context tree
	void revert(void);

	// shrinks the history down to a former size
	void revertHistory(size_t newsize);

	// the estimated probability of observing a particular symbol or sequence
	double predict(symbol_t sym);
	double predict(symbol_list_t symbol_list);

	// generate a specified number of random symbols
	// distributed according to the context tree statistics
	void genRandomSymbols(symbol_list_t &symbols, size_t bits);

	// generate a specified number of random symbols distributed according to
	// the context tree statistics and update the context tree with the newly
	// generated bits
	void genRandomSymbolsAndUpdate(symbol_list_t &symbols, size_t bits);

	// the logarithm of the block probability of the whole sequence
	double logBlockProbability(void);

	// Calculate the probability of the next symbol given the next history P(x[i]=sym|h)
	double getLogProbNextSymbolGivenH(symbol_t sym);

	// Calculate the probability of the next symbol given the next history
	// P(x[i]=sym|h) and update
	double getLogProbNextSymbolGivenHWithUpdate(symbol_t sym);

	// Create a path list from root node to one level above the leaf node
	// along the context, used for updating and reverting the Context tree
	// from bottom up
	void walkAndGeneratePath(int bit_fix, std::vector<CTNode*> &context_path,
			CTNode **current);

	// Debug tree, print history symbols and the context tree in Pre order
	void debugTree(void);

	// Print history symbols, and print the structure of the context tree
	void debugTreeStructure(void);

	// Print the Pre order traversal of the context tree
	void printTree(CTNode *node);

	// Print the Context Tree structure
	void printTreeStructure(std::vector<CTNode*> node_list, int cur_depth, int type);

	// get the n'th history symbol, NULL if doesn't exist
	const symbol_t *nthHistorySymbol(size_t n) const;

	// the depth of the context tree
	size_t depth(void) const {
		return m_depth;
	}

	// the size of the stored history
	size_t historySize(void) const {
		return m_history.size();
	}

	// number of nodes in the context tree
	size_t size(void) const {
		return m_root ? m_root->size() : 0;
	}

private:
	history_t m_history; // the agents history
	CTNode *m_root;      // the root node of the context tree
	size_t m_depth;      // the maximum depth of the context tree

};

#endif // __PREDICT_HPP__
