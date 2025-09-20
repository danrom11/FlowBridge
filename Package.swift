// swift-tools-version:5.9
import PackageDescription

let package = Package(
    name: "FlowBridge",
    platforms: [
        .iOS(.v13), .macOS(.v10_15), .tvOS(.v13), .watchOS(.v6)
    ],
    products: [
        .library(name: "FlowCore", targets: ["FlowCore"]),
        .library(name: "FlowBridge", targets: ["FlowBridge"]),
    ],
    targets: [
        .target(
            name: "FlowCore",
            path: "Sources/FlowCore",
            publicHeadersPath: "include"
        ),
        .target(
            name: "FlowBridge",
            dependencies: ["FlowCore"],
            path: "Sources/FlowBridge",
            publicHeadersPath: "include"
        ),
    ],
    
    cLanguageStandard: .c11,
    cxxLanguageStandard: .cxx17
)

