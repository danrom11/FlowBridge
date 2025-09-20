# FlowBridge
FlowBridge is a cross-language bridge for signals and slots between C++, Objective-C, and Swift. Type-safe events in C++ and convenient hooks in Swift.

---

## ✨ Features
- Type-safe signals in C++
- Automatic bridging to Swift / Objective-C
- Pass values (string, number, bool, binary data) between layers
- No boilerplate callbacks — event-driven design

---

## 📦 Installation

Add FlowBridge as a Swift Package dependency:

```swift
dependencies: [
    .package(url: "https://github.com/danrom11/FlowBridge.git", from: "1.0.0")
]
```

---

## 🔧 Usage

FlowBridge revolves around **signals** (events you define in C++) and **slots** (handlers you connect in Swift/Objective-C).

### 1. Define a signal in C++
Use the `FLOW_SIGNAL` macro to declare a named signal:
```cpp
#include "FlowBridge.hpp"

FLOW_SIGNAL(tick_score);
```
ℹ️ You don’t strictly need to register a signal — emitting will create it automatically.
But calling FlowBridge::registerSignal(tick_score) is recommended for clearer architecture and to document the intent.

### 2. Emit from C++
Emit an event whenever you want:
```cpp
FlowBridge::emit(tick_score, "Hello from C++");
```
You can pass strings, numbers, booleans, or even binary data

### 3. Connect in Swift

In Swift, subscribe by name and handle the payload:
```swift
import FlowBridge

struct FlowSignals {
  static let tick_score = "tick_score"
}

FlowBridge.connect(FlowSignals.tick_score) { data in
  if let text = data as? String {
    print("Received:", text)
  }
}
```
Or you can use a selector:
```swift
import FlowBridge

struct FlowSignals {
  static let tick_score = "tick_score"
}

    
@objc func test(_ data: Any?) {
  print("Data: \(data ?? "none")")
        
  if let stringData = data as? String {
    DispatchQueue.main.async {
      self.nameLabel.text = stringData
    }
  }
}

override func viewDidLoad() {
  super.viewDidLoad()
  nameLabel.text = "0"
        
  FlowBridge.connect(FlowSignals.tick_score, target: self, slot: #selector(test(_:)))
        
  let manager = ManagerWrapper.shared
  manager().start(withInterval: 1000)
}
```

---

## 📂 Examples
A full demo project is available in https://github.com/danrom11/FlowBridge-Examples
