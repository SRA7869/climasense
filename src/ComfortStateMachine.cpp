#include "ComfortStateMachine.h"

const char* toString(ComfortState s) {
    switch (s) {
        case ComfortState::COMFORTABLE:        return "COMFORTABLE";
        case ComfortState::WARM:               return "WARM";
        case ComfortState::HUMID:              return "HUMID";
        case ComfortState::VERY_UNCOMFORTABLE: return "VERY_UNCOMFORTABLE";
    }
    return "UNKNOWN";
}

ComfortState ComfortStateMachine::decide(const ComfortAssessment& a) {
    const bool hot   = (a.temp != TempLevel::NORMAL);
    const bool humid = (a.humidity == HumidityLevel::HIGH);

    if (a.temp == TempLevel::VERY_HIGH) return ComfortState::VERY_UNCOMFORTABLE;
    if (hot && humid)                   return ComfortState::VERY_UNCOMFORTABLE;
    if (hot)                            return ComfortState::WARM;
    if (humid)                          return ComfortState::HUMID;
    return ComfortState::COMFORTABLE;
}

StateTransition ComfortStateMachine::update(const ComfortAssessment& a) {
    const ComfortState next = decide(a);
    StateTransition t{next != state_, state_, next};
    if (t.changed) {
        state_ = next;
        ++transitions_;
    }
    return t;
}
