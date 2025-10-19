#!/usr/bin/env zsh

# Setup script to create 'dev' function for the project
# This makes it so you can type 'dev install' instead of './scripts/dev.sh install'

set -e

echo "Setting up 'dev' function for this project..."

# Get the absolute path to the project root
SCRIPT_DIR="$(cd "$(dirname "${(%):-%x}")" && pwd)"
DEV_SCRIPT="$SCRIPT_DIR/dev.sh"

# Create a function that works from project root
FUNCTION_CMD="dev() { $DEV_SCRIPT \"\$@\"; }"
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
