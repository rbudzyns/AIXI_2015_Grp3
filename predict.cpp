#include "predict.hpp"
#include "util.hpp"

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
    double ktmul;
    if(sym == 0) {
        ktmul = log2((m_count[0]+0.5)/(m_count[0]+m_count[1]+1));
    } else {
        ktmul = log2((m_count[1]+0.5)/(m_count[0]+m_count[1]+1));         
    }
    return ktmul;
}

// Calculate the logarithm of the weighted block probability.
void CTNode::updateLogProbability(void) {
    double_t prob_weighted = pow(2, m_log_prob_est)+ pow(2, m_child[0]->m_log_prob_weighted+m_child[1]->m_log_prob_weighted);
    m_log_prob_weighted = log2(prob_weighted);
}

// Update the node after having observed a new symbol.
void CTNode::update(const symbol_t symbol){
    // Update 0 or 1 counter for this node  
    m_count[symbol]++;
    // Update the KT estimate for this node
    //Add(As log(P)) the log probabilities, equation 23,24
    m_log_prob_est += logKTMul(symbol);
    // Update the weighted probability of this node    
    updateLogProbability();
}

void CTNode::revert(const symbol_t symbol){
    m_count[symbol]--;
    if(m_count[0] == 0 && m_count[1] == 0) {
        m_log_prob_est = 0.0;
        m_log_prob_weighted = 0.0;
        m_count[0] = 0;
        m_count[1] = 0;
    } else {
        m_log_prob_est -= logKTMul(symbol);
        updateLogProbability();
    }
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
    std::vector<CTNode*> context_path;
    CTNode* current = m_root;

    walkAndGeneratePath(context_path, current);
        
    while(context_path.empty() != true) {
        // Update the nodes along the context path bottom up
        current->update(sym);
        current = context_path.back();
        context_path.pop_back();
    }
}


void ContextTree::update(const symbol_list_t &symbol_list) {
    m_update_partial_count = 0;
    m_update_partial_list.clear();
    
    for(size_t i = 0; i < symbol_list.size(); i++) {
        update(symbol_list[i]);
        m_update_partial_count++;
        m_update_partial_list.push_back(symbol_list[i]);
    }
}


// updates the history statistics, without touching the context tree
void ContextTree::updateHistory(const symbol_list_t &symbol_list) {
    for (size_t i=0; i < symbol_list.size(); i++) {
        m_history.push_back(symbol_list[i]);
    }
}


void ContextTree::walkAndGeneratePath(std::vector<CTNode*> &context_path, CTNode *current) {
        // Update the context tree for the next obeserved bit
    // Path size is min of history size and max depth of the CT
    int path_size = (m_history.size() < m_depth)? m_history.size() : m_depth;
        
    int traverse_depth = 0;
    
    int cur_history_sym;
    // Update the (0,1) count of each context node upto min(depth,history)
    // and remember the path
    while(traverse_depth <= path_size) {
        // Store the current node on the context path, used when updating the CT bottom up
        context_path.push_back(current);
        // Start with the root
        // Go down using the history. 
        // If a context in the history is not present in CT, then add new node
        if(m_update_partial_count > 0 && traverse_depth <= m_update_partial_count) {
            cur_history_sym = m_update_partial_list[m_update_partial_count-traverse_depth];
        } 
        else {
            cur_history_sym = m_history.at(m_history.size()-(traverse_depth-m_update_partial_count));
        }
        // cur_history_sym could be 0 or 1
        // If sym is 0, then move right
        // If sym is 1, then move left
        if(traverse_depth < m_depth) {
            if(current->m_child[cur_history_sym] == NULL) {
                CTNode node;
                current->m_child[cur_history_sym] = &node;
            }
        }
        current =  current->m_child[cur_history_sym];
        traverse_depth++;
    }
}

// Revert the CT to its state prior to 
// the most recently observed symbol
void ContextTree::revert(void) {
    std::vector<CTNode*> context_path;
    CTNode* current = m_root;

    walkAndGeneratePath(context_path, current);

    while(context_path.empty() != true) {
        // Update the nodes along the context path bottom up
        current->revert(m_history.at(m_history.size()-1));
        current = context_path.back();
        context_path.pop_back();
        revertHistory(m_history.size()-1);
    }
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
    double_t last_block_probability, new_block_probability, prob_next_bit;
    symbol_t sym;   
    
    for(int i = 0; i < bits; i++) {
        last_block_probability = logBlockProbability();
        update(0);
        new_block_probability = logBlockProbability();
        prob_next_bit = new_block_probability/last_block_probability;
        revert();
        
        // Sample the next bit
        if(rand01() <= prob_next_bit) {
            sym = 0;
        } else {
            sym = 1;
        } 
        update(sym);
        m_history.push_back(sym);
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

void ContextTree::debugTree() {
    aixi::log << "History : ";
    for(int i=0; i < m_history.size(); i++) {
        aixi::log << m_history.at(i);
    }
    
    printTree(m_root);
}

void ContextTree::printTree(CTNode *node) {
    aixi::log << "Node Weighted probability " << node->m_log_prob_weighted;
    printTree(node->m_child[1]);
    printTree(node->m_child[0]);
}
