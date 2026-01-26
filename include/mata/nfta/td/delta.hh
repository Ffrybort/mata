/**
 * @file delta.hh
 * @brief A set of all transition rules.
 *
 */

#ifndef NFTA_DELTA_HH
#define NFTA_DELTA_HH

#include <mata/nfta/types.hh>
#include <mata/alphabet.hh>

namespace mata::nfta
{
/**
 * @brief
 */
struct Transition
{
    Symbol symbol;
    State single;
    std::vector<State> tuple;

    explicit Transition(
        const Symbol symbol = {},
        const State single = {},
        const std::vector<State>& tuple = {}
    )
        : symbol(symbol),
          single(single),
          tuple(tuple)
    {
    }

    bool operator<(const Transition& other) const {
        if (single != other.single) return single < other.single;
        if (symbol != other.symbol) return symbol < other.symbol;
        if (tuple.size() != other.tuple.size()) return tuple.size() < other.tuple.size();
        return tuple < other.tuple;
    }

    bool operator==(const Transition& other) const {
        return single == other.single
        && symbol == other.symbol
        && tuple == other.tuple;
        }
    };

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
        std::vector<Transition> get_transitions() const
        {
            return std::vector<Transition>(transitions.begin(), transitions.end());
        }

        /**
         * @brief Returns an iterator to the beginning of the transitions.
         * @return Iterator to the first transition.
         */
        auto begin() const { return transitions.begin(); }

        /**
         * @brief Returns an iterator to the end of the transitions.
         * @return Iterator past the last transition.
         */
        auto end() const { return transitions.end(); }

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

        bool operator==(const Delta& other) const {
            return transitions == other.transitions;
        }

    }; // class delta


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

} // namespace nfta
#endif //NFTA_DELTA_HH
