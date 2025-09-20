//
//  FlowSignal.cpp
//  FlowBridge
//
// Created by Daniil Arsentev on 12.09.2025.
//

#include "FlowSignal.h"

void FlowSignal::Connection::reset() {
    if (owner_ && id_) {
        owner_->disconnect(id_);
        owner_ = nullptr;
        id_ = 0;
    }
}

FlowSignal::Connection FlowSignal::connect_any(FlowSlotAny slot) {
    std::lock_guard<std::mutex> lk(mutex_);
    const auto id = next_id_++;
    slots_.emplace(id, std::move(slot));
    return Connection{ this, id };
}

void FlowSignal::disconnect(ConnectionId id) {
    std::lock_guard<std::mutex> lk(mutex_);
    slots_.erase(id);
}

void FlowSignal::disconnectAll() {
    std::lock_guard<std::mutex> lk(mutex_);
    slots_.clear();
}

void FlowSignal::emit_any(const std::any& data) {
    std::vector<FlowSlotAny> copy;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        copy.reserve(slots_.size());
        for (auto& kv : slots_) copy.push_back(kv.second);
    }
    for (auto& s : copy) s(data);
}

