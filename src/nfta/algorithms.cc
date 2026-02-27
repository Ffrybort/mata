#include "mata/nfta/algorithms.hh"


namespace mata::nfta {
    bool is_deterministic(const Nfta& /*automaton*/)
    {
        throw std::runtime_error("not implemented");
    }

    bool is_complete(const Nfta& automaton, Alphabet& shared_alphabet) {
        throw std::runtime_error("not implemented");
    }

    void minimise(Nfta /*automaton*/)
    {
        throw std::runtime_error("not implemented");
    }

    void determinise(Nfta /*automaton*/)
    {
        throw std::runtime_error("not implemented");
    }

    void complete(Nfta /*automaton*/)
    {
        throw std::runtime_error("not implemented");
    }

    Nfta automaton_union(Nfta /*aut1*/, Nfta /*aut2*/)
    {
        throw std::runtime_error("not implemented");
    }

    Nfta automaton_complementation(Nfta /*aut1*/, Nfta /*aut2*/)
    {
        throw std::runtime_error("not implemented");
    }

    Nfta automaton_intersection(Nfta /*aut1*/, Nfta /*aut2*/)
    {
        throw std::runtime_error("not implemented");
    }

} // namespace mata::nfta
