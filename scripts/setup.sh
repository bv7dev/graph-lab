#!/bin/bash

# Setup script to create 'dev' function for the project
# This makes it so you can type 'dev install' instead of './scripts/dev.sh install'

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/devtools.sh"

# Install dependencies for Ubuntu 24.04
echo "Installing dependencies for Ubuntu 24.04..."

# Update package lists
sudo apt update

# Add PPA for g++-14
echo "Adding PPA for g++-14..."
sudo apt install -y software-properties-common
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test

# Update package lists again after adding PPA
sudo apt update

# Install build tools and zsh
echo "Installing build tools..."
sudo apt install -y cmake ninja-build build-essential zsh

# Install clang-format for code formatting
echo "Installing clang-format..."
sudo apt install -y clang-format

# Install g++-14
echo "Installing g++-14..."
sudo apt install -y g++-14

# Install gdb for debugging
echo "Installing gdb for debugging..."
sudo apt install -y gdb

# Install GLFW dependencies
echo "Installing GLFW and OpenGL dependencies..."
sudo apt install -y libwayland-dev wayland-protocols libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev pkgconf libxkbcommon-dev

# Install Python for dev tools
echo "Installing Python..."
sudo apt install -y python3 python3-pip python3-venv

# Set up project-local Python dev tools
echo "Setting up Python dev tools..."
ensure_dev_tools

# Create a function that works from project root
FUNCTION_CMD="dev() { "$SCRIPT_DIR/dev.sh" \"\$@\"; }"
SHELL_RC="$HOME/.zshrc"

# Create a temporary file with the updated content
TEMP_RC=$(mktemp)

# Process the existing .zshrc file
if [ -f "$SHELL_RC" ]; then
    # Remove any existing dev function and project-specific comments
    # Also remove any trailing blank lines to prevent accumulation
    awk '
    /^# Project-specific dev function/ { skip=1; next }
    /^dev\(\) \{/ { skip=1; next }
    skip && /^}$/ { skip=0; next }
    !skip { print }
    ' "$SHELL_RC" > "$TEMP_RC"

    # Remove trailing blank lines
    while [ -s "$TEMP_RC" ] && [ "$(tail -c1 "$TEMP_RC")" = "" ]; do
        head -c -1 "$TEMP_RC" > "${TEMP_RC}.tmp" && mv "${TEMP_RC}.tmp" "$TEMP_RC"
    done
    echo "" >> "$TEMP_RC"
else
    touch "$TEMP_RC"
fi

# Add the new dev function (comment and function together)
echo "" >> "$TEMP_RC"
echo "# Project-specific dev function" >> "$TEMP_RC"
echo "$FUNCTION_CMD" >> "$TEMP_RC"

# Replace the original file
mv "$TEMP_RC" "$SHELL_RC"

echo "Updated 'dev' function in $SHELL_RC"

echo "Setup complete!"
echo ""

# Print dev help
zsh -c "$SCRIPT_DIR/dev.sh help"
