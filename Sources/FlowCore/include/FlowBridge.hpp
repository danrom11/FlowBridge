//
//  FlowBridge.hpp
//  FlowBridge
//
// Created by Daniil Arsentev on 20.09.2025.
//

#ifndef FlowBridge_hpp
#define FlowBridge_hpp

#include <string>
#include <functional>
#include <any>
#include "FlowHub.h"
#include "FlowSignal.h"

namespace FlowBridge {

// ---------- BASIC C++ API by string ----------

using Connection = FlowSignal::Connection;

inline void registerSignal(const std::string& name) {
    (void)FlowHub::instance().getOrCreate(name);
}

inline void unregisterSignal(const std::string& name) {
    FlowHub::instance().unregisterSignal(name);
}

template<typename T>
inline void emit(const std::string& name, const T& value) {
    FlowHub::instance().getOrCreate(name)->emit<T>(value);
}

inline void emit(const std::string& name) { // пустая посылка
    FlowHub::instance().getOrCreate(name)->emit_any();
}

template<typename T>
inline Connection connect(const std::string& name, std::function<void(const T&)> slot) {
    return FlowHub::instance().getOrCreate(name)->connect<T>(std::move(slot));
}

// «сырой» any-слот
inline Connection connect_any(const std::string& name, FlowSignal::FlowSlotAny slot) {
    return FlowHub::instance().getOrCreate(name)->connect_any(std::move(slot));
}

inline size_t signalCount() { return FlowHub::instance().count(); }

// ------------------------ CONVENIENT SIGNALS ------------------------

/**
* A typed signal object that stores its name (C-string).
* Created by the FLOW_SIGNAL(name) macro.
*/
template <typename Tag>
struct Signal {
    constexpr explicit Signal(const char* n) : n_(n) {}
    constexpr const char* name() const { return n_; }
private:
    const char* n_;
};

// The macro declares a unique tag type and an inline signal variable.
// Example: FLOW_SIGNAL(tickScore);
// Gets the `tickScore` variable, which can be passed to emit/connect.
#define FLOW_SIGNAL(name)                             \
    struct name##_SignalTag {};                       \
    inline constexpr ::FlowBridge::Signal<name##_SignalTag> name{#name}

// Overloads of emit/connect under Signal<...>
template<typename Tag, typename T>
inline void emit(const Signal<Tag>& sig, const T& value) {
    FlowHub::instance().getOrCreate(sig.name())->template emit<T>(value);
}

template<typename Tag>
inline void emit(const Signal<Tag>& sig) {
    FlowHub::instance().getOrCreate(sig.name())->emit_any();
}

template<typename Tag, typename T>
inline Connection connect(const Signal<Tag>& sig, std::function<void(const T&)> slot) {
    return FlowHub::instance().getOrCreate(sig.name())->template connect<T>(std::move(slot));
}

template<typename Tag>
inline Connection connect_any(const Signal<Tag>& sig, FlowSignal::FlowSlotAny slot) {
    return FlowHub::instance().getOrCreate(sig.name())->connect_any(std::move(slot));
}

template<typename Tag>
inline void registerSignal(const Signal<Tag>& sig) {
    (void)FlowHub::instance().getOrCreate(sig.name());
}

template<typename Tag>
inline void unregisterSignal(const Signal<Tag>& sig) {
    FlowHub::instance().unregisterSignal(sig.name());
}

// --------------------------- CONVENIENT SLOTS ---------------------------

/**
* Typed slot. Stores a std::function<void(const T&)>.
* Created using make_slot<T>(callable) or the FLOW_SLOT(name, T, callable) macro.
*/
template<typename T>
struct Slot {
    using value_type = T;
    std::function<void(const T&)> fn;
};

// Helper for creating a slot with type T inference from a template.
template<typename T, typename F>
inline Slot<T> make_slot(F&& f) {
    return Slot<T>{ std::function<void(const T&)>(std::forward<F>(f)) };
}

// Macro declares an inline slot variable:
//   FLOW_SLOT(onTick, int, [](int v){ ... });
#define FLOW_SLOT(name, T, callable) \
    inline ::FlowBridge::Slot<T> name = ::FlowBridge::make_slot<T>(callable)

// Connection via a slot object (type is inferred from Slot<T>)
template<typename Tag, typename T>
inline Connection connect(const Signal<Tag>& sig, const Slot<T>& slot) {
    return connect<T>(sig, slot.fn);
}

// Support for "string" signal + Slot<T> if needed
template<typename T>
inline Connection connect(const std::string& name, const Slot<T>& slot) {
    return connect<T>(name, slot.fn);
}

} // namespace FlowBridge



#endif /* FlowBridge_hpp */
