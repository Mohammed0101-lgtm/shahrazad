#include "position.h"
#include "ttable.h"
#include "move.h"
#include <vector>

TranspositionTable tt;

struct SearchNode {
    SearchNode *parent;
    Position    pos;
    Position  **children;
    int         val;
};

typedef struct SearchNode SearchNode;

int min(int a, int b) { return a < b ? a : b; }
int max(int a, int b) { return a > b ? a : b; } 

bool isTerminal(SearchNode *node) { return node->children == nullptr; }
bool isRootNode(SearchNode *node) { return node->parent   == nullptr; }

int search(const Position& pos, SearchNode *node, int depth, int alpha, int beta, SearchStack *ss) {
    int color = pos.getSide();
    int position_key = static_cast<int16_t>(pos.position_key);
    int alphaOrig = alpha;
    ttEntry *entry = tt.probe(pos.position_key);

    if (entry) {
        TT_data tt_data = entry->read();

        if (tt_data.value && tt_data.depth >= depth) {
            if (tt_data.bound == EXACT)
                return tt_data.eval;
            else if (tt_data.bound == LOWER)
                alpha = std::max(alpha, static_cast<int>(tt_data.eval));
            else if (tt_data.bound == UPPER)
                beta = std::min(beta, static_cast<int>(tt_data.eval));
            
            if (alpha >= beta)
                return tt_data.eval;
        }
    }

    if (depth == 0 || isTerminal(node)) 
        return node->val; 

    if (!isRootNode(node)) {
        // Perform any non-root specific operations here if needed
    }

    if (alpha >= beta)
        return alpha;
    
    std::vector<Move> moves = generate_all_moves(pos, color);
    int value = INT_MIN;

    for (const Move& move : moves) {
        Position child_pos = pos;
        child_pos.do_move(move);

        SearchNode child_node; 
        child_node.pos = child_pos;
        child_node.val = node->val;
        child_node.children = nullptr;
        child_node.parent = node;

        value = std::max(value, -search(child_pos, &child_node, depth - 1, -beta, -alpha, ss));
        alpha = std::max(alpha, value);
        
        if (alpha >= beta) 
            break;
    }

    TT_data tt_data;
    tt_data.eval = value;
    tt_data.depth = depth;
    tt_data.value = 1; 
    tt_data.bound = (value <= alphaOrig) ? UPPER : (value >= beta) ? LOWER : EXACT;
    entry->save(tt_data);

    return value;
}