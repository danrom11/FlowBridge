//
//  FlowBridge.h
//  FlowBridge
//
// Created by Daniil Arsentev 09.12.2025.
//

#ifndef FlowBridge_h
#define FlowBridge_h

// Forward declarations for C compatibility
#ifdef __cplusplus
extern "C" {
#endif

typedef const char* FlowSignalNameC;
typedef void (*FlowSlotCFunction)(const char*);

void FlowBridge_registerSignal(FlowSignalNameC signalName);
void FlowBridge_unregisterSignal(FlowSignalNameC signalName);
void FlowBridge_connectFunction(FlowSignalNameC signalName, FlowSlotCFunction slot);
void FlowBridge_emit(FlowSignalNameC signalName, const char* data);
void FlowBridge_emitEmpty(FlowSignalNameC signalName);
const char* FlowBridge_createString(const char* source);
void FlowBridge_freeString(const char* str);

#define FLOW_SIGNAL(name) \
    static constexpr FlowSignalNameC name = #name;

#define FLOW_SLOT(func) (FlowSlotCFunction)(func)

#ifdef __cplusplus
}
#endif

// Objective-C part - only if compiled as Objective-C
#if defined(__OBJC__)

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NSString *FlowSignalName;

@interface FlowBridge : NSObject

+ (void)registerSignal:(FlowSignalName)signalName;
+ (void)unregisterSignal:(FlowSignalName)signalName;
+ (void)connect:(FlowSignalName)signalName target:(id)target slot:(SEL)slot;
+ (void)connect:(FlowSignalName)signalName slot:(void (^)(id _Nullable))slot;
+ (void)disconnect:(FlowSignalName)signalName target:(id)target;
+ (void)disconnectAll:(FlowSignalName)signalName;
+ (void)emit:(FlowSignalName)signalName data:(id _Nullable)data;
+ (void)emit:(FlowSignalName)signalName;
+ (NSUInteger)getSignalCount;
+ (NSArray<NSString *> *)getAllSignalNames;

#define FLOW_SIGNAL_DEF(name) static NSString * const name = @#name;
#define FLOW_SLOT_SEL(method) @selector(method:)

@end

NS_ASSUME_NONNULL_END

#endif // __OBJC__

#endif /* FlowBridge_h */
