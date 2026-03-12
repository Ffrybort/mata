/**
 * @file delta.hh
 * @brief A structure to hold transitions similar to the nfa delta.
 *
 * The structures are named as top-down transitions.
 */

#ifndef NFTA_DELTA_HH
#define NFTA_DELTA_HH

#include <mata/nfta/types.hh>
#include "mata/utils/sparse-set.hh"
#include <mata/alphabet.hh>
#include "mata/utils/synchronized-iterator.hh"

#include <algorithm>
#include <functional>
#include <iterator>
#include <list>
#include <queue>
#include <utility>
#include <ranges>


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
 * @brief Structure represents a post of a single @c symbol: a set of target states in transitions.
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
     * @brief Determine whether the symbol is constant (has an empty target set)
     *
     * todo some better way of representing a constant?
     */
    bool is_constant() const {
        assert(!target_tuples.empty() && "Empty target tuples");
        const bool result = target_tuples.at(0).empty();
        if (result) { assert(target_tuples.size() == 1 && "Target tuples of a constant must have size one"); }
        return result;
    }

    StateVectorSet::iterator begin() { return target_tuples.begin(); }
    StateVectorSet::iterator end() { return target_tuples.end(); }
    StateVectorSet::const_iterator cbegin() const { return target_tuples.cbegin(); }
    StateVectorSet::const_iterator cend() const { return target_tuples.cend(); }

    size_t count(const std::vector<State> s) const { return target_tuples.count(s); }
    bool empty() const { return target_tuples.empty(); }
    // size_t num_of_target_tuples() const { return target_tuples.size(); }


    void insert(std::vector<State> s);
    void insert(const StateVectorSet& states);

    // THIS BREAKS THE SORTEDNESS INVARIANT,
    // dangerous,
    // but useful for adding states in a random order to sort later (supposedly more efficient than inserting in a random order)
    void push_back(const std::vector<State>& s) { target_tuples.push_back(s); }

    template <typename... Args>
    StateVectorSet& emplace_back(Args&&... args) {
    // Forwarding the variadic template pack of arguments to the emplace_back() of the underlying container.
        return target_tuples.emplace_back(std::forward<Args>(args)...);
    }

    void erase(const std::vector<State>& s) { target_tuples.erase(s); }

    StateVectorSet::const_iterator find(const std::vector<State> s) const { return target_tuples.find(s); }
    StateVectorSet::iterator find(const std::vector<State> s) { return target_tuples.find(s); }
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
    // dangerous, breaks the sortedness invariant
    using super::push_back, super::emplace_back;
    // is adding non-const version as well ok?
    using super::front;
    using super::back;
    using super::pop_back;
    using super::filter;
    using super::clear;
    using super::erase;
    using super::find;

    iterator find(const Symbol symbol) {
        static SymbolPost symbol_post{};
        symbol_post.symbol = symbol;
        return super::find(symbol_post);
    }
    const_iterator find(const Symbol symbol) const {
        static SymbolPost symbol_post{};
        symbol_post.symbol = symbol;
        return super::find(symbol_post);
    }

    ///returns an iterator to the smallest epsilon, or end() if there is no epsilon
    const_iterator first_epsilon_it(Symbol first_epsilon) const;

    /**
     * @brief Get the set of all target states in the @c StatePost.
     * @return Set of all target states in the @c StatePost.
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
         * @param[in] symbol_post_end End iterator over symbol posts (which functions as an sentinel; is not iterated over).
         */
        Moves(const StatePost& state_post, StatePost::const_iterator symbol_post_it, StatePost::const_iterator symbol_post_end);
        Moves(Moves&&) = default;
        Moves(Moves&) = default;
        Moves& operator=(Moves&& other) noexcept;
        Moves& operator=(const Moves& other) noexcept;

        class const_iterator;
        const_iterator begin() const;

        static const_iterator end();

    private:
        const StatePost* state_post_{ nullptr };
        StatePost::const_iterator symbol_post_it_{}; ///< Current symbol post iterator to iterate over.
        /// End symbol post iterator which is no longer iterated over (one after the last symbol post iterated over or
        ///  end()).
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
    Moves moves(StatePost::const_iterator symbol_post_it, StatePost::const_iterator symbol_post_end) const;
    /**
     * Iterator over epsilon moves in @c StatePost represented as @c Move instances.
     */
    Moves moves_epsilons(Symbol first_epsilon = EPSILON) const;
    /**
     * Iterator over alphabet (normal) symbols (not over epsilons) in @c StatePost represented as @c Move instances.
     */
    Moves moves_symbols(Symbol last_symbol = EPSILON - 1) const;

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
    const_iterator(const StatePost& state_post);
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
 * @brief Specialization of utils::SynchronizedExistentialIterator for iterating over SymbolPosts.
 */
class SynchronizedExistentialSymbolPostIterator : public utils::SynchronizedExistentialIterator<utils::OrdVector<SymbolPost>::const_iterator> {
public:
    /**
     * @brief Get union of all targets.
     */
    StateVectorSet unify_targets() const;

    /**
     * @brief Synchronize with the given SymbolPost @p sync.
     *
     * Alignes the synchronized iterator to the same symbol as @p sync.
     * @return True iff the synchronized iterator points to the same symbol as @p sync.
     */
    bool synchronize_with(const SymbolPost& sync);

    /**
     * @brief Synchronize with the given symbol @p sync_symbol.
     *
     * Alignes the synchronized iterator to the same symbol as @p sync_symbol.
     * @return True iff the synchronized iterator points to the same symbol as @p sync.
     */
    bool synchronize_with(Symbol sync_symbol);
}; // class SynchronizedExistentialSymbolPostIterator.

/**
 * @brief Delta is a data structure for representing transition relation.
 *
 * Transition is represented as a triple Transition(source state, symbol, target state). Move is the part (symbol, target
 *  state), specified for a single source state.
 * Its underlying data structure is vector of StatePost classes. Each index to the vector corresponds to one source
 *  state, that is, a number for a certain state is an index to the vector of state posts.
 * Transition relation (delta) in Mata stores a set of transitions in a four-level hierarchical structure:
 *  Delta, StatePost, SymbolPost, and a set of target states.
 * A vector of 'StatePost's indexed by a source states on top, where the StatePost for a state 'q' (whose number is
 *  'q' and it is the index to the vector of 'StatePost's) stores a set of 'Move's from the source state 'q'.
 * Namely, 'StatePost' has a vector of 'SymbolPost's, where each 'SymbolPost' stores a symbol 'a' and a vector of
 *  target states of 'a'-moves from state 'q'. 'SymbolPost's are ordered by the symbol, target states are ordered by
 *  the state number.
 */
class Delta {
public:
    inline static const StatePost empty_state_post; // When posts[q] is not allocated, then delta[q] returns this.

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
     * If we try to access a state post of a @p state which is present in the automaton as an initial/final state,
     *  yet does not have allocated space in @c Delta, an @c empty_post is returned. Hence, the function has no side
     *  effects (no allocation is performed; iterators remain valid).
     * @param source[in] Source state of a state post to access.
     * @return State post of @p source.
     */
    const StatePost& state_post(const State s) const {
        if (s >= num_of_states()) { // todo maybe this should throw instead
            return empty_state_post;
        }
        return state_posts_[s];
    }

    /**
     * @brief Get constant reference to the state post of @p source.
     *
     * If we try to access a state post of a @p source which is present in the automaton as an initial/final state,
     *  yet does not have allocated space in @c Delta, an @c empty_post is returned. Hence, the function has no side
     *  effects (no allocation is performed; iterators remain valid).
     * @param s[in] Source state of a state post to access.
     * @return State post of @p source.
     */
    const StatePost& operator[](const State s) const { return state_post(s); }

    /**
     * @brief Renumber targets by adding an offset to each state.
     */
    std::vector<StatePost> renumber_targets(State offset) const;

    /**
     * @brief Renumber targets by a monotonic lambda function.
     */
    std::vector<StatePost> renumber_targets(const std::function<State(State)>& renumberer) const;

    /**
     * @brief Get mutable (non-constant) reference to the state post of @p source. todo
     *
     * The function allows modifying the state post.
     *
     * BEWARE, IT HAS A SIDE EFFECT.
     *
     * If we try to access a state post of a @p source which is present in the automaton as an initial/final state,
     *  yet does not have allocated space in @c Delta, a new state post for @p source will be allocated along with
     *  all state posts for all previous states. This in turn may cause that the entire post data structure is
     *  re-allocated. Iterators to @c Delta will get invalidated.
     * Use the constant 'state_post()' is possible. Or, to prevent the side effect from causing issues, one might want
     *  to make sure that posts of all states in the automaton are allocated, e.g., write an NFA method that allocate
     *  @c Delta for all states of the NFA.
     * @param source[in] Source state of a state post to access.
     * @return State post of @p source.
     */
    StatePost& mutable_state_post(State s);

    template <typename... Args>
    StatePost& emplace_back(Args&&... args) {
    // Forwarding the variadic template pack of arguments to the emplace_back() of the underlying container.
        return state_posts_.emplace_back(std::forward<Args>(args)...);
    }

    void clear() { state_posts_.clear(); }

    /**
     * @brief Allocate state posts up to @p state.
     *
     * @param state new state to add.
     * @return True if delta was allocated, false if the state was present already.
     */
    bool add_state(const State state) {
        if(state < this->num_of_states()) { return false; }
        state_posts_.resize(state + 1);
        return true;
    }

    /**
     * @brief Increase delta by one state.
     * @return the new state.
     */
    State add_state() {
        state_posts_.resize(this->num_of_states() + 1);
        return static_cast<State>(num_of_states() - 1);
    }

    /**
     * @return Number of states in the whole Delta.
     */
    size_t num_of_states() const { return state_posts_.size(); }

    /**
     * Check whether the @p state is in @c Delta.
     */
    bool contains_state(const State state) const { return state < num_of_states(); }

    /**
     * @return Number of transitions in Delta.
     */
    size_t num_of_transitions() const;

    /**
     * @brief Add a single transition.
     */
    void add(State source, Symbol symbol, const std::vector<State>& targets);
    void add(const Transition& trans) { add(trans.source, trans.symbol, trans.targets); }

    /**
     * @brief Add multiple transitions from the same source and symbol.
     */
    void add(State source, const SymbolPost& symbol_post);
    void add(State source, const StatePost& post); // todo test
    void add_multiple(State source, Symbol symbol, const StateVectorSet& target_tuples);

    /**
     * @brief Remove a single transition.
     */
    void remove(State source, Symbol symbol, const std::vector<State>& targets);
    void remove(const Transition& transition) { remove(transition.source, transition.symbol, transition.targets); }

    /**
     * @brief Remove an entire SymbolPost.
     */
    void try_remove(State source, Symbol symbol); // remove all targets

    /**
     * Check whether @c Delta contains a passed transition.
     */
    bool contains(State source, Symbol symbol, const std::vector<State>& targets) const;
    /**
     * Check whether @c Delta contains a transition passed as a triple.
     */
    bool contains(const Transition& transition) const {
        return contains(transition.source, transition.symbol, transition.targets);
	}

    /**
     * Check whether automaton contains no transitions.
     * @return True if there are no transitions in the automaton, false otherwise.
     */
    bool is_empty() const;

    /**
     * @brief Append post vector to the delta.
     *
     * @param post_vector Vector of posts to be appended.
     */
    void append(const std::vector<StatePost>& post_vector) {
        for(const StatePost& pst : post_vector) {
            this->state_posts_.push_back(pst);
        }
    }

    bool is_sorted();

    /**
     * @brief Change targets by an offset.

     * @param offset
     * @return std::vector<StatePost> Copied posts.
     */
    std::vector<StatePost> renumber_targets(State offset);
    using const_iterator = std::vector<StatePost>::const_iterator;
    const_iterator cbegin() const { return state_posts_.cbegin(); }
    const_iterator cend() const { return state_posts_.cend(); }
    const_iterator begin() const { return state_posts_.begin(); }
    const_iterator end() const { return state_posts_.end(); }

    class Transitions;

    /**
     * Iterator over transitions represented as @c Transition instances.
     */
    Transitions transitions() const;

    // get all transitions
    std::vector<Transition> get_transitions() const;

    /**
     * Get transitions leading to @p state_to.
     * @param state_to[in] Target state for transitions to get.
     * @return Transitions leading to @p states_to.
     *
     * Operation is slow, traverses over all symbol posts.
     */
    std::vector<Transition> get_transitions_to(const std::vector<State>& states_to) const;

    /**
     * Get transitions from @p state_from to @p state_to.
     * @param state_from[in] Source state.
     * @param state_from[in] Target state.
     * @return Transitions from @p source to @p state_to.
     *
     * Operation is slow, traverses over all symbol posts.
     */
    std::vector<Transition> get_transitions_between(State state_from, const std::vector<State>& states_to) const;

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
     * @param[in] state State from which successors are checked.
     * @return Set of states that are successors of the given @p state.
     */
    utils::OrdVector<State> get_successors(State s) const; // todo test

    utils::OrdVector<State> get_successors(State state, Symbol symbol) const; // todo test

    /**
     * Iterate over @p epsilon symbol posts under the given @p state.
     * @param[in] s State from which epsilon transitions are checked.
     * @param[in] epsilon User defined epsilon default.
     * @return An iterator to @c SymbolPost with epsilon symbol. End iterator when there are no epsilon transitions.
     */
    StatePost::const_iterator epsilon_symbol_posts(State s, Symbol epsilon = EPSILON) const;

    /**
     * @brief Expand @p target_alphabet by symbols from this delta.
     *
     * The value of the already existing symbols will NOT be overwritten.
     */
    void add_symbols_to(OnTheFlyAlphabet& target_alphabet) const;

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
     * @brief Get the maximum non-epsilon used symbol.
     */
    Symbol get_largest_symbol() const;

    /**
     * @brief Defragment the Delta. todo
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
     * symbols -> sources -> targets
     */
public:
    struct SourceTransitions { /// source tuple and all possible targets
        std::vector<State> sources;
        utils::OrdVector<State> targets;

        SourceTransitions() : sources{}, targets{} {}

        explicit SourceTransitions(std::vector<State> s)
            : sources(std::move(s)), targets{} {}
        std::weak_ordering operator<=>(const SourceTransitions& other) const { return sources <=> other.sources; }
        bool operator==(const SourceTransitions& other) const { return sources == other.sources; }
    };

    struct SymbolTransitions { /// all transitions from a given symbol
        Symbol symbol{};
        utils::OrdVector<SourceTransitions> sources_transitions;

        SymbolTransitions() : symbol{}, sources_transitions{} {}

        explicit SymbolTransitions(Symbol s)
            : symbol(s), sources_transitions{} {}

        std::weak_ordering operator<=>(const SymbolTransitions& other) const { return symbol <=> other.symbol; }
        bool operator==(const SymbolTransitions& other) const { return symbol == other.symbol; }
    };

    struct ReversedDelta {
        utils::OrdVector<SymbolTransitions> symbol_transitions{};

        ReversedDelta() : symbol_transitions{} {}
    };
    static void print_reversed_delta(const Delta::ReversedDelta& delta);
    ReversedDelta get_reversed() const;

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
 * It iterates over triples (State source, Symbol symbol, State target).
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
