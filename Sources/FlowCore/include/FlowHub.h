//
//  FlowHub.h
//  FlowBridge
//
// Created by Daniil Arsentev on 20.09.2025.
//

#ifndef FlowHub_h
#define FlowHub_h

#include <memory>
#include <string>
#include <map>
#include <mutex>
#include <vector>
#include "FlowSignal.h"

class FlowHub {
public:
    static FlowHub& instance();

    std::shared_ptr<FlowSignal> getOrCreate(const std::string& name);
    void unregisterSignal(const std::string& name);
    std::vector<std::string> allSignalNames() const;
    size_t count() const;

private:
    FlowHub() = default;
    FlowHub(const FlowHub&) = delete;
    FlowHub& operator=(const FlowHub&) = delete;

    mutable std::mutex mutex_;
    std::map<std::string, std::shared_ptr<FlowSignal>> map_;
};


#endif /* FlowHub_h */
