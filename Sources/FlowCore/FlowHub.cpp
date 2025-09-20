//
//  FlowHub.cpp
//  FlowBridge
//
// Created by Daniil Arsentev on 20.09.2025.
//

#include "FlowHub.h"

FlowHub& FlowHub::instance() {
    static FlowHub hub;
    return hub;
}

std::shared_ptr<FlowSignal> FlowHub::getOrCreate(const std::string& name) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = map_.find(name);
    if (it == map_.end()) {
        it = map_.emplace(name, std::make_shared<FlowSignal>()).first;
    }
    return it->second;
}

void FlowHub::unregisterSignal(const std::string& name) {
    std::lock_guard<std::mutex> lk(mutex_);
    map_.erase(name);
}

std::vector<std::string> FlowHub::allSignalNames() const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<std::string> res;
    res.reserve(map_.size());
    for (auto& kv : map_) res.push_back(kv.first);
    return res;
}

size_t FlowHub::count() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return map_.size();
}
