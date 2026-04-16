/**
 * @file delta.hh
 * @brief A top-down oriented structure to hold transitions.
 */

#ifndef NFTA_DELTA_HH
#define NFTA_DELTA_HH

#include <mata/nfta/types.hh>
// #include "mata/utils/sparse-set.hh"
#include <mata/alphabet.hh>
// #include "mata/utils/synchronized-iterator.hh"

#include <algorithm>
#include <functional>
#include <iterator>
// #include <list>
#include <queue>
#include <utility>
// #include <ranges>

namespace mata::nfta
{

/**
 * @brief Structure to hold a nfta transition, top-down format (single source and multiple targets).
 */
struct Transition
{
    State source;
    Symbol symbol;
    std::vector<State> targets;

    explicit Transition(const State source = {}, const Symbol symbol = {}, const std::vector<State>& targets = {})
        : source(source), symbol(symbol), targets(targets) {}

    bool operator<(const Transition& other) const {
        if (source != other.source) return source < other.source;
        if (symbol != other.symbol) return symbol < other.symbol;
        if (targets.size() != other.targets.size()) return targets.size() < other.targets.size();
        return targets < other.targets;
    }

    bool operator==(const Transition& other) const {
        return source == other.source
        && symbol == other.symbol
        && targets == other.targets;
    }
};

/**
 * @brief Move from a @c StatePost for a single state, represented as a pair of @c symbol and @c targets.
 */
class Move {
public:
    Symbol symbol;
    std::vector<State> targets;
    explicit Move(const Symbol symbol = {}, std::vector<State> targets = {}) : symbol{symbol}, targets{std::move(targets)} {}

    bool operator==(const Move&) const = default;

    /// sorted by targets first
    bool operator < (const Move& other) const {
      if (targets != other.targets) { return targets < other.targets; }
      return symbol < other.symbol;
    }
}; // class Move.

/**
 * @brief The transition relation reversed to a bottom-up format.
 *
 * It uses a similar four-level structure as @c Delta. This time it is: @c ReversedDelta, @c SymbolTransitions,
 *  @c SourceTransitions, and a set of target states.
 *  @ 'ReversedDelta' consists of a set of 'SymbolTransitions' instances. 'SymbolTransitions' stores all transitions
 *   that use a given symbol, consisting of a symbol and a set of 'SourceTransitions' instances. 'SourceTransitions'
 *   corresponds to transitions from a given tuple of states (using a given symbol), and holds a set of target states.
 * When reversing a @c Delta, target tuples become source tuples, and sources are collected into a set of targets.
 */
class ReversedDelta { // todo this could save memory by source pointers
public:
    struct SourceTransitions {
        std::vector<State> sources;
        utils::OrdVector<State> targets;

        SourceTransitions() : sources{}, targets{} {}

        explicit SourceTransitions(std::vector<State> s) : sources(std::move(s)), targets{} {}

        std::weak_ordering operator<=>(const SourceTransitions& other) const {
            return sources <=> other.sources;
        }

        bool operator==(const SourceTransitions& other) const {
            return sources == other.sources;
        }
       };

       struct SymbolTransitions {
           Symbol symbol{};
           utils::OrdVector<SourceTransitions> sources_transitions;

           SymbolTransitions() : symbol{}, sources_transitions{} {}

           explicit SymbolTransitions(const Symbol s)
               : symbol(s), sources_transitions{} {}

           std::weak_ordering operator<=>(const SymbolTransitions& other) const {
               return symbol <=> other.symbol;
           }

           bool operator==(const SymbolTransitions& other) const {
               return symbol == other.symbol;
           }

           unsigned get_arity() const {
               assert(!sources_transitions.empty() && "Empty source transitions");
               return static_cast<unsigned>(sources_transitions.at(0).sources.size());
           }

           bool is_constant() const {
               return get_arity() == 0;
           }
     };

    utils::OrdVector<SymbolTransitions> symbol_transitions{};
    ReversedDelta() : symbol_transitions{} {}

    /**
     * @brief Print in a readable format to @c std::cout or a given stream.
     */
    void print(std::ostream& os = std::cout) const;

    /**
     * @brief Get states that have a constant (arity 0) transition leading to them.
     */
    utils::OrdVector<State> get_initial_states() const;

    /// initial here means the state has a constant transition
    /**
     * @brief Get states that have a constant (arity 0) transition leading to them grouped by symbol.
     */
    std::vector<std::pair<Symbol, utils::OrdVector<State>>>get_initial_states_by_symbol() const;

    static bool equal(const SymbolTransitions& a, const SymbolTransitions& b) {
        return a.symbol == b.symbol && a.sources_transitions == b.sources_transitions;
    }

    static bool equal(const SourceTransitions& a, const SourceTransitions& b) {
        return a.sources == b.sources && a.targets == b.targets;
    }

    bool is_identical(const ReversedDelta& other) const {
        if (symbol_transitions.size() != other.symbol_transitions.size()) return false;
        for (size_t i = 0; i < symbol_transitions.size(); ++i) {
            const auto& a = symbol_transitions.at(i);
            const auto& b = other.symbol_transitions.at(i);
            if (a.symbol != b.symbol) return false;
            if (a.sources_transitions.size() != b.sources_transitions.size()) return false;
            for (size_t j = 0; j < a.sources_transitions.size(); ++j) {
                const auto& sa = a.sources_transitions.at(j);
                const auto& sb = b.sources_transitions.at(j);
                if (sa.sources != sb.sources) return false;
                if (sa.targets != sb.targets) return false;
            }
        }
      return true;
    }
};

/**
 * @brief Structure represents a post of a single @c symbol: a set of target state tuples.
 */
class SymbolPost {
public:
    Symbol symbol{};
    StateVectorSet target_tuples{};

    SymbolPost() = default;
    explicit SymbolPost(const Symbol symbol) : symbol{ symbol } {}
    SymbolPost(const Symbol symbol, const std::vector<State>& one_tuple) : symbol{ symbol }, target_tuples{ one_tuple } {}
    SymbolPost(const Symbol symbol, StateVectorSet  multiple_tuples) : symbol{ symbol }, target_tuples{std::move( multiple_tuples )} {}

    SymbolPost(SymbolPost&& rhs) noexcept : symbol{ rhs.symbol }, target_tuples{ std::move(rhs.target_tuples) } {}
    SymbolPost(const SymbolPost& rhs) = default;
    SymbolPost& operator=(SymbolPost&& rhs) noexcept;
    SymbolPost& operator=(const SymbolPost& rhs) = default;

    std::weak_ordering operator<=>(const SymbolPost& other) const { return symbol <=> other.symbol; }
    bool operator==(const SymbolPost& other) const { return symbol == other.symbol; }

    /**
     * @brief Determine whether the symbol is constant - it has a single target tuple of size 0.
     */
    bool is_constant() const { // {{{
        const bool result = get_arity() == 0;
        if (result) { assert(target_tuples.size() == 1 && "Target tuples of a constant must have size one"); }
        return result;
    } // }}}

    /**
     * @brief Find a symbol's arity by the first target tuple's size.
     */
    size_t get_arity() const { // {{{
        assert(!target_tuples.empty() && "Empty target tuples");
        return target_tuples.at(0).size();
    } // }}}

    /**
     * @brief Check if the target tuples are empty.
     *
     * This is a temporary state caused by removing transitions and should be corrected by deleting the symbol post.
     */
    bool empty() const { return target_tuples.empty(); }

    /**
     * @brief Insert one tuple of target states;
     */
    void insert(const std::vector<State> &tuple);

    /**
     * @brief Insert multiple tuples of target states;
     */
    void insert(const StateVectorSet& states);

    /**
     * @brief Add a tuple to the back of the target tuples set.
     *
     * This breaks the sortedness invariant if used improperly. It may be used to add targets to sort later, or when
     * working with already sorted targets and a previously empty set.
     */
    void push_back(const std::vector<State>& tuple) { target_tuples.push_back(tuple); }

    /**
     * @brief Remove a tuple.
     */
    void erase(const std::vector<State>& tuple) { target_tuples.erase(tuple); }

    /// iterator to a given tuple
    StateVectorSet::const_iterator find(const std::vector<State> &tuple) const { return target_tuples.find(tuple); }
    /// const iterator to a given tuple
    StateVectorSet::iterator find(const std::vector<State> &tuple) { return target_tuples.find(tuple); }
    /// iterator to the first tuple
    StateVectorSet::iterator begin() { return target_tuples.begin(); }
    /// iterator to the end
    StateVectorSet::iterator end() { return target_tuples.end(); }
    /// const iterator to the first tuple
    StateVectorSet::const_iterator cbegin() const { return target_tuples.cbegin(); }
    /// const iterator to the end
    StateVectorSet::const_iterator cend() const { return target_tuples.cend(); }

    /**
     * @brief Check if the symbol post is valid - sorted and target tuples are not empty.
     */
    bool is_sorted() const;
}; // class mata::nfta::SymbolPost.

/**
 * @brief A data structure representing possible transitions from a single source state.
 */
class StatePost : utils::OrdVector<SymbolPost> {
    using super = OrdVector<SymbolPost>;
public:
    using super::iterator, super::const_iterator;
    using super::begin, super::end, super::cbegin, super::cend;
    using super::OrdVector;
    using super::operator=;
    using super::operator==;
    StatePost(const StatePost&) = default;
    StatePost(StatePost&&) = default;
    StatePost& operator=(const StatePost&) = default;
    StatePost& operator=(StatePost&&) = default;
    bool operator==(const StatePost&) const = default;
    using super::insert;
    using super::reserve;
    using super::empty, super::size;
    using super::to_vector;
    using super::push_back, super::emplace_back;
    using super::front;
    using super::back;
    using super::pop_back;
    using super::filter;
    using super::clear;
    using super::erase;
    using super::find;


    /// @brief iterator to a given symbol post
    iterator find(const Symbol symbol) {
      static SymbolPost symbol_post{};
        symbol_post.symbol = symbol;
        return super::find(symbol_post);
    }

    /// @brief const iterator to a given symbol post
    const_iterator find(const Symbol symbol) const {
        static SymbolPost symbol_post{};
        symbol_post.symbol = symbol;
        return super::find(symbol_post);
    }

    /**
     * @brief Get the set of all target states (across all tuples) in the @c StatePost.
     */
    OrdVector<State> get_successors() const;

    /**
     * @brief Returns a reference to target states for a given symbol in the @c StatePost.
     *
     * If there is no such symbol, a static empty set is returned.
     */
    OrdVector<State> get_successors(Symbol symbol) const;

    /**
     * @brief Iterator over moves represented as @c Move instances.
     *
     * It iterates over pairs (symbol, target) for the given @c StatePost.
     */
    class Moves {
    public:
        Moves() = default;

        /**
         * @brief construct moves iterating over a range @p symbol_post_it (including) to @p symbol_post_end (excluding).
         *
         * @param[in] state_post State post to iterate over.
         * @param[in] symbol_post_it First iterator over symbol posts to iterate over.
         * @param[in] symbol_post_end End iterator over symbol posts (which functions as a sentinel; is not iterated over).
         */
        Moves(const StatePost& state_post, const_iterator symbol_post_it, const_iterator symbol_post_end);
        Moves(Moves&&) = default;
        Moves(Moves&) = default;
        Moves& operator=(Moves&& other) noexcept;
        Moves& operator=(const Moves& other) noexcept;
        class const_iterator;
        const_iterator begin() const;
        static const_iterator end();

    private:
        const StatePost* state_post_{ nullptr };

        /// Current symbol post iterator to iterate over.
        StatePost::const_iterator symbol_post_it_{};
        /// End symbol post iterator which is no longer iterated over (one after the last symbol post)
        StatePost::const_iterator symbol_post_end_{};
    }; // class Moves.

    /**
     * Iterator over all moves (over all labels) in @c StatePost represented as @c Move instances.
     */
    Moves moves() const { return { *this, this->cbegin(), this->cend() }; }

    /**
     * Iterator over specified moves in @c StatePost represented as @c Move instances.
     *
     * @param[in] symbol_post_it First iterator over symbol posts to iterate over.
     * @param[in] symbol_post_end End iterator over symbol posts (which functions as an sentinel, is not iterated over).
     */
    Moves moves(const_iterator symbol_post_it, const_iterator symbol_post_end) const;

    /**
     * Count the number of all moves in @c StatePost.
     */
    size_t num_of_moves() const;
}; // class StatePost.

/**
 * Iterator over moves.
 */
class StatePost::Moves::const_iterator {
private:
    const StatePost* state_post_{ nullptr };
    StatePost::const_iterator symbol_post_it_{};
    StateVectorSet::const_iterator targets_it_{};
    StatePost::const_iterator symbol_post_end_{};
    bool is_end_{ false };

    /// Internal allocated instance of @c Move which is set for the move currently iterated over and returned as
    ///  a reference with @c operator*().
    Move move_{};

public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = Move;
    using difference_type = size_t;
    using pointer = Move*;
    using reference = Move&;

    /// Construct end iterator.
    const_iterator(): is_end_{ true } {}
    /// Const all moves iterator.
    explicit const_iterator(const StatePost& state_post);
    /// Construct iterator from @p symbol_post_it (including) to @p symbol_post_it_end (excluding).
    const_iterator(const StatePost& state_post, StatePost::const_iterator symbol_post_it,
                   StatePost::const_iterator symbol_post_end);
    const_iterator(const const_iterator& other) noexcept = default;
    const_iterator(const_iterator&&) = default;

    const Move& operator*() const { return move_; }
    const Move* operator->() const { return &move_; }

    // Prefix increment
    const_iterator& operator++();
    // Postfix increment
    const_iterator operator++(int);

    const_iterator& operator=(const const_iterator& other) noexcept = default;
    const_iterator& operator=(const_iterator&&) = default;

    bool operator==(const const_iterator& other) const;
}; // class const_iterator.


/**
 * @brief @c  Delta is a data structure for representing transition relation.
 *
 * Transition is represented as a triple Transition(source state, symbol, target states vector). Move is the part (symbol, target
 *  states), specified for a single source state.
 * Its underlying data structure is vector of StatePost classes. Each index to the vector corresponds to one source
 *  state, that is, a number for a certain state is an index to the vector of state posts.
 * Transition relation (delta) in Mata stores a set of transitions in a four-level hierarchical structure:
 *  @c Delta, @c StatePost, @c SymbolPost, and a set of target tuples.
 * A vector of 'StatePost's indexed by a source states on top, where the StatePost for a state 'q' (whose number is
 *  'q' and it is the index to the vector of 'StatePost's) stores a set of 'Move's from the source state 'q'.
 * Namely, 'StatePost' has a vector of 'SymbolPost's, where each 'SymbolPost' stores a symbol 'a' and a vector of
 *  target states of 'a'-moves from state 'q'. 'SymbolPost's are ordered by the symbol, target tuples are ordered in
 *  lexicographical order.
 */
class Delta {
public:
    Delta(): state_posts_{} {}
    Delta(const Delta& other) = default;
    Delta(Delta&& other) = default;
    explicit Delta(const size_t n): state_posts_{ n } {}

    Delta& operator=(const Delta& other) = default;
    Delta& operator=(Delta&& other) = default;

    bool operator==(const Delta& other) const;

    void reserve(const size_t n) { state_posts_.reserve(n); }
    void resize(const size_t n) { state_posts_.resize(n); }

    /**
     * @brief Get constant reference to the state post of @p source.
     *
     * @param s [in] Source state of a state post to access.
     * @throws std::runtime_error if state post is out of range
     * @return State post of @p source.
     */
    const StatePost& state_post(const State s) const {
        if (s >= num_of_states()) {
          throw std::runtime_error("state post is out of range");
        }
        return state_posts_[s];
    }

    /**
     * @brief Get constant reference to the state post of @p source.
     *
     * @return State post of @p source.
     * @throws std::runtime_error if state post is out of range
     */
    const StatePost& operator[](const State s) const { return state_post(s); }

    /**
     * @brief Renumber targets by adding the @p offset to each state.
     */
    std::vector<StatePost> renumber_targets(State offset) const;

    /**
     * @brief Renumber targets by a monotonic lambda function.
     */
    std::vector<StatePost> renumber_targets(const std::function<State(State)>& renumberer) const;

    /**
     * @brief Get mutable (non-constant) reference to the state post of @p source.
     *
     * The function allows modifying the state post.
     *
     * BEWARE, IT HAS A SIDE EFFECT.
     *
     * If we try to access a state post of a @p s which does not have allocated space in @c Delta, a new state post
     * for be allocated along with  all state posts for all previous states. Iterators to @c Delta will get invalidated.
     */
    StatePost& mutable_state_post(State s);

    /**
     * Delete all state posts.
     */
    void clear() { state_posts_.clear(); }

    /**
     * @brief Allocate state posts up to @p state.
     *
     * @param state new state to add.
     * @return True if any state was allocated, false if the state was present already.
     */
    bool add_state(const State state) { // {{{
        if(state < this->num_of_states()) { return false; }
        state_posts_.resize(state + 1);
        return true;
    } // }}}

    /**
     * @brief Add one state (the next available value) to @c Delta.
     *
     * @return the new state
     */
    State add_state() { // {{{
        state_posts_.resize(this->num_of_states() + 1);
        return static_cast<State>(num_of_states() - 1);
    } // }}}

    /**
     * @brief Get the number of states in the whole @c Delta.
     */
    size_t num_of_states() const { return state_posts_.size(); }

    /**
     * @brief Check whether the @p state is in @c Delta.
     */
    bool contains_state(const State state) const { return state < num_of_states(); }

    /**
     * @brief Get the number of transitions in Delta.
     */
    size_t num_of_transitions() const;

    /**
     * @brief Add a single transition.
     */
    void add(State source, Symbol symbol, const std::vector<State>& targets);

    /**
     * @brief Add a transition represented as a @c Transition instance.
     */
    void add(const Transition& trans) { add(trans.source, trans.symbol, trans.targets); }

    /**
     * @brief Add an entire @c SymbolPost.
     *
     * If a there already is a 'SymbolPost' with the same symbol, targets are merged.
     */
    void add(State source, const SymbolPost& symbol_post);

    /**
     * @brief Remove a single transition.
     *
     * @throw std::runtime_error if the transition does not exist.
     */
    void remove(State source, Symbol symbol, const std::vector<State>& targets);

    /**
     * @brief Remove a transition given by a @c Transition instance.
     *
     * @throw std::runtime_error if the transition does not exist.
     */
    void remove(const Transition& transition) { remove(transition.source, transition.symbol, transition.targets); }

    /**
     * @brief Remove an entire SymbolPost.
     *
     * If the 'SymbolPost' does not exist, nothing happens.
     */
    void try_remove(State source, Symbol symbol);

    /**
     * Check whether @c Delta contains a passed transition.
     */
    bool contains(State source, Symbol symbol, const std::vector<State>& targets) const;

    /**
     * Check whether @c Delta contains a transition given by a @Transition instance.
     */
    bool contains(const Transition& transition) const {
        return contains(transition.source, transition.symbol, transition.targets);
    }

    /**
     * Check whether automaton @Delta is empty - it may have allocated states but contains no @c SymbolPost.
     */
    bool empty() const;

    /**
     * @brief Append a vector of @SymbolPost instances to the delta.
     *
     * States are NOT renamed in this function, they are expected to already have been renamed.
     */
    void append(const std::vector<StatePost>& post_vector) { // {{{
        for(const StatePost& pst : post_vector) {
            this->state_posts_.push_back(pst);
        }
    } // }}}

    bool is_sorted();

    /**
     * @brief Rename all target states by adding an offset.
     *
     * @param offset to add to each state.
     * @return std::vector<StatePost> Copied posts.
     *
     * This function may be used to rename states before appending the renamed 'StatePost's to another @c Delta.
     */
    std::vector<StatePost> renumber_targets(State offset);

    using const_iterator = std::vector<StatePost>::const_iterator;

    /// Iterator to the first @c StatePost
    const_iterator cbegin() const { return state_posts_.cbegin(); }
    /// Iterator beyond the last @c StatePost
    const_iterator cend() const { return state_posts_.cend(); }
    /// Const iterator to the first @c StatePost
    const_iterator begin() const { return state_posts_.begin(); }
    /// Iterator beyond the last @c StatePost
    const_iterator end() const { return state_posts_.end(); }

    class Transitions;
    /**
     * Iterator over transitions represented as @c Transition instances.
     */
    Transitions transitions() const;

    // get all transitions
    std::vector<Transition> get_transitions() const;

    /**
     * @brief Resize the delta to fit the given @p states.
     * @tparam States A variadic parameter pack of states to resize the delta for.
     * @param states States to resize the delta for.
     */
    template<typename... States> requires utils::AllOfType<State, States...>
    Delta& resize_for_states(States... states) {
        if constexpr (sizeof...(states) > 0) {
            if (const State max_state{ std::max({ static_cast<State>(states)... }) }; max_state >= num_of_states()) {
                reserve_on_insert(state_posts_, max_state);
                state_posts_.resize(max_state + 1);
            }
        }
        return *this;
    }
    Delta& resize_for_states(const std::vector<State>& states) {
      if (!states.empty()) {
          const State max_state = *std::ranges::max_element(states);
          resize_for_states(max_state);
      }
      return *this;
    }

    /**
     * Get the set of state vectors that are successors of the given @p state.
     * @param s
     * @param[in] state State from which successors are checked.
     * @return Set of states that are successors of the given @p state.
     */
    utils::OrdVector<State> get_successors(State s) const; // todo test

    /**
     * Get the set of state vectors that are successors of the given @p state.
     * @param[in] s State from which successors are checked.
     * @param symbol
     * @return Set of states that are successors of the given @p state.
     */
    utils::OrdVector<State> get_successors(State s, Symbol symbol) const; // todo test

    /**
     * @brief Get the set of symbols used on the transitions in the automaton.
     *
     * Does not necessarily have to equal the set of symbols in the alphabet used by the automaton.
     * @return Set of symbols used on the transitions.
     */
    utils::OrdVector<Symbol> get_used_symbols(bool exclude_constants = false) const;

    /**
     * @brief Get the set of symbols used on the transitions in the automaton.
     *
     * Does not necessarily have to equal the set of symbols in the alphabet used by the automaton.
     * @return Set of symbols used on the transitions.
     */
     utils::OrdVector<SymbolArity> get_used_symbols_arities() const; // todo test

    /**
     * @brief Defragment the Delta.
     *
     * This function removes all state posts which are not in @p is_staying and renames the remaining state posts
     * according to @p renaming.
     *
     * @param[in] is_staying Boolean vector indicating which states are staying in the Delta.
     * @param[in] renaming Vector of states to rename the remaining state posts to.
     * @return Self with defragmented delta.
     */
    void defragment(const BoolVector& is_staying, const std::vector<State>& renaming);

    /**
     * Reversed delta for bottom-up operations.
     *
     * @param allowed [in, optional] Filter out transitions containing any state that is not allowed.
     */
    ReversedDelta get_reversed(const BoolVector *allowed = nullptr) const;

    /**
     * @brief Compute epsilon closures for each state.
     *
     * @param epsilon symbol to consider epsilon
     * @param include_state [in, optional] true by default, if true, the state itself is included
     */
    std::vector<StateSet> get_epsilon_closures(Symbol epsilon, bool include_state) const;

protected:
    std::vector<StatePost> state_posts_;
}; // class Delta.

/**
 * @brief Defragment the Delta.
 *
 * This function removes all state posts which are not in @p is_staying and renames the remaining state posts
 * according to @p renaming.
 *
 * @param[in] delta Delta to defragment.
 * @param[in] is_staying Boolean vector indicating which states are staying in the Delta.
 * @param[in] renaming Vector of states to rename the remaining state posts to.
 * @return The defragmented Delta.
 */
Delta defragment(const Delta& delta, const BoolVector& is_staying, const std::vector<std::vector<State>>& renaming);

/**
 * @brief Iterator over transitions represented as @c Transition instances.
 *
 * It iterates over triples (source, symbol, target tuple).
 */
class Delta::Transitions {
public:
    Transitions() = default;
    explicit Transitions(const Delta* delta): delta_{ delta } {}
    Transitions(Transitions&&) = default;
    Transitions(const Transitions&) = default;
    Transitions& operator=(Transitions&&) = default;
    Transitions& operator=(const Transitions&) = default;

    class const_iterator;
    const_iterator begin() const;

    static const_iterator end();
private:
    const Delta* delta_;
}; // class Transitions.

/**
 * Iterator over transitions.
 */
class Delta::Transitions::const_iterator {
private:
    const Delta* delta_ = nullptr;
    size_t current_state_{};
    StatePost::const_iterator state_post_it_{};
    StateVectorSet::const_iterator symbol_post_it_{};
    bool is_end_{ false };
    Transition transition_{};

public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = Transition;
    using difference_type = size_t;
    using pointer = Transition*;
    using reference = Transition&;

    const_iterator(): is_end_{ true } {}
    explicit const_iterator(const Delta& delta);
    const_iterator(const Delta& delta, State current_state);

    const_iterator(const const_iterator& other) noexcept = default;
    const_iterator(const_iterator&&) = default;

    const Transition& operator*() const { return transition_; }
    const Transition* operator->() const { return &transition_; }

    // Prefix increment
    const_iterator& operator++();
    // Postfix increment
    const_iterator operator++(int);

    const_iterator& operator=(const const_iterator& other) noexcept = default;
    const_iterator& operator=(const_iterator&&) = default;

    bool operator==(const const_iterator& other) const;
}; // class Delta::Transitions::const_iterator.


} // namespace nfta
#endif //NFTA_DELTA_HH
// end of file delta.hh