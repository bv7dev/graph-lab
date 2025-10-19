# cppseed

cppseed 🌱 grow clean and modern C++23 projects, not boilerplate.

A lean C++23 project template for Linux with fast builds, testing, and a simple developer workflow.

Linux-only (tested on Ubuntu 24.04).

## Quick start

```bash
# Clone the repository
git clone <your-repo> && cd cppseed

# Set up the 'dev' helper (adds a local shim)
./scripts/setup.sh

# Install system dependencies (first time only)
dev install

# Build everything (Release) or iterate fast (Debug)
dev release
# or
dev debug

# Run an app from src/apps (auto-builds on first run)
dev run example
```

## What’s inside

- C++23, CMake + Ninja
- Strict warnings for apps: -Wall -Wextra -Wpedantic -Werror
- doctest via CMake FetchContent (auto-fetched)
- Auto-discovered targets:
	- Apps: every `src/apps/*.cpp` becomes an executable
	- Tests: every `tests/test_*.cpp` becomes a test executable
- Dev helpers: one-liner builds, tests, formatting, lint
- clang-format, clangd-friendly setup

## Focus mode (VS Code)

For VS Code users: `dev focus` hides build/config files in the Explorer to reduce noise; `dev config` shows them again. These commands toggle the `files.exclude` section in `.vscode/settings.json` and don’t affect the actual build.

```bash
dev focus   # hide build/config files in VS Code
dev config  # show them again
```

### VS Code tasks

This repo includes tasks for a smooth loop:

- build-debug / build-release
- build-debug-current
- run-current (runs the currently open app/test file)
- run-tests
- format-code / lint-code
- clean-build

## Commands

| Command | What it does |
| --- | --- |
| `dev debug [target]` | Build Debug (target must match a file in `src/apps`, or `all`) |
| `dev release [target]` | Build Release (target must match a file in `src/apps`, or `all`) |
| `dev run [target]` | Run an app. If the executable is missing, it configures Debug and builds the target, then runs it. |
| `dev test` | Build (Debug) and run tests with ctest |
| `dev format [scope]` | Format code with clang-format (`all`/`src`/`include`/path) |
| `dev lint [scope]` | Run diagnostics via clangd-tidy |
| `dev clean` | Remove the build dir and tooling cache |
| `dev all` | Clean, build (Release), run tests, format |
| `dev install` | Install system dependencies (Ubuntu 24.04) |
| `dev focus` / `dev config` | Toggle visibility of build/config files in VS Code |

Notes:
- `dev run` will try to build missing targets in Debug automatically.
- When no target is provided and only one app exists, `dev run` will run that single app.

## Structure

- `src/apps/` — application entry points (each `*.cpp` becomes an executable)
- `include/` — public headers
- `tests/` — test files (`test_*.cpp`)
- `scripts/` — developer helpers (`dev.sh`, setup)
- `build/` — build output (ignored)

## Add a new app

Drop a file in `src/apps/` and it becomes a runnable target:

```cpp
// src/apps/hello.cpp
#include <print>

int main() {
	std::println("Hello from cppseed!");
	return 0;
}
```

Build and run it:

```bash
dev debug hello
dev run hello
```

## Tests

Add test files as `tests/test_*.cpp` (doctest is already available). Run:

```bash
dev test
```
