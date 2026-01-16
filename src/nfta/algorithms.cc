#include "algorithms.hh"

namespace mata::nfta
{

    bool isDeterministic(const Nfta& /*automaton*/)
    {
        throw std::runtime_error("not implemented");
    }

    bool isMinimal(const Nfta& /*automaton*/)
    {
        throw std::runtime_error("not implemented");
    }

    bool isComplete(const Nfta& /*automaton*/)
    {
        throw std::runtime_error("not implemented");
    }

    void minimize(Nfta /*automaton*/)
    {
        throw std::runtime_error("not implemented");
    }

    void determinize(Nfta /*automaton*/)
    {
        throw std::runtime_error("not implemented");
    }

    void complete(Nfta /*automaton*/)
    {
        throw std::runtime_error("not implemented");
    }

    Nfta automatonUnion(Nfta /*aut1*/, Nfta /*aut2*/)
    {
        throw std::runtime_error("not implemented");
    }

    Nfta automatonComplementation(Nfta /*aut1*/, Nfta /*aut2*/)
    {
        throw std::runtime_error("not implemented");
    }

    Nfta automatonIntersection(Nfta /*aut1*/, Nfta /*aut2*/)
    {
        throw std::runtime_error("not implemented");
    }

} // namespace mata::nfta
