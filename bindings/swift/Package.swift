// swift-tools-version:5.5
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "RobotsTxt",
    products: [
        .library(
            name: "RobotsTxt",
            targets: ["RobotsTxt"]),
    ],
    dependencies: [],
    targets: [
        .systemLibrary(
            name: "CRobots",
            path: "Sources/CRobots",
            pkgConfig: nil,
            providers: []
        ),
        .target(
            name: "RobotsTxt",
            dependencies: ["CRobots"],
            path: "Sources/RobotsTxt"
        ),
        .testTarget(
            name: "RobotsTxtTests",
            dependencies: ["RobotsTxt"],
            path: "Tests/RobotsTxtTests"
        ),
    ]
)
