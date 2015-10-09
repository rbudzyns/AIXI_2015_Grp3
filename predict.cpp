#include "predict.hpp"

#include <cassert>
#include <cmath>

CTNode::CTNode(void) :
    m_log_prob_est(0.0),
    m_log_prob_weighted(0.0)
{
    m_count[0] = 0;
    m_count[1] = 0;
    m_child[0] = NULL;
    m_child[1] = NULL;
}


CTNode::~CTNode(void) {
    if (m_child[0]) delete m_child[0];
    if (m_child[1]) delete m_child[1];
}


// number of descendants of a node in the context tree
size_t CTNode::size(void) const {

    size_t rval = 1;
    rval += child(false) ? child(false)->size() : 0;
    rval += child(true)  ? child(true)->size()  : 0;
    return rval;
}


// compute the logarithm of the KT-estimator update multiplier
double CTNode::logKTMul(symbol_t sym) const {
    // Eq 27, and then take log
    return NULL; // TODO: implement
    
}

// Calculate the logarithm of the weighted block probability.
void CTNode::updateLogProbability(void) {
    // TODO : implement
}

// Update the node after having observed a new symbol.
void CTNode::update(const symbol_t symbol){
    // TODO : implement
}

void CTNode::revert(const symbol_t symbol){
    // TODO : implement
}

// create a context tree of specified maximum depth
ContextTree::ContextTree(size_t depth) :
    m_root(new CTNode()),
    m_depth(depth)
{ return; }


ContextTree::~ContextTree(void) {
    if (m_root) delete m_root;
}


// clear the entire context tree
void ContextTree::clear(void) {
    m_history.clear();
    if (m_root) delete m_root;
    m_root = new CTNode();
}


void ContextTree::update(const symbol_t sym) {
    // Update the context tree for the next obeserved bit
    int path_size = (m_history.size() < m_depth)? m_history.size() : m_depth;
    std::vector<CTNode*> context_path;
    
    CTNode* current = m_root;
    int traverse_nodes_n = 0;
    // Update the (0,1) count of each context node upto min(depth,history)
    // and remember the path
    while(traverse_nodes_n <= path_size) {
        context_path.push_back(current);
        if(sym == 0) {
            current.m_log_prob_est *= log2((current.m_count[0]+0.5)/(current.m_count[0]+current.m_count[1]+1));
            current->m_count[0]++;
            current =  current->m_child[0];
        } else {
            current.m_log_prob_est *= log2((current.m_count[1]+0.5)/(current.m_count[0]+current.m_count[1]+1));         
            current->m_count[1]++;
            current =  current->m_child[1];
        }
    }
    
    // Add new node for sym to the context tree
    CTNode node;
    if(sym == 0) {
        current.m_child[0] = node;
    } else {
        current.m_child[1] = node;
    }
    
    while(current != NULL) {
    }
}


void ContextTree::update(const symbol_list_t &symbol_list) {
    for(size_t i = 0; i < symbol_list.size(); i++) {
        update(symbol_list.front());
    }
}


// updates the history statistics, without touching the context tree
void ContextTree::updateHistory(const symbol_list_t &symbol_list) {

    for (size_t i=0; i < symbol_list.size(); i++) {
        m_history.push_back(symbol_list[i]);
    }
}


// removes the most recently observed symbol from the context tree
void ContextTree::revert(void) {
    // TODO: implement
}


// shrinks the history down to a former size
void ContextTree::revertHistory(size_t newsize) {

    assert(newsize <= m_history.size());
    while (m_history.size() > newsize) m_history.pop_back();
}



// generate a specified number of random symbols
// distributed according to the context tree statistics
void ContextTree::genRandomSymbols(symbol_list_t &symbols, size_t bits) {

    genRandomSymbolsAndUpdate(symbols, bits);

    // restore the context tree to it's original state
    for (size_t i=0; i < bits; i++) revert();
}


// generate a specified number of random symbols distributed according to
// the context tree statistics and update the context tree with the newly
// generated bits
void ContextTree::genRandomSymbolsAndUpdate(symbol_list_t &symbols, size_t bits) {
    // TODO: implement
}


// the logarithm of the block probability of the whole sequence
double ContextTree::logBlockProbability(void) {
    return m_root->logProbWeighted();
}


// get the n'th most recent history symbol, NULL if doesn't exist
const symbol_t *ContextTree::nthHistorySymbol(size_t n) const {
    return n < m_history.size() ? &m_history[n] : NULL;
}
