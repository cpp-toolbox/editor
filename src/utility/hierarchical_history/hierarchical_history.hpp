#ifndef HIERARCHICAL_HISTORY_HPP
#define HIERARCHICAL_HISTORY_HPP

#include <string>
#include <vector>
#include <tuple>
#include <iostream>


// this is a history system that works in layers
// either you are at the head of a layer of history or you have
// moved within a layer of history and not at the head
// if you are not at the head of a layer, then when history is added
// a new layer gets created, 
//
// TODO: clean the below up to explain this better
//
// movements within a layer by browsing history with go back/forward
// are not recorded into history when 
//
// simply because it makes the 
// history array convoluted and confusing for the user, when you're 
// done with a history layer, it will simply jump you back 
//
class HierarchicalHistory {
private:
	// NOTE: flc stands for file line col
    std::vector<std::tuple<std::string, int, int>> file_line_col_history; // Stores the (filepath, line, col) triples
    int current_index;

public:
    HierarchicalHistory();
    void add_flc_to_history(const std::string& filepath, int line, int col);
    void go_back();
    void go_forward();
    std::tuple<std::string, int, int> get_current_history_flc() const;
    void display_history() const;
    void display_pointer() const;
};

#endif // HIERARCHICAL_HISTORY_HPP
