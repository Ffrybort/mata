/** @file delta.cc
 * @brief Implementation of the @c mata::nfta::Delta class and related functions.
 */

#include "mata/nfta/delta.hh"

using namespace mata::utils;
using namespace mata::nfta;
using mata::Symbol;

SymbolPost& SymbolPost::operator=(SymbolPost&& rhs) noexcept {
    if (*this != rhs) {
        symbol = rhs.symbol;
        target_tuples = std::move(rhs.target_tuples);
    }
    return *this;
} // operator=

void SymbolPost::insert(const std::vector<State> &tuple) {
    if(target_tuples.empty() || target_tuples.back() < tuple) {
        target_tuples.push_back(tuple);
        return;
    }
    // Find the place where to put the element (if not present).
    // Insert to OrdVector without the searching of a proper position inside insert(const Key&x).
    if (const auto it = std::ranges::lower_bound(target_tuples, tuple);
            it == target_tuples.end() || *it != tuple) {
        target_tuples.insert(it, tuple);
    }
} // insert single vector

void SymbolPost::insert(const StateVectorSet& states) {
    target_tuples.insert(states); // union function already in OrdVector
} // insert multiple

OrdVector<State> StatePost::get_successors() const {
    std::vector<State> successors;  // plain vector to collect everything

    for (const SymbolPost& symbol_post : *this) {
        for (const auto& targets : symbol_post.target_tuples) {
            // append all targets
            successors.insert(successors.end(), targets.begin(), targets.end());
        }
    }
    return OrdVector<State>(successors);
} // get_successors

OrdVector<State> StatePost::get_successors(const Symbol symbol) const {
    auto symbol_post_it = this->end();

    // EPSILON is the maximum symbol
    if (symbol != EPSILON) {
        symbol_post_it = find(symbol);
    } else if (!this->empty()) {
        symbol_post_it = this->end() - 1;
    }

    if (symbol_post_it == this->end()) { return OrdVector<State>(); }

    std::vector<State> successors;  // plain vector to collect everything
    for (const auto& targets : symbol_post_it->target_tuples) {
        successors.insert(successors.end(), targets.begin(), targets.end());
    }
    return OrdVector<State>(successors);
} // get_successors symbol


OrdVector<State> Delta::get_successors(const State s) const {
    return state_post(s).get_successors();
} // get_successors state

OrdVector<State> Delta::get_successors(const State s, const Symbol symbol) const {
    return state_post(s).get_successors(symbol);
} // get_successors s, symbol

std::vector<Transition> Delta::get_transitions() const {
    // this function is expensive

    std::vector<Transition> all_transitions;
    // Iterate over states
    for (State source = 0; source < state_posts_.size(); ++source) {
        // Iterate over symbols
        for (const StatePost& state_post = state_posts_[source]; const SymbolPost& symbol_post : state_post) {
            // Iterate over tuples
            for (const auto& targets : symbol_post.target_tuples) {
                all_transitions.emplace_back(source, symbol_post.symbol, targets); 
            }
        }
    }
    return all_transitions;
} // get_transitions

void Delta::add(const State source, const Symbol symbol, const std::vector<State>& targets) {
    resize_for_states(targets);
    resize_for_states(source);

    if (StatePost& state_post{ state_posts_[source] }; state_post.empty() || state_post.back().symbol < symbol) {
        state_post.insert(SymbolPost{ symbol, targets });
    } else {
        if (const auto symbol_post{ state_post.find(SymbolPost{ symbol }) };
            symbol_post != state_post.end()) {
            // add targets to a symbol post
            symbol_post->insert(targets);
        } else {
            // create a new symbol post
            const SymbolPost new_symbol_post{ symbol, targets };
            state_post.insert(new_symbol_post);
        }
    }
} // add transition


void Delta::add(const State source, const SymbolPost& symbol_post) {
    if(symbol_post.target_tuples.empty()) { return; }
    resize_for_states(source);
    for (const auto& targets : symbol_post.target_tuples) {
        resize_for_states(targets);
    }

    StatePost& state_post = state_posts_[source];
    if (const auto it = state_post.find(symbol_post.symbol); it != state_post.end()) {
        // Symbol already exists - merge targets
        it->insert(symbol_post.target_tuples);
    } else {
        // New symbol - insert whole SymbolPost
        state_post.insert(symbol_post);
    }
} // add symbol post

void Delta::remove(const State source, const Symbol symbol, const std::vector<State>& targets) {
    auto no_transition = [&]() {
        throw std::invalid_argument(
            "Transition [" + std::to_string(source) + ", " + std::to_string(symbol) + ", " +
            std::to_string(targets) + "] does not exist."
        );
    };

    if (source >= state_posts_.size()) { no_transition(); }

    StatePost& state_transitions = state_posts_[source];
    if (state_transitions.empty() || state_transitions.back().symbol < symbol) { no_transition(); }

    const auto symbol_transitions = state_transitions.find(symbol);
    if (symbol_transitions == state_transitions.end()) { no_transition(); }

    symbol_transitions->erase(targets);
    if (symbol_transitions->empty()) { // remove symbol is no targets remain
        state_transitions.erase(*symbol_transitions);
    }
} // remove

void Delta::try_remove(const State source, const Symbol symbol) {
    if (source >= state_posts_.size()) { ; }
    StatePost& state_transitions = state_posts_[source];
    if (state_transitions.empty() || state_transitions.back().symbol < symbol) { ; }
    if (const auto symbol_transitions = state_transitions.find(symbol); symbol_transitions != state_transitions.end()) {
        state_transitions.erase(*symbol_transitions);
    }
} // try_remove


bool Delta::contains(const State source, const Symbol symbol, const std::vector<State>& targets) const {
    if (state_posts_.empty()) { return false; }
    if (state_posts_.size() <= source) { return false; }

    const StatePost& tl = state_posts_[source];
    if (tl.empty()) { return false; }
    const auto symbol_transitions{ tl.find(SymbolPost{ symbol} ) };
    if (symbol_transitions == tl.cend()) {
        return false;
    }

    return symbol_transitions->target_tuples.find(targets) != symbol_transitions->target_tuples.end();
} // contains

size_t Delta::num_of_transitions() const {
    size_t number_of_transitions = 0;
    for (const StatePost& state_post: state_posts_) {
        for (const SymbolPost& symbol_post: state_post) {
            number_of_transitions += symbol_post.target_tuples.size();
        }
    }
    return number_of_transitions;
} // num_of_transitions

bool Delta::empty() const {
    return std::ranges::all_of(state_posts_, [](const StatePost& state_post) { return state_post.empty(); });
} // empty

bool SymbolPost::is_sorted() const {
    if (target_tuples.empty()) { return false; }
    if (!utils::is_sorted(target_tuples.to_vector())) {
        return false;
    }
    return true;
} // is_sorted

bool Delta::is_sorted() {
    for (const StatePost& state_post : state_posts_) {
        if (!utils::is_sorted(state_post.to_vector())) {
            return false;
        }
        for (const SymbolPost& symbol_post : state_post) {
            if (!symbol_post.is_sorted()) { return false; }
        }
    }
    return true;
} // is_sorted


Delta::Transitions::const_iterator::const_iterator(const Delta& delta): delta_{ &delta } {
    const size_t post_size = delta_->num_of_states();
    for (size_t i = 0; i < post_size; ++i) {
        if (!(*delta_)[static_cast<State>(i)].empty()) {
            current_state_ = i;
            state_post_it_ = (*delta_)[static_cast<State>(i)].begin();
            symbol_post_it_ = state_post_it_->target_tuples.begin();
            transition_.source = static_cast<State>(current_state_);
            transition_.symbol = state_post_it_->symbol;
            transition_.targets = *symbol_post_it_;
            return;
        }
    }

    // No transition found, delta contains only empty state posts.
    is_end_ = true;
}

Delta::Transitions::const_iterator::const_iterator(const Delta& delta, const State current_state)
    : delta_{ &delta }, current_state_{ current_state } {
    const size_t post_size = delta_->num_of_states();
    for (State s{ static_cast<State>(current_state_) }; s < static_cast<State>(post_size); ++s) {
        if (const StatePost& state_post{ delta_->state_post(s) }; !state_post.empty()) {
            current_state_ = s;
            state_post_it_ = state_post.begin();
            symbol_post_it_ = state_post_it_->target_tuples.begin();
            transition_.source = static_cast<State>(current_state_);
            transition_.symbol = state_post_it_->symbol;
            transition_.targets = *symbol_post_it_;
            return;
        }
    }

    // No transition found, delta from the current state contains only empty state posts.
    is_end_ = true;
} // const_iterator

Delta::Transitions::const_iterator& Delta::Transitions::const_iterator::operator++() {
    assert(delta_->begin() != delta_->end());

    ++symbol_post_it_;
    if (symbol_post_it_ != state_post_it_->target_tuples.end()) {
        transition_.targets = *symbol_post_it_;
        return *this;
    }

    ++state_post_it_;
    if (state_post_it_ != (*delta_)[static_cast<State>(current_state_)].cend()) {
        symbol_post_it_ = state_post_it_->target_tuples.begin();
        transition_.symbol = state_post_it_->symbol;
        transition_.targets = *symbol_post_it_;
        return *this;
    }

    const size_t state_posts_size{ delta_->num_of_states() };
    do { // Skip empty posts.
        ++current_state_;
    } while (current_state_ < state_posts_size && (*delta_)[static_cast<State>(current_state_)].empty());
    if (current_state_ >= state_posts_size) {
        is_end_ = true;
        return *this;
    }

    const StatePost& state_post{ (*delta_)[static_cast<State>(current_state_)] };
    state_post_it_ = state_post.begin();
    symbol_post_it_ = state_post_it_->target_tuples.begin();

    transition_.source = static_cast<State>(current_state_);
    transition_.symbol = state_post_it_->symbol;
    transition_.targets = *symbol_post_it_;

    return *this;
} // const_iterator::operator++


Delta::Transitions::const_iterator Delta::Transitions::const_iterator::operator++(int) {
    const const_iterator tmp{ *this };
    ++(*this);
    return tmp;
}

bool Delta::Transitions::const_iterator::operator==(const const_iterator& other) const {
    if (is_end_ && other.is_end_) { return true; }
    if (is_end_ != other.is_end_) { return false; }
    return current_state_ == other.current_state_ &&
        state_post_it_ == other.state_post_it_ &&
        symbol_post_it_ == other.symbol_post_it_;
} // const_iterator::operator==

std::vector<StatePost> Delta::renumber_targets(const State offset) const {
    std::vector<StatePost> result;
    result.reserve(state_posts_.size());

    for (const StatePost& state_post : state_posts_) {
        StatePost new_state_post;
        new_state_post.reserve(state_post.size());
        for (const SymbolPost& symbol_post : state_post) {
            StateVectorSet new_target_tuples;
            new_target_tuples.reserve(symbol_post.target_tuples.size());
            for (const auto& targets : symbol_post.target_tuples) {
                std::vector<State> new_targets;
                new_targets.reserve(targets.size());

                for (const State s : targets) {
                    new_targets.push_back(s + offset);
                }
                new_target_tuples.emplace_back(std::move(new_targets));
            }
            new_state_post.emplace_back(
                symbol_post.symbol,
                std::move(new_target_tuples)
            );
        } // for symbol posts
        result.emplace_back(std::move(new_state_post));
    } // for state posts
    return result;
} // renumber_targets

std::vector<StatePost> Delta::renumber_targets(const std::function<State(State)>& renumberer) const {
    std::vector<StatePost> result;
    result.reserve(num_of_states());

    for (const StatePost& state_post : state_posts_) {
        StatePost new_state_post;
        new_state_post.reserve(state_post.size());

        for (const SymbolPost& symbol_post : state_post) {
            StateVectorSet new_target_tuples;
            new_target_tuples.reserve(symbol_post.target_tuples.size());

            for (const auto& targets : symbol_post.target_tuples) {
                std::vector<State> new_targets;
                new_targets.reserve(targets.size());
                std::ranges::transform(targets, std::back_inserter(new_targets), renumberer);
                new_target_tuples.emplace_back(std::move(new_targets));
            }

            new_state_post.emplace_back(
                symbol_post.symbol,
                std::move(new_target_tuples)
            );
        } // for symbol posts
        result.emplace_back(std::move(new_state_post));
    } // for state posts
    return result;
} // renumber_targets

StatePost& Delta::mutable_state_post(const State s) {
    if (s >= state_posts_.size()) {
        utils::reserve_on_insert(state_posts_, s);
        const size_t new_size{ s + 1 };
        state_posts_.resize(new_size);
    }

    return state_posts_[s];
} // mutable_state_post

void  Delta::defragment(const BoolVector& is_staying, const std::vector<State>& renaming) {
    #ifndef NDEBUG
    assert(is_staying.size() == num_of_states());
    State staying_states_count = 0;
    for (State s = 0; s < num_of_states(); ++s) { if (is_staying[s]) { ++staying_states_count; } }
    // Check renaming validity
    for (State s = 0; s < num_of_states(); ++s) {
        if (!is_staying[s]) { continue; }
        assert(s < renaming.size());
        assert(renaming[s] < staying_states_count);
    }
    #endif

    // lambda to determine whether a state post is staying or not
    auto not_staying = [&is_staying](const std::vector<State>& targets) {
        return !std::ranges::all_of(targets.begin(), targets.end(),
            [&](const State s) { return is_staying[s]; });
    };

    size_t source_new = 0 ;
    for (size_t source_orig = 0, num_of_states = this->num_of_states(); source_orig < num_of_states; ++source_orig) {
        if (!is_staying[source_orig]) { continue; }
        StatePost& state_post = state_posts_[source_orig];

        for (auto state_post_it = state_post.begin(); state_post_it != state_post.end();) {
            StateVectorSet& target_tuples = state_post_it->target_tuples;
            target_tuples.erase_if(not_staying);
            // rename manually
            // renaming[old_value] => new_value
            for (auto& targets : target_tuples) {
                for (State& state : targets) {
                    state = renaming[state];
                }
            }
            if (target_tuples.empty()) { state_post_it = state_post.erase(state_post_it); } else { ++state_post_it; }
        } // for symbol posts

        // Move the filtered state post to the new position, if needed.
        if (source_new != source_orig) { state_posts_[source_new] = std::move(state_post); }
        ++source_new;
    } // for all states
    // Resize to remove filtered-out state posts.
    state_posts_.resize(source_new);
} // defragment

void ReversedDelta::print(std::ostream& os) const {
    os << "ReversedDelta {\n";
    for (const auto& sym_trans : symbol_posts) {
        os << "  Symbol: " << sym_trans.symbol << "\n";
        os << "  RevStateTuplePost:\n";
        for (const auto& src_trans : sym_trans.state_tuple_posts) {
            os << "    Sources: [";
            for (size_t i = 0; i < src_trans.sources.size(); ++i) {
                os << src_trans.sources[i];
                if (i + 1 < src_trans.sources.size()) os << ", ";
            }
            os << "] -> Targets: [";
            for (size_t i = 0; i < src_trans.targets.size(); ++i) {
                os << src_trans.targets.at(i);
                if (i + 1 < src_trans.targets.size()) os << ", ";
            }
            os << "]\n";
        }
    }
    os << "}\n";
} // print

OrdVector<State> ReversedDelta::get_initial_states() const {
    std::vector<State> result;
    for (const auto& sym_trans : symbol_posts) {
        if (sym_trans.is_constant()) {
            const auto& targets = sym_trans.state_tuple_posts.front().targets;
            result.insert(result.end(), targets.begin(), targets.end());
        }
    }
    return OrdVector<State>(result);
} // get_initial_states

std::vector<std::pair<Symbol, OrdVector<State>>> ReversedDelta::get_initial_states_by_symbol() const {
    std::vector<std::pair<Symbol, OrdVector<State>>> result;
    for (const auto& sym_trans : symbol_posts) {
        if (sym_trans.is_constant()) {
            const auto& targets = sym_trans.state_tuple_posts.front().targets;
            result.emplace_back(sym_trans.symbol, OrdVector<State>(targets));
        }
    }
    return result;
} // get_initial_states_by_symbol

ReversedDelta Delta::get_reversed(const BoolVector *allowed) const {
    ReversedDelta result;

    for (State q = 0; q < state_posts_.size(); ++q) {
        if (allowed && !(*allowed)[q]) { continue; }
        for (const auto& symbol_post : state_posts_[q]) {
            ReversedDelta::RevSymbolPost sym { symbol_post.symbol };
            auto sym_it = result.symbol_posts.find(sym);
            if (sym_it == result.symbol_posts.end()) {
                sym_it = result.symbol_posts.insert(sym).first;
            }

            for (const auto& tuple : symbol_post.target_tuples) {
                if (allowed && std::ranges::any_of(tuple, [&](const State s) {
                    return !(*allowed)[s]; })) { continue; }

                ReversedDelta::RevStateTuplePost src{tuple};
                auto src_it = sym_it->state_tuple_posts.find(src);
                if (src_it == sym_it->state_tuple_posts.end()) {
                    src_it = sym_it->state_tuple_posts.insert(src).first;
                }
                src_it->targets.insert(q);
            }
        } // for all symbol posts
    } // for all states
    return result;
} // get_reversed

bool Delta::operator==(const Delta& other) const {
    const Transitions this_transitions{ transitions() };
    Transitions::const_iterator this_transitions_it{ this_transitions.begin() };
    const Transitions::const_iterator this_transitions_end{ mata::nfta::Delta::Transitions::end() };
    const Transitions other_transitions{ other.transitions() };
    Transitions::const_iterator other_transitions_it{ other_transitions.begin() };
    const Transitions::const_iterator other_transitions_end{ mata::nfta::Delta::Transitions::end() };
    while (this_transitions_it != this_transitions_end) {
        if (other_transitions_it == other_transitions_end || *this_transitions_it != *other_transitions_it) {
            return false;
        }
        ++this_transitions_it;
        ++other_transitions_it;
    }
    return other_transitions_it == other_transitions_end;
}

StatePost::Moves::const_iterator::const_iterator(
    const StatePost& state_post, const StatePost::const_iterator symbol_post_it,
    const StatePost::const_iterator symbol_post_end)
    : state_post_{ &state_post }, symbol_post_it_{ symbol_post_it }, symbol_post_end_{ symbol_post_end } {
    if (symbol_post_it_ == symbol_post_end_) {
        is_end_ = true;
        return;
    }

    move_.symbol = symbol_post_it_->symbol;
    targets_it_ = symbol_post_it_->target_tuples.cbegin();
    move_.targets = *targets_it_;
}

StatePost::Moves::const_iterator::const_iterator(const StatePost& state_post)
    : state_post_{ &state_post }, symbol_post_it_{ state_post.begin() }, symbol_post_end_{ state_post.end() } {
    if (symbol_post_it_ == symbol_post_end_) {
        is_end_ = true;
        return;
    }

    move_.symbol = symbol_post_it_->symbol;
    targets_it_ = symbol_post_it_->target_tuples.cbegin();
    move_.targets = *targets_it_;
}

StatePost::Moves::const_iterator& StatePost::Moves::const_iterator::operator++() {
    ++targets_it_;
    if (targets_it_ != symbol_post_it_->target_tuples.end()) {
        move_.targets = *targets_it_;
        return *this;
    }

    // Iterate over to the next symbol post, which can be either an end iterator, or symbol post whose
    //  symbol <= symbol_post_end_.
    ++symbol_post_it_;
    if (symbol_post_it_ == symbol_post_end_) {
        is_end_ = true;
        return *this;
    }
    // The current symbol post is valid (not equal symbol_post_end_).
    move_.symbol = symbol_post_it_->symbol;
    targets_it_ = symbol_post_it_->target_tuples.begin();
    move_.targets = *targets_it_;
    return *this;
}

StatePost::Moves::const_iterator StatePost::Moves::const_iterator::operator++(int) {
    const StatePost::Moves::const_iterator tmp{ *this };
    ++(*this);
    return tmp;
}

bool StatePost::Moves::const_iterator::operator==(const StatePost::Moves::const_iterator& other) const {
    if (is_end_ && other.is_end_) {
        return true;
    } if ((is_end_ && !other.is_end_) || (!is_end_ && other.is_end_)) {
        return false;
    }
    return symbol_post_it_ == other.symbol_post_it_ && targets_it_ == other.targets_it_
           && symbol_post_end_ == other.symbol_post_end_;
}

size_t StatePost::num_of_moves() const {
    size_t counter{ 0 };
    for (const SymbolPost& symbol_post: *this) {
        counter += symbol_post.target_tuples.size();
    }
    return counter;
}

StatePost::Moves& StatePost::Moves::operator=(Moves&& other) noexcept {
    if (&other != this) {
        state_post_ = other.state_post_;
        symbol_post_it_ = other.symbol_post_it_;
        symbol_post_end_ = other.symbol_post_end_;
    }
    return *this;
}

StatePost::Moves& StatePost::Moves::operator=(const Moves& other) noexcept {
    if (&other != this) {
        state_post_ = other.state_post_;
        symbol_post_it_ = other.symbol_post_it_;
        symbol_post_end_ = other.symbol_post_end_;
    }
    return *this;
}

StatePost::Moves StatePost::moves(
    const const_iterator symbol_post_it, const const_iterator symbol_post_end) const {
    return { *this, symbol_post_it, symbol_post_end };
}

StatePost::Moves::const_iterator StatePost::Moves::begin() const {
     return { *state_post_, symbol_post_it_, symbol_post_end_ };
}

StatePost::Moves::const_iterator StatePost::Moves::end() { return const_iterator{}; }

Delta::Transitions Delta::transitions() const { return Transitions{ this }; }

Delta::Transitions::const_iterator Delta::Transitions::begin() const { return const_iterator{ *delta_ }; }
Delta::Transitions::const_iterator Delta::Transitions::end() { return const_iterator{}; }

StatePost::Moves::Moves(
    const StatePost& state_post, const StatePost::const_iterator symbol_post_it, const StatePost::const_iterator symbol_post_end)
    : state_post_{ &state_post }, symbol_post_it_{ symbol_post_it }, symbol_post_end_{ symbol_post_end } {}

OrdVector<Symbol> Delta::get_used_symbols(const bool exclude_constants) const {
    std::vector<Symbol> symbols{};
    for (const StatePost& state_post: state_posts_) {
        for (const SymbolPost & symbol_post: state_post) {
            if (exclude_constants && symbol_post.is_constant()) {
                continue;
            }
            reserve_on_insert(symbols);
            symbols.push_back(symbol_post.symbol);
        }
    }
    OrdVector<Symbol> sorted_symbols(symbols);
    return sorted_symbols;
} // get_used_symbols

OrdVector<SymbolArity> Delta::get_used_symbols_arities() const {
    std::vector<SymbolArity> symbols{};
    for (const StatePost& state_post: state_posts_) {
        for (const SymbolPost & symbol_post: state_post) {
            reserve_on_insert(symbols);
            assert(!symbol_post.target_tuples.empty() && "Empty symbol post");
            symbols.emplace_back(symbol_post.symbol, symbol_post.target_tuples.at(0).size());
        }
    }
    OrdVector<SymbolArity> sorted_symbols(symbols);
    return sorted_symbols;
} // get_used_symbols_arities

// end of file delta.cc