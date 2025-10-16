#pragma once

#include "sml/model.h"

namespace sm {

template <typename E, model::Guard<E> G, model::Action<E> A>
struct GuardedEventWithAction {
    using Raw = E;

    GuardedEventWithAction(G guard, A action)
        : guard_{stdlike::move(guard)}
        , action_{stdlike::move(action)} {}

    // Not copyable
    GuardedEventWithAction(const GuardedEventWithAction&) = delete;
    GuardedEventWithAction& operator=(const GuardedEventWithAction&) = delete;

    GuardedEventWithAction(GuardedEventWithAction&& rhs)
        : guard_{stdlike::move(rhs.guard_)}
        , action_{stdlike::move(rhs.action_)} {}

    bool willAccept(const E& event) {
        return guard_(event);
    }

    void accept(const E& event) {
        action_(event);
    }

 private:
    G guard_;
    A action_;
};

template <typename E, model::Guard<E> G>
struct GuardedEvent {
    using Raw = E;

    explicit GuardedEvent(G guard) : guard_{stdlike::move(guard)} {}

    // Not copyable
    GuardedEvent(const GuardedEvent&) = delete;
    GuardedEvent& operator=(const GuardedEvent&) = delete;

    GuardedEvent(GuardedEvent&& rhs) : guard_{stdlike::move(rhs.guard_)} {}

    template <model::Action<E> A>
    auto operator/(A action) {
        return GuardedEventWithAction<E, G, A>{
            stdlike::move(guard_),
            stdlike::move(action),
        };
    }

    bool willAccept(const E& event) {
        return guard_(event);
    }

    void accept(const E&) const {}

 private:
    G guard_;
};

template <typename E, model::Action<E> A>
struct EventWithAction {
    using Raw = E;

    explicit EventWithAction(A action) : action_{stdlike::move(action)} {}

    // Not copyable
    EventWithAction(const EventWithAction&) = delete;
    EventWithAction& operator=(const EventWithAction&) = delete;

    EventWithAction(EventWithAction&& rhs) : action_{stdlike::move(rhs.action_)} {}

    bool willAccept(const E&) const {
        return true;
    }

    void accept(const E& event) {
        action_(event);
    }

 private:
    A action_;
};

template <typename E>
struct Event {
    using Raw = E;

    template <model::Guard<E> G>
    auto operator[](G guard) const {
        return GuardedEvent<E, G>{stdlike::move(guard)};
    }

    template <model::Action<E> A>
    auto operator/(A action) const {
        return EventWithAction<E, A>{stdlike::move(action)};
    }

    bool willAccept(const E&) const {
        return true;
    }

    void accept(const E&) const {}
};

class OnEnterEvent {};
class OnExitEvent {};

template <typename E>
[[maybe_unused]] static constexpr Event<E> event;
[[maybe_unused]] static constexpr auto onEnter = event<OnEnterEvent>;
[[maybe_unused]] static constexpr auto onExit = event<OnExitEvent>;

}  // namespace sm
