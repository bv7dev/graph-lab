#!/usr/bin/env zsh

# Simple C++ Project Developer Script
# Usage: ./dev.sh [debug|release|clean|test|format|run|all]

set -e

# Get the directory where this script is located and cd to project root
SCRIPT_DIR="$(cd "$(dirname "${(%):-%x}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_ROOT"

PROJECT_NAME="simple-cpp-23-template"
BUILD_DIR="build"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

source "$SCRIPT_DIR/devtools.sh"

# Check if command exists
command_exists() { command -v "$1" >/dev/null 2>&1; }

# Clean build directory
clean() {
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"

    echo "Cleaning cache directory..."
    rm -rf .cache/

    echo "Removing symlink to compile_commands.json..."
    rm -f compile_commands.json
}

# Configure and build for debug
debug() {
    local TARGET="${1:-all}"

    if [ "$TARGET" = "all" ]; then
        echo "Building all targets (debug)..."
    else
        echo "Building $TARGET (debug)..."
    fi

    mkdir -p "$BUILD_DIR"

    # Only reconfigure if CMakeLists.txt changed or build directory is missing CMakeCache.txt
    if [ ! -f "$BUILD_DIR/CMakeCache.txt" ] || [ CMakeLists.txt -nt "$BUILD_DIR/CMakeCache.txt" ]; then
        echo "Configuring..."
        cmake -S . -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
    fi

    # Build specific target or all
    if [ "$TARGET" = "all" ]; then
        cmake --build "$BUILD_DIR" --parallel
    else
        # Verify target exists
        if [ ! -f "src/apps/$TARGET.cpp" ]; then
            echo -e "${RED}Error: Target '$TARGET' not found${NC}"
            echo "Available targets:"
            find src/apps -name "*.cpp" -exec basename {} .cpp \; | sort
            return 1
        fi
        cmake --build "$BUILD_DIR" --target "$TARGET" --parallel
    fi

    ln -sf "$BUILD_DIR/compile_commands.json" compile_commands.json
}

# Configure and build for release
release() {
    local TARGET="${1:-all}"

    if [ "$TARGET" = "all" ]; then
        echo "Building all targets (release)..."
    else
        echo "Building $TARGET (release)..."
    fi

    mkdir -p "$BUILD_DIR"

    # Only reconfigure if CMakeLists.txt changed or build directory is missing CMakeCache.txt
    if [ ! -f "$BUILD_DIR/CMakeCache.txt" ] || [ CMakeLists.txt -nt "$BUILD_DIR/CMakeCache.txt" ]; then
        echo "Configuring..."
        cmake -S . -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
    fi

    # Build specific target or all
    if [ "$TARGET" = "all" ]; then
        cmake --build "$BUILD_DIR" --parallel
    else
        # Verify target exists
        if [ ! -f "src/apps/$TARGET.cpp" ]; then
            echo -e "${RED}Error: Target '$TARGET' not found${NC}"
            echo "Available targets:"
            find src/apps -name "*.cpp" -exec basename {} .cpp \; | sort
            return 1
        fi
        cmake --build "$BUILD_DIR" --target "$TARGET" --parallel
    fi

    ln -sf "$BUILD_DIR/compile_commands.json" compile_commands.json
}

# Build and run tests
test() {
    debug
    echo "Running tests..."
    cd "$BUILD_DIR"
    ctest --output-on-failure
    cd ..
}

# Format code (project files only)
format() {
    if ! command_exists clang-format; then
        echo -e "${RED}clang-format not found${NC}"
        exit 1
    fi

    local TARGET="${1:-all}"
    local FILES=()

    case "$TARGET" in
        "all")
            echo "Formatting all source files..."
            FILES=($(find src include tests -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) 2>/dev/null))
            ;;
        "src")
            echo "Formatting src/ files..."
            FILES=($(find src -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) 2>/dev/null))
            ;;
        "include")
            echo "Formatting include/ files..."
            FILES=($(find include -type f \( -name "*.h" -o -name "*.hpp" \) 2>/dev/null))
            ;;
        *)
            if [ -f "$TARGET" ]; then
                echo "Formatting $TARGET..."
                FILES=("$TARGET")
            elif [ -d "$TARGET" ]; then
                echo "Formatting files in $TARGET..."
                FILES=($(find "$TARGET" -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) 2>/dev/null))
            else
                echo -e "${RED}Error: File or directory not found: $TARGET${NC}"
                echo "Usage: dev format [all|src|include|<file>|<directory>]"
                return 1
            fi
            ;;
    esac

    if [ ${#FILES[@]} -eq 0 ]; then
        echo -e "${RED}No files found to format${NC}"
        return 1
    fi

    echo "Formatting ${#FILES[@]} file(s)..."
    clang-format -i "${FILES[@]}"
    echo -e "${GREEN}✓ Formatted ${#FILES[@]} file(s)${NC}"
}

# Lint with clang-tidy diagnostics via clangd
lint() {
    # Ensure compile_commands.json exists
    if [ ! -f "compile_commands.json" ]; then
        echo "compile_commands.json not found. Building first..."
        debug
    fi

    # Ensure Python dev tools are installed
    ensure_dev_tools

    # Use clangd-tidy from project venv
    local CLANGD_TIDY=".devtools/venv/bin/clangd-tidy"

    if [ ! -f "$CLANGD_TIDY" ]; then
        echo -e "${RED}clangd-tidy not found in dev tools venv${NC}"
        echo "This shouldn't happen - try running: ./scripts/dev.sh install"
        return 1
    fi

    # Find clangd from VS Code/Cursor extension
    local CLANGD=$(find ~/.vscode-server ~/.cursor-server -name "clangd" -type f -path "*/bin/clangd" 2>/dev/null | head -1)
    if [ -z "$CLANGD" ]; then
        echo -e "${RED}clangd not found${NC}"
        echo "Install the clangd extension in VS Code/Cursor"
        return 1
    fi

    local TARGET="${1:-all}"
    local FILES=()

    case "$TARGET" in
        "all")
            echo "Linting all source, header, and test files..."
            FILES=($(find src include tests -name "*.cpp" -o -name "*.hpp" | grep -v "build/"))
            ;;
        "src")
            echo "Linting source files..."
            FILES=($(find src -name "*.cpp" | grep -v "build/"))
            ;;
        "include")
            echo "Linting header files..."
            FILES=($(find include -name "*.hpp" | grep -v "build/"))
            ;;
        "tests")
            echo "Linting test files..."
            FILES=($(find tests -name "*.cpp" | grep -v "build/"))
            ;;
        *)
            if [ -f "$TARGET" ]; then
                echo "Linting $TARGET..."
                FILES=("$TARGET")
            else
                echo -e "${RED}Error: File not found: $TARGET${NC}"
                echo "Usage: dev lint [all|src|include|tests|<file>]"
                return 1
            fi
            ;;
    esac

    if [ ${#FILES[@]} -eq 0 ]; then
        echo -e "${RED}No source files found${NC}"
        return 1
    fi

    echo "Running clang-tidy on ${#FILES[@]} file(s) via clangd..."
    echo ""

    # Run clangd-tidy with -j 1 (sequential) to avoid file locks
    # Stream output directly (don't capture to variable - that causes hangs)
    # Use tee to both display and save for counting
    local TMP_OUTPUT=$(mktemp)
    "$CLANGD_TIDY" --clangd-executable="$CLANGD" -j 1 "${FILES[@]}" < /dev/null 2>&1 | tee "$TMP_OUTPUT"
    local EXIT_CODE=${PIPESTATUS[0]}

    # Count diagnostics from saved output
    local HINT_COUNT=$(grep -c "Hint:" "$TMP_OUTPUT" 2>/dev/null || true)
    local INFO_COUNT=$(grep -c "Information:" "$TMP_OUTPUT" 2>/dev/null || true)
    local WARN_COUNT=$(grep -c "Warning:" "$TMP_OUTPUT" 2>/dev/null || true)
    local ERROR_COUNT=$(grep -c "Error:" "$TMP_OUTPUT" 2>/dev/null || true)

    # Ensure counts are numbers (grep -c returns 0 when no matches)
    HINT_COUNT=${HINT_COUNT:-0}
    INFO_COUNT=${INFO_COUNT:-0}
    WARN_COUNT=${WARN_COUNT:-0}
    ERROR_COUNT=${ERROR_COUNT:-0}

    local TOTAL_ISSUES=$((HINT_COUNT + INFO_COUNT + WARN_COUNT + ERROR_COUNT))

    rm -f "$TMP_OUTPUT"

    echo ""
    echo "=========================================="
    echo "Lint Summary:"
    echo "  Files checked: ${#FILES[@]}"
    echo "  Hints: $HINT_COUNT"
    echo "  Info: $INFO_COUNT"
    echo "  Warnings: $WARN_COUNT"
    echo "  Errors: $ERROR_COUNT"
    echo "=========================================="
    echo ""

    if [ $ERROR_COUNT -gt 0 ]; then
        echo -e "${RED}✗ Found $ERROR_COUNT error(s)${NC}"
        return 1
    elif [ $TOTAL_ISSUES -gt 0 ]; then
        echo -e "\033[0;33m⚠ Found $TOTAL_ISSUES issue(s)${NC}"
        return 0
    else
        echo -e "${GREEN}✓ All files passed linting!${NC}"
        return 0
    fi
}

# Run the application
run() {
    local TARGET_NAME="$1"

    # Discover available app targets from src/apps/*.cpp
    local AVAILABLE_TARGETS=()
    while IFS= read -r f; do
        # Extract basename without extension
        local base
        base="$(basename "$f" .cpp)"
        AVAILABLE_TARGETS+=("$base")
    done < <(find src/apps -maxdepth 1 -type f -name '*.cpp' 2>/dev/null | sort)

    # Handle case with no apps
    if [ ${#AVAILABLE_TARGETS[@]} -eq 0 ]; then
        echo -e "${RED}No applications found in src/apps${NC}"
        exit 1
    fi

    # If no target provided and only one app exists, default to it
    if [ -z "$TARGET_NAME" ]; then
        if [ ${#AVAILABLE_TARGETS[@]} -eq 1 ]; then
            TARGET_NAME="${AVAILABLE_TARGETS[0]}"
            echo "No target provided. Defaulting to '${TARGET_NAME}' (only app found in src/apps)."
        else
            echo -e "${RED}No target provided${NC}"
            echo "Usage: dev run <target>"
            echo "Available targets from src/apps:"
            for t in "${AVAILABLE_TARGETS[@]}"; do
                echo "  - $t"
            done
            exit 1
        fi
    fi

    # Validate provided target against discovered apps
    local valid=false
    for t in "${AVAILABLE_TARGETS[@]}"; do
        if [ "$t" = "$TARGET_NAME" ]; then
            valid=true
            break
        fi
    done

    if [ "$valid" != true ]; then
        echo -e "${RED}Invalid target: $TARGET_NAME${NC}"
        echo "Available targets from src/apps:"
        for t in "${AVAILABLE_TARGETS[@]}"; do
            echo "  - $t"
        done
        exit 1
    fi

    # Run the built executable if present; otherwise, build it (Debug) and then run
    if [ ! -f "$BUILD_DIR/$TARGET_NAME" ]; then
        echo "Executable '$BUILD_DIR/$TARGET_NAME' not found. Building target '$TARGET_NAME' (Debug)..."
        mkdir -p "$BUILD_DIR"
        cmake -S . -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
        cmake --build "$BUILD_DIR" --target "$TARGET_NAME" --parallel
    fi

    if [ -f "$BUILD_DIR/$TARGET_NAME" ]; then
        "$BUILD_DIR/$TARGET_NAME"
    else
        echo -e "${RED}Failed to build or locate '$TARGET_NAME' in '$BUILD_DIR'.${NC}"
        exit 1
    fi
}

# Run full pipeline
all() {
    clean
    release
    test
    lint
    format
    echo -e "${GREEN}All tasks completed!${NC}"
}



# Comment out the selected lines in settings.json (dev config)
config() {
    echo "Commenting out files.exclude section in .vscode/settings.json..."

    # Find the start line of files.exclude section
    START_LINE=$(grep -n '"files.exclude": {' .vscode/settings.json | cut -d: -f1)

    if [ -z "$START_LINE" ]; then
        echo -e "${RED}Could not find files.exclude section in settings.json${NC}"
        exit 1
    fi

    # Find the closing brace for files.exclude (next } after the opening)
    END_LINE=$(awk -v start="$START_LINE" 'NR > start && /^[[:space:]]*}/ { print NR; exit }' .vscode/settings.json)

    if [ -z "$END_LINE" ]; then
        echo -e "${RED}Could not find closing brace for files.exclude section${NC}"
        exit 1
    fi

    # Comment out the files.exclude content (lines after the opening brace) if not already commented
    sed -i "$((START_LINE+1)),$((END_LINE-1)){/^[[:space:]]*\/\/ /!s/^\([[:space:]]*\)/\1\/\/ /;}" .vscode/settings.json

    echo -e "${GREEN}Configuration mode activated - build files are now visible in VS Code${NC}"
}

# Uncomment the selected lines in settings.json (dev focus)
focus() {
    echo "Uncommenting files.exclude section in .vscode/settings.json..."

    # Find the start line of files.exclude section
    START_LINE=$(grep -n '"files.exclude": {' .vscode/settings.json | cut -d: -f1)

    if [ -z "$START_LINE" ]; then
        echo -e "${RED}Could not find files.exclude section in settings.json${NC}"
        exit 1
    fi

    # Find the closing brace for files.exclude (next } after the opening)
    END_LINE=$(awk -v start="$START_LINE" 'NR > start && /^[[:space:]]*}/ { print NR; exit }' .vscode/settings.json)

    if [ -z "$END_LINE" ]; then
        echo -e "${RED}Could not find closing brace for files.exclude section${NC}"
        exit 1
    fi

    # Remove comments from the files.exclude content (lines after the opening brace)
    sed -i "$((START_LINE+1)),$((END_LINE-1))s/^\([[:space:]]*\)\/\/ /\1/" .vscode/settings.json

    echo -e "${GREEN}Focus mode activated - build files are now hidden in VS Code${NC}"
}

# Show usage
usage() {
    echo "Usage: dev {command} [options]"
    echo ""
    echo "Build Commands:"
    echo "  debug [target]    - Build debug version (target must match a file in src/apps, defaults to all)"
    echo "  release [target]  - Build release version (target must match a file in src/apps, defaults to all)"
    echo "  clean             - Clean build directory"
    echo "  test              - Build and run tests"
    echo "  all               - Run full pipeline (clean, release, test)"
    echo ""
    echo "Code Quality:"
    echo "  format [target]   - Format code (all|src|include|<file>|<dir>, defaults to all)"
    echo "  lint [target]     - Run clang-tidy linter (all|src|include|tests|<file>, defaults to all)"
    echo ""
    echo "Run & Dev:"
    echo "  run [target]      - Run an app from src/apps (defaults to the only app if there's just one)"
    echo ""
    echo "VS Code:"
    echo "  focus             - Hide config and build files in VS Code"
    echo "  config            - Show config and build files in VS Code"
    echo ""
    echo "Examples:"
    echo "  dev debug demo2d           # Build only demo2d in debug mode"
    echo "  dev release                # Build all targets in release mode"
    echo "  dev format src/apps        # Format only files in src/apps/"
    echo "  dev lint src/apps/demo.cpp # Lint specific file"
    echo "  dev lint tests             # Lint all test files"
}

case $1 in
    "debug") debug "$2" ;;
    "release") release "$2" ;;
    "clean") clean ;;
    "test") test ;;
    "format") format "$2" ;;
    "lint") lint "$2" ;;
    "run") run "$2" ;;
    "all") all ;;
    "config") config ;;
    "focus") focus ;;
    *) usage;;
esac
