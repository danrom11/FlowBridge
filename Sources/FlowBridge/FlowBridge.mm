//
//  FlowBridge.mm
//  FlowBridge
//
// Created by Daniil Arsentev on 12.09.2025.
//

#import "FlowBridge.h"
#import <Foundation/Foundation.h>

#include <string>
#include <vector>
#include <any>
#include <unordered_map>
#include <mutex>
#include <cstring>
#include <cstdlib>

#import "FlowSignal.h"
#import "FlowHub.h"

// ====== INTERNAL STORAGE FOR CONNECTIONS (so they don't get destroyed right away) ======
namespace {
    // Key is the signal name; value is the set of held connections.
    std::unordered_map<std::string, std::vector<std::shared_ptr<FlowSignal::Connection>>> g_connStore;
    std::mutex g_connMutex;

    void storeConnection(const std::string& name, FlowSignal::Connection&& c) {
        std::lock_guard<std::mutex> lk(g_connMutex);
        auto& vec = g_connStore[name];
        vec.emplace_back(std::make_shared<FlowSignal::Connection>(std::move(c)));
    }

    void clearConnections(const std::string& name) {
        std::lock_guard<std::mutex> lk(g_connMutex);
        g_connStore.erase(name);
    }
}

// ===== C-COMPATIBLE PART =====

extern "C" {

void FlowBridge_registerSignal(FlowSignalNameC signalName) {
    if (!signalName) return;
    @autoreleasepool {
        std::string key(signalName);
        FlowHub::instance().getOrCreate(key);
    }
}

void FlowBridge_unregisterSignal(FlowSignalNameC signalName) {
    if (!signalName) return;
    @autoreleasepool {
        FlowHub::instance().unregisterSignal(signalName);
        clearConnections(signalName);
    }
}

void FlowBridge_connectFunction(FlowSignalNameC signalName, FlowSlotCFunction slot) {
    if (!signalName || !slot) return;
    @autoreleasepool {
        auto sig = FlowHub::instance().getOrCreate(signalName);
        auto conn = sig->connect_any([slot](const std::any& a){
            const char* cstr = nullptr;
            if (!a.has_value()) {
                cstr = nullptr;
            } else if (a.type() == typeid(std::string)) {
                cstr = std::any_cast<const std::string&>(a).c_str();
            } else if (a.type() == typeid(int)) {
                static thread_local std::string buf;
                buf = std::to_string(std::any_cast<const int&>(a));
                cstr = buf.c_str();
            } else if (a.type() == typeid(double)) {
                static thread_local std::string buf;
                buf = std::to_string(std::any_cast<const double&>(a));
                cstr = buf.c_str();
            } else if (a.type() == typeid(bool)) {
                static thread_local std::string buf;
                buf = std::any_cast<const bool&>(a) ? "true" : "false";
                cstr = buf.c_str();
            } else if (a.type() == typeid(std::vector<uint8_t>)) {
                static thread_local std::string buf;
                buf = "<bytes>";
                cstr = buf.c_str();
            } else {
                static thread_local std::string buf;
                buf = "<unsupported>";
                cstr = buf.c_str();
            }
            slot(cstr);
        });
        storeConnection(signalName, std::move(conn));
    }
}

void FlowBridge_emit(FlowSignalNameC signalName, const char* data) {
    if (!signalName) return;
    @autoreleasepool {
        auto sig = FlowHub::instance().getOrCreate(signalName);
        if (data) {
            sig->emit<std::string>(std::string(data));
        } else {
            sig->emit_any();
        }
    }
}

void FlowBridge_emitEmpty(FlowSignalNameC signalName) {
    if (!signalName) return;
    @autoreleasepool {
        FlowHub::instance().getOrCreate(signalName)->emit_any();
    }
}

const char* FlowBridge_createString(const char* source) {
    if (!source) return nullptr;
    size_t length = strlen(source);
    char* copy = (char*)malloc(length + 1);
    if (copy) strcpy(copy, source);
    return copy;
}

void FlowBridge_freeString(const char* str) {
    if (str) free((void*)str);
}

} // extern "C"

// ===== Objective-C API =====

@implementation FlowBridge

+ (std::shared_ptr<FlowSignal>)getOrCreateSignal:(NSString *)signalName {
    std::string key = [signalName UTF8String];
    return FlowHub::instance().getOrCreate(key);
}

+ (void)registerSignal:(FlowSignalName)signalName {
    (void)[self getOrCreateSignal:signalName];
}

+ (void)unregisterSignal:(FlowSignalName)signalName {
    FlowHub::instance().unregisterSignal([signalName UTF8String]);
    clearConnections([signalName UTF8String]);
}

+ (void)connect:(FlowSignalName)signalName target:(id)target slot:(SEL)slot {
    auto signal = [self getOrCreateSignal:signalName];

    auto conn = signal->connect_any([target, slot](const std::any& data) {
        @autoreleasepool {
            id nsData = nil;
            try {
                if (!data.has_value()) {
                    nsData = nil;
                } else if (data.type() == typeid(std::string)) {
                    const auto& s = std::any_cast<const std::string&>(data);
                    nsData = [NSString stringWithUTF8String:s.c_str()];
                } else if (data.type() == typeid(int)) {
                    nsData = @(std::any_cast<const int&>(data));
                } else if (data.type() == typeid(double)) {
                    nsData = @(std::any_cast<const double&>(data));
                } else if (data.type() == typeid(bool)) {
                    nsData = @(std::any_cast<const bool&>(data));
                } else if (data.type() == typeid(std::vector<uint8_t>)) {
                    const auto& vec = std::any_cast<const std::vector<uint8_t>&>(data);
                    nsData = [NSData dataWithBytes:vec.data() length:vec.size()];
                } else {
                    nsData = nil;
                }
            } catch (...) {
                nsData = nil;
            }

            if ([target respondsToSelector:slot]) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
                [target performSelector:slot withObject:nsData];
#pragma clang diagnostic pop
            }
        }
    });
    storeConnection([signalName UTF8String], std::move(conn));
}

+ (void)connect:(FlowSignalName)signalName slot:(void (^)(id _Nullable))slot {
    auto signal = [self getOrCreateSignal:signalName];

    auto conn = signal->connect_any([slot](const std::any& data) {
        @autoreleasepool {
            id nsData = nil;
            try {
                if (!data.has_value()) {
                    nsData = nil;
                } else if (data.type() == typeid(std::string)) {
                    const auto& s = std::any_cast<const std::string&>(data);
                    nsData = [NSString stringWithUTF8String:s.c_str()];
                } else if (data.type() == typeid(int)) {
                    nsData = @(std::any_cast<const int&>(data));
                } else if (data.type() == typeid(double)) {
                    nsData = @(std::any_cast<const double&>(data));
                } else if (data.type() == typeid(bool)) {
                    nsData = @(std::any_cast<const bool&>(data));
                } else if (data.type() == typeid(std::vector<uint8_t>)) {
                    const auto& vec = std::any_cast<const std::vector<uint8_t>&>(data);
                    nsData = [NSData dataWithBytes:vec.data() length:vec.size()];
                } else {
                    nsData = nil;
                }
            } catch (...) {
                nsData = nil;
            }

            if (slot) slot(nsData);
        }
    });
    storeConnection([signalName UTF8String], std::move(conn));
}

+ (void)disconnect:(FlowSignalName)signalName target:(id)target {
    [self disconnectAll:signalName];
}

+ (void)disconnectAll:(FlowSignalName)signalName {
    auto signal = [self getOrCreateSignal:signalName];
    signal->disconnectAll();
    clearConnections([signalName UTF8String]);
}

+ (void)emit:(FlowSignalName)signalName data:(id)data {
    auto signal = [self getOrCreateSignal:signalName];
    @autoreleasepool {
        if (!data || data == (id)kCFNull) {
            signal->emit_any();
        } else if ([data isKindOfClass:[NSString class]]) {
            std::string str = [(NSString *)data UTF8String];
            signal->emit<std::string>(str);
        } else if ([data isKindOfClass:[NSNumber class]]) {
            NSNumber *number = (NSNumber *)data;
            const char *t = [number objCType];
            if (strcmp(t, @encode(int)) == 0) {
                signal->emit<int>([number intValue]);
            } else if (strcmp(t, @encode(double)) == 0) {
                signal->emit<double>([number doubleValue]);
            } else if (strcmp(t, @encode(BOOL)) == 0) {
                signal->emit<bool>([number boolValue]);
            } else if (strcmp(t, @encode(float)) == 0) {
                signal->emit<double>((double)[number floatValue]);
            } else {
                signal->emit<double>([number doubleValue]);
            }
        } else if ([data isKindOfClass:[NSData class]]) {
            NSData *nsData = (NSData *)data;
            std::vector<uint8_t> bytes(nsData.length);
            [nsData getBytes:bytes.data() length:nsData.length];
            signal->emit<std::vector<uint8_t>>(bytes);
        } else {
            std::string desc([[data description] UTF8String]);
            signal->emit<std::string>(desc);
        }
    }
}

+ (void)emit:(FlowSignalName)signalName {
    [self emit:signalName data:nil];
}

+ (NSUInteger)getSignalCount {
    return (NSUInteger)FlowHub::instance().count();
}

+ (NSArray<NSString *> *)getAllSignalNames {
    auto names = FlowHub::instance().allSignalNames();
    NSMutableArray *result = [NSMutableArray arrayWithCapacity:names.size()];
    for (auto &s : names) [result addObject:[NSString stringWithUTF8String:s.c_str()]];
    return result;
}

@end
