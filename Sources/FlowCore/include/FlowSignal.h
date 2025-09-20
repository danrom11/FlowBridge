//
//  FlowSignal.h
//  FlowBridge
//
// Created by Daniil Arsentev on 12.09.2025.
//

#ifndef FlowSignal_h
#define FlowSignal_h

#include <functional>
#include <vector>
#include <memory>
#include <mutex>
#include <any>
#include <atomic>
#include <unordered_map>

class FlowSignal {
public:
    using FlowSlotAny = std::function<void(const std::any&)>;
    using ConnectionId = uint64_t;

    class Connection {
    public:
        Connection() = default;
        Connection(FlowSignal* owner, ConnectionId id) : owner_(owner), id_(id) {}
        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;
        Connection(Connection&& other) noexcept { *this = std::move(other); }
        Connection& operator=(Connection&& other) noexcept {
            if (this != &other) {
                reset();
                owner_ = other.owner_;
                id_ = other.id_;
                other.owner_ = nullptr;
                other.id_ = 0;
            }
            return *this;
        }
        ~Connection() { reset(); }
        void reset();

    private:
        FlowSignal* owner_ = nullptr;
        ConnectionId id_ = 0;
    };

    Connection connect_any(FlowSlotAny slot);

    // Type-safe connection: call the slot only if the type matches exactly
    template<typename T>
    Connection connect(std::function<void(const T&)> slot) {
        return connect_any([s = std::move(slot)](const std::any& a){
            if (a.has_value() && a.type() == typeid(T)) {
                s(std::any_cast<const T&>(a));
            }
        });
    }

    void disconnect(ConnectionId id);
    void disconnectAll();

    void emit_any(const std::any& data = std::any());

    template<typename T>
    void emit(const T& data) { emit_any(std::any(data)); }

private:
    std::mutex mutex_;
    std::unordered_map<ConnectionId, FlowSlotAny> slots_;
    std::atomic<ConnectionId> next_id_{1};
};


#endif /* FlowSignal_h */
