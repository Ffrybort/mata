/**
 * @file delta.hh
 * @brief A set of all transition rules.
 *
 * todo rewrite delta with SymbolPost and StatePost
 */

#ifndef NFTA_DELTA_HH
#define NFTA_DELTA_HH

#include "types.hh"
#include "mata/alphabet.hh"
//#include "mata/nfa/delta.hh"

namespace mata::nfta
{
    // simple delta prototype
    class Delta
    {
        std::set<Transition> transitions;
        typename std::set<Transition>::const_iterator iter;
    public:
        Delta() : transitions(), iter(transitions.begin())  {}

        explicit Delta(std::set<Transition> transitions)
            : transitions(std::move(transitions)), iter(transitions.begin())
        {
            reset_iterator();
        }

        /**
        * @brief Add a transition to the Delta. Ignores duplicates.
        */
        void add(const Transition& transition)
        {
            transitions.insert(transition);
        }

        /**
         * @brief Checks if Delta is empty.
         */
        [[nodiscard]] bool is_empty() const
        {
            return transitions.empty();
        }

        /**
         * @brief Returns the number of transitions in the Delta.
         */
        [[nodiscard]] std::size_t size() const
        {
            return transitions.size();
        }

        /**
         * @brief Checks whether a specific transition exists in the Delta.
         */
        [[nodiscard]] bool contains(const Transition& transition) const
        {
            return transitions.contains(transition);
        }

        /**
         * @brief Returns a const reference to the set of transitions.
         * @return Const reference to internal transitions set.
         */
        [[nodiscard]] const std::set<Transition>& get_transitions() const
        {
            return transitions;
        }

        /**
         * @brief Returns an iterator to the beginning of the transitions.
         * @return Iterator to the first transition.
         */
        [[nodiscard]] auto begin() const { return transitions.begin(); }

        /**
         * @brief Returns an iterator to the end of the transitions.
         * @return Iterator past the last transition.
         */
        [[nodiscard]] auto end() const { return transitions.end(); }

        /**
         * @brief Returns a pointer to the next transition after the given one, or nullptr.
         *
         * @return Pointer to the next transition, or nullptr.
         */
        const Transition* next_transition()
        {
            if (iter == transitions.end()) return nullptr;
            return &*(iter++);
        }

        void reset_iterator() { iter = transitions.begin(); }
    }; // class delta


//    struct Transition_bottom_up
//    {
//        Symbol symbol;
//        std::vector<State> sources;
//        State target;
//
//        explicit Transition_bottom_up(
//            const Symbol symbol = {},
//            const std::vector<State>& sources = {},
//            const State target = {}
//        )
//            : symbol(symbol),
//              sources(sources),
//              target(target)
//        {
//        }
//
//        bool operator<(const Transition_bottom_up& other) const
//        {
//            if (sources.size() != other.sources.size()) return sources.size() < other.sources.size();
//            if (sources != other.sources) return sources < other.sources;
//            if (symbol != other.symbol) return symbol < other.symbol;
//            return target < other.target;
//        }
//    };

//
//
///**
// *
// */
//class SymbolPost {
//public:
//    Symbol symbol{};
//    StateVectorSet targets{};  // set of vectors
//
//    SymbolPost() = default;
//    explicit SymbolPost(const Symbol symbol) : symbol{ symbol } {}
//    SymbolPost(const Symbol symbol, std::vector<State> states_to) : symbol{ symbol }, targets{ std::move(states_to) } {}
//
//    SymbolPost(SymbolPost&& rhs) noexcept : symbol{ rhs.symbol }, targets{ std::move(rhs.targets) } {}
//    SymbolPost(const SymbolPost& rhs) = default;
//    SymbolPost& operator=(SymbolPost&& rhs) noexcept;
//    SymbolPost& operator=(const SymbolPost& rhs) = default;
//
//    std::weak_ordering operator<=>(const SymbolPost& other) const { return symbol <=> other.symbol; }
//    bool operator==(const SymbolPost& other) const { return symbol == other.symbol; }
//
//    StateVectorSet::iterator begin() { return targets.begin(); }
//    StateVectorSet::iterator end() { return targets.end(); }
//
//    StateVectorSet::const_iterator cbegin() const { return targets.cbegin(); }
//    StateVectorSet::const_iterator cend() const { return targets.cend(); }
//
//    size_t count(State s) const;
//    bool contains(State s) const;
//    bool contains(const std::vector<State>& state_vector) const;
//
//    bool is_empty() const { return targets.empty(); }
//    size_t num_of_target_vectors() const { return targets.size(); }
//
//    void insert(const std::vector<State>& states) { targets.insert(states); }
//
//    // THIS BREAKS THE SORTEDNESS INVARIANT,
//    // useful for adding states in a random order to sort later
//    void push_back(const std::vector<State>& states) { targets.push_back(states); }
//
//    template <typename... Args>
//    StateVectorSet& emplace_back(Args&&... args) {
//    // Forwarding the variadic template pack of arguments to the emplace_back() of the underlying container.
//        return targets.emplace_back(std::forward<Args>(args)...);
//    }
//
//    void erase(const std::vector<State>& states) { targets.erase(states); }
//
//    StateVectorSet::const_iterator find(const std::vector<State>& states) const { return targets.find(states); }
//    StateVectorSet::iterator find(const std::vector<State>& states) { return targets.find(states); }
//}; // class mata::nfta::SymbolPost.
//
///**
// *
// */
//class StatePost : utils::OrdVector<SymbolPost> {
//    using super = OrdVector<SymbolPost>;
//public:
//    // the move class only works with StatePost and SymbolPost and can be reused
//    using Moves = mata::nfa::StatePost::Moves;
//    using super::iterator, super::const_iterator;
//    using super::begin, super::end, super::cbegin, super::cend;
//    using super::OrdVector;
//    using super::operator=;
//    using super::operator==;
//
//    StatePost(const StatePost&) = default;
//    StatePost(StatePost&&) = default;
//    StatePost& operator=(const StatePost&) = default;
//    StatePost& operator=(StatePost&&) = default;
//    bool operator==(const StatePost&) const = default;
//
//    using super::insert;
////    using super::reserve;
//    using super::empty; /*, super::size;*/
//    using super::to_vector;
//    // dangerous, breaks the sortedness invariant
//    using super::push_back, super::emplace_back;
//    using super::front;
//    using super::back;
//    using super::pop_back;
//    using super::filter;
//    using super::clear;
//
//    using super::erase;
//
//    using super::find;
//    iterator find(const Symbol symbol) {
//        static SymbolPost symbol_post{};
//        symbol_post.symbol = symbol;
//        return super::find(symbol_post);
//    }
//    const_iterator find(const Symbol symbol) const {
//        static SymbolPost symbol_post{};
//        symbol_post.symbol = symbol;
//        return super::find(symbol_post);
//    }
//
//    ///returns an iterator to the smallest epsilon, or end() if there is no epsilon
//    const_iterator first_epsilon_it(Symbol first_epsilon) const;
//
//    /**
//     * @brief Get the set of all target state vectors in the @c StatePost.
//     */
//    StateVectorSet get_successors() const;
//
//    /**
//     * @brief Returns a reference to target state vectors for a given symbol in the @c StatePost.
//     */
//    const StateVectorSet& get_successors(Symbol symbol) const;
//
//
//
//     /**
//      * Iterator over all moves (over all labels) in @c StatePost represented as @c Move instances.
//      */
//     Moves moves() const { return { *this, this->cbegin(), this->cend() }; }
//     /**
//      * Iterator over specified moves in @c StatePost represented as @c Move instances.
//      *
//      * @param[in] symbol_post_it First iterator over symbol posts to iterate over.
//      * @param[in] symbol_post_end End iterator over symbol posts (which functions as an sentinel, is not iterated over).
//      */
//     Moves moves(StatePost::const_iterator symbol_post_it, StatePost::const_iterator symbol_post_end) const;
//     /**
//      * Iterator over epsilon moves in @c StatePost represented as @c Move instances.
//      */
//     Moves moves_epsilons(Symbol first_epsilon = EPSILON) const;
//     /**
//      * Iterator over alphabet (normal) symbols (not over epsilons) in @c StatePost represented as @c Move instances.
//      */
//     Moves moves_symbols(Symbol last_symbol = EPSILON - 1) const;
//
//     /**
//      * Count the number of all moves in @c StatePost.
//      */
//     size_t num_of_moves() const;
//}; // class StatePost.

//class Delta {
//public:
//    inline static const StatePost empty_state_post;
//
//    Delta(): state_posts_{} {}
//    Delta(const Delta& other) = default;
//    Delta(Delta&& other) = default;
//    explicit Delta(const size_t n): state_posts_{ n } {}
//
//    Delta& operator=(const Delta& other) = default;
//    Delta& operator=(Delta&& other) = default;
//
//    bool operator==(const Delta& other) const;
//
//    void reserve(const size_t n) {
//        state_posts_.reserve(n);
//    };
//
//    /**
//     * @brief Get constant reference to the state post of @p source.
//     *
//     * If we try to access a state post of a @p source which is present in the automaton as an initial/final state,
//     *  yet does not have allocated space in @c Delta, an @c empty_post is returned. Hence, the function has no side
//     *  effects (no allocation is performed; iterators remain valid).
//     * @param source[in] Source state of a state post to access.
//     * @return State post of @p source.
//     */
//    const StatePost& state_post(const State source) const {
//        if (source >= num_of_states()) {
//            return empty_state_post;
//        }
//        return state_posts_[source];
//    }
//
//    /**
//     * @brief Get constant reference to the state post of @p source.
//     *
//     * If we try to access a state post of a @p source which is present in the automaton as an initial/final state,
//     *  yet does not have allocated space in @c Delta, an @c empty_post is returned. Hence, the function has no side
//     *  effects (no allocation is performed; iterators remain valid).
//     * @param source[in] Source state of a state post to access.
//     * @return State post of @p source.
//     */
//    const StatePost& operator[](const State source) const { return state_post(source); }
//
//    /**
//     * @brief Get mutable (non-constant) reference to the state post of @p source.
//     *
//     * The function allows modifying the state post.
//     *
//     * BEWARE, IT HAS A SIDE EFFECT.
//     *
//     * If we try to access a state post of a @p source which is present in the automaton as an initial/final state,
//     *  yet does not have allocated space in @c Delta, a new state post for @p source will be allocated along with
//     *  all state posts for all previous states. This in turn may cause that the entire post data structure is
//     *  re-allocated. Iterators to @c Delta will get invalidated.
//     * Use the constant 'state_post()' is possible. Or, to prevent the side effect from causing issues, one might want
//     *  to make sure that posts of all states in the automaton are allocated, e.g., write an NFA method that allocate
//     *  @c Delta for all states of the NFA.
//     * @param source[in] Source state of a state post to access.
//     * @return State post of @p source.
//     */
//    StatePost& mutable_state_post(State source);
//
//    /**
//     * @brief Defragment the Delta.
//     *
//     * This function removes all state posts which are not in @p is_staying and renames the remaining state posts
//     * according to @p renaming.
//     *
//     * @param[in] is_staying Boolean vector indicating which states are staying in the Delta.
//     * @param[in] renaming Vector of states to rename the remaining state posts to.
//     * @return Self with defragmented delta.
//     */
//    Delta& defragment(const BoolVector& is_staying, const std::vector<State>& renaming);
//    friend Delta defragment(const Delta& delta, const BoolVector& is_staying, const std::vector<State>& renaming);
//
//    template <typename... Args>
//    StatePost& emplace_back(Args&&... args) {
//    // Forwarding the variadic template pack of arguments to the emplace_back() of the underlying container.
//        return state_posts_.emplace_back(std::forward<Args>(args)...);
//    }
//
//    void clear() { state_posts_.clear(); }
//
//    /**
//     * @brief Allocate state posts up to @p num_of_states states, creating empty @c StatePost for yet unallocated state
//     *  posts.
//     *
//     * @param[in] num_of_states Number of states in @c Delta to allocate state posts for. Have to be at least
//     *  num_of_states() + 1.
//     */
//    void allocate(const size_t num_of_states) {
//        assert(num_of_states >= this->num_of_states());
//        state_posts_.resize(num_of_states);
//    }
//
//    /**
//     * @return Number of states in the whole Delta, including both source and target states.
//     */
//    size_t num_of_states() const { return state_posts_.size(); }
//
//    /**
//     * Check whether the @p state is used in @c Delta.
//     */
//    bool uses_state(const State state) const { return state < num_of_states(); }
//
//    /**
//     * @return Number of transitions in Delta.
//     */
//    size_t num_of_transitions() const;
//
//    void add(State source, Symbol symbol, State target);
//    void add(const Transition& trans) { add(trans.source, trans.symbol, trans.target); }
//    void remove(State source, Symbol symbol, State target);
//    void remove(const Transition& transition) { remove(transition.source, transition.symbol, transition.target); }
//
//    /**
//     * Check whether @c Delta contains a passed transition.
//     */
//    bool contains(State source, Symbol symbol, State target) const;
//    /**
//     * Check whether @c Delta contains a transition passed as a triple.
//     */
//    bool contains(const Transition& transition) const;
//
//    /**
//     * Check whether automaton contains no transitions.
//     * @return True if there are no transitions in the automaton, false otherwise.
//     */
//    bool empty() const;
//
//    /**
//     * @brief Append post vector to the delta.
//     *
//     * @param post_vector Vector of posts to be appended.
//     */
//    void append(const std::vector<StatePost>& post_vector) {
//        for(const StatePost& pst : post_vector) {
//            this->state_posts_.push_back(pst);
//        }
//    }
//
//    /**
//     * @brief Copy posts of delta and apply a lambda update function on each state from
//     * targets.
//     *
//     * IMPORTANT: In order to work properly, the lambda function needs to be
//     * monotonic, that is, the order of states in targets cannot change.
//     *
//     * @param target_renumberer Monotonic lambda function mapping states to different states.
//     * @return std::vector<Post> Copied posts.
//     */
//    std::vector<StatePost> renumber_targets(const std::function<State(State)>& target_renumberer) const;
//
//    /**
//     * @brief Add transitions to multiple destinations
//     *
//     * @param source From
//     * @param symbol Symbol
//     * @param targets Set of states to
//     */
//    void add(State source, Symbol symbol, const StateSet& targets);
//
//    using const_iterator = std::vector<StatePost>::const_iterator;
//    const_iterator cbegin() const { return state_posts_.cbegin(); }
//    const_iterator cend() const { return state_posts_.cend(); }
//    const_iterator begin() const { return state_posts_.begin(); }
//    const_iterator end() const { return state_posts_.end(); }
//
//    class Transitions;
//
//    /**
//     * Iterator over transitions represented as @c Transition instances.
//     */
//    Transitions transitions() const;
//
//    /**
//     * Get transitions leading to @p state_to.
//     * @param state_to[in] Target state for transitions to get.
//     * @return Transitions leading to @p state_to.
//     *
//     * Operation is slow, traverses over all symbol posts.
//     */
//    std::vector<Transition> get_transitions_to(State state_to) const;
//
//    /**
//     * Get transitions from @p state_from to @p state_to.
//     * @param state_from[in] Source state.
//     * @param state_from[in] Target state.
//     * @return Transitions from @p source to @p state_to.
//     *
//     * Operation is slow, traverses over all symbol posts.
//     */
//    std::vector<Transition> get_transitions_between(State state_from, State state_to) const;
//
//    /**
//     * @brief Resize the delta to fit the given @p states.
//     * @tparam States A variadic parameter pack of states to resize the delta for.
//     * @param states States to resize the delta for.
//     */
//    template<typename... States> requires utils::AllOfType<State, States...>
//    Delta& resize_for_states(States... states) {
//        if constexpr (sizeof...(states) > 0) {
//            if (const State max_state{ std::max({ static_cast<State>(states)... }) }; max_state >= num_of_states()) {
//                reserve_on_insert(state_posts_, max_state);
//                state_posts_.resize(max_state + 1);
//            }
//        }
//        return *this;
//    }
//
//    /**
//     * Get the set of states that are successors of the given @p state.
//     * @param[in] state State from which successors are checked.
//     * @return Set of states that are successors of the given @p state.
//     */
//    StateSet get_successors(State state) const;
//
//    const StateSet& get_successors(State state, Symbol symbol) const;
//
//    StateSet get_successors(State state, Symbol symbol, EpsilonClosureOpt epsilon_closure_opt) const;
//
//    /**
//     * Iterate over @p epsilon symbol posts under the given @p state.
//     * @param[in] state State from which epsilon transitions are checked.
//     * @param[in] epsilon User can define his favourite epsilon or used default.
//     * @return An iterator to @c SymbolPost with epsilon symbol. End iterator when there are no epsilon transitions.
//     */
//    StatePost::const_iterator epsilon_symbol_posts(State state, Symbol epsilon = EPSILON) const;
//
//    /**
//     * Iterate over @p epsilon symbol posts under the given @p state_post.
//     * @param[in] state_post State post from which epsilon transitions are checked.
//     * @param[in] epsilon User can define his favourite epsilon or used default.
//     * @return An iterator to @c SymbolPost with epsilon symbol. End iterator when there are no epsilon transitions.
//     */
//    static StatePost::const_iterator epsilon_symbol_posts(const StatePost& state_post, Symbol epsilon = EPSILON);
//
//    /**
//     * @brief Expand @p target_alphabet by symbols from this delta.
//     *
//     * The value of the already existing symbols will NOT be overwritten.
//     */
//    void add_symbols_to(OnTheFlyAlphabet& target_alphabet) const;
//
//    /**
//     * @brief Get the set of symbols used on the transitions in the automaton.
//     *
//     * Does not necessarily have to equal the set of symbols in the alphabet used by the automaton.
//     * @return Set of symbols used on the transitions.
//     */
//    utils::OrdVector<Symbol> get_used_symbols() const;
//
//    utils::OrdVector<Symbol> get_used_symbols_vec() const;
//    std::set<Symbol> get_used_symbols_set() const;
//    utils::SparseSet<Symbol> get_used_symbols_sps() const;
//    std::vector<bool> get_used_symbols_bv() const;
//    BoolVector get_used_symbols_chv() const;
//
//    /**
//     * @brief Get the maximum non-epsilon used symbol.
//     */
//    Symbol get_max_symbol() const;
//
//protected:
//    std::vector<StatePost> state_posts_;
//}; // class Delta.





} // namespace nfta
#endif //NFTA_DELTA_HH
