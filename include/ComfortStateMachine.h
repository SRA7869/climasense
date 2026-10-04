#pragma once
#include "ComfortCalculator.h"

enum class ComfortState { COMFORTABLE, WARM, HUMID, VERY_UNCOMFORTABLE };

const char* toString(ComfortState s);

struct StateTransition {
    bool changed;
    ComfortState from;
    ComfortState to;
};

class ComfortStateMachine {
public:
    StateTransition update(const ComfortAssessment& a);
    ComfortState state() const { return state_; }
    int transitionCount() const { return transitions_; }

private:
    static ComfortState decide(const ComfortAssessment& a);

    ComfortState state_ = ComfortState::COMFORTABLE;
    int transitions_ = 0;
};
