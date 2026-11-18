# Spec Delta

## Purpose
Defines the supported targets (Windows, Linux, Web, WebXR) and their packaging and runtime expectations.

## ADDED Requirements

### Requirement: Supported targets
The game SHALL build from one code base for Windows x64, Linux x64 and the Web (WebAssembly with WebGL 2), and the web build SHALL offer WebXR immersive VR where the browser supports it.

#### Scenario: CI builds all targets
- **WHEN** a commit is pushed to main
- **THEN** Windows, Linux and Web builds are produced successfully

### Requirement: Web delivery
The web build SHALL load the hub with an initial download of at most 10 MB and fetch each portal's assets on demand, and SHALL persist saves in browser storage.

#### Scenario: Lazy portal loading
- **WHEN** the player enters portal 3 for the first time in the web build
- **THEN** portal 3's asset package is downloaded and a loading indicator is shown

#### Scenario: Web save persists
- **WHEN** the browser tab is closed and reopened
- **THEN** snowflakes, Shining Stars, found marks and checkpoints are still present

### Requirement: WebXR entry
The web build SHALL show an "Enter VR" control only when immersive VR is supported and SHALL fall back to flat mode when the session ends.

#### Scenario: XR unsupported
- **WHEN** the browser does not support immersive VR
- **THEN** no "Enter VR" control is shown

### Requirement: Desktop packaging
Desktop releases SHALL be self-contained archives (zip for Windows, tar.gz and AppImage for Linux) that run without installation.

#### Scenario: Run from extracted archive
- **WHEN** the Windows zip is extracted on a clean machine and the executable is started
- **THEN** the hub loads
