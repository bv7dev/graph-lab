# Ensure Python dev tools venv is set up
ensure_dev_tools() {
    local VENV_DIR=".devtools/venv"
    local REQUIREMENTS=".devtools/requirements.txt"

    # Check if venv exists and is valid
    if [ ! -f "$VENV_DIR/bin/python3" ]; then
        echo "Setting up Python dev tools environment..."
        python3 -m venv "$VENV_DIR"
        "$VENV_DIR/bin/pip" install --upgrade pip > /dev/null 2>&1
        "$VENV_DIR/bin/pip" install -r "$REQUIREMENTS"
        echo "✓ Python dev tools installed"
        echo ""
    fi

    # Check if requirements have changed (simple hash check)
    if [ -f "$VENV_DIR/.installed" ]; then
        local OLD_HASH=$(cat "$VENV_DIR/.installed" 2>/dev/null || echo "")
        local NEW_HASH=$(md5sum "$REQUIREMENTS" 2>/dev/null | cut -d' ' -f1 || echo "")
        if [ "$OLD_HASH" != "$NEW_HASH" ]; then
            echo "Dev tools requirements changed, updating..."
            "$VENV_DIR/bin/pip" install -r "$REQUIREMENTS" --upgrade
            echo "$NEW_HASH" > "$VENV_DIR/.installed"
            echo "✓ Dev tools updated"
            echo ""
        fi
    else
        md5sum "$REQUIREMENTS" 2>/dev/null | cut -d' ' -f1 > "$VENV_DIR/.installed" 2>/dev/null || true
    fi
}
