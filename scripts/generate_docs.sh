#!/bin/bash
set -e

# ---------------------------
# Determine repository root
# ---------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
cd "$REPO_ROOT"

# ---------------------------
# Variables and configuration
# ---------------------------
THEME_REPO="https://github.com/uniot-io/uniot-doxygen-css.git"
THEME_DIR="$REPO_ROOT/doxygen"

# ---------------------------
# Clone or update the theme repository
# ---------------------------
if [ -d "$THEME_DIR" ]; then
  echo "Theme directory exists. Pulling latest changes..."
  cd "$THEME_DIR"
  git pull
  cd "$REPO_ROOT"
else
  echo "Cloning theme repository..."
  git clone "$THEME_REPO" "$THEME_DIR"
fi

# ---------------------------
# Extract version from library.json
# ---------------------------
if [ ! -f "$REPO_ROOT/library.json" ]; then
  echo "Error: library.json not found in the repository root." >&2
  exit 1
fi

VERSION=$(jq -r '.version' "$REPO_ROOT/library.json")
echo "Extracted version: $VERSION"

# ---------------------------
# Update the Doxyfile's PROJECT_NUMBER with the extracted version
# ---------------------------
# Update the Doxyfile's PROJECT_NUMBER with the extracted version
Doxyfile_PATH="$THEME_DIR/Doxyfile"
if [ ! -f "$Doxyfile_PATH" ]; then
  echo "Error: Doxyfile not found in $THEME_DIR." >&2
  exit 1
fi

echo "Updating Doxyfile with version..."
if [[ "$OSTYPE" == "darwin"* ]]; then
  sed -i '' "s/^PROJECT_NUMBER.*/PROJECT_NUMBER = $VERSION/" "$Doxyfile_PATH"
else
  sed -i "s/^PROJECT_NUMBER.*/PROJECT_NUMBER = $VERSION/" "$Doxyfile_PATH"
fi

# ---------------------------
# Run Doxygen
# ---------------------------
echo "Running Doxygen..."
# Open theme directory (all following commands are executed in this directory)
cd "$THEME_DIR"
doxygen Doxyfile

# ---------------------------
# Organize versioned documentation
# ---------------------------
echo "Organizing versioned documentation..."
# Assumes that Doxygen output is in $THEME_DIR/docs/html
mkdir -p docs/latest docs/"$VERSION"

# Copy generated HTML docs to "latest" and the version-specific folder
cp -R docs/html/* docs/latest/
cp -R docs/html/* docs/"$VERSION"/

# Remove the original docs/html folder as it's no longer needed
rm -rf docs/html

# ---------------------------
# Generate versions.json from git tags
# ---------------------------
echo "Generating versions.json from git tags..."
cd "$REPO_ROOT"
# Get all tags sorted in descending order (assumes tags are version numbers)
TAGS=$(git tag --sort=-version:refname)
cd "$THEME_DIR"
# Use jq to generate a JSON array (drop the trailing empty element)
JSON=$(echo "$TAGS" | jq -R -s -c 'split("\n")[:-1]')
echo "{\"versions\": $JSON}" > docs/versions.json
echo "Generated docs/versions.json:"
cat docs/versions.json

echo "Documentation generated successfully in: $THEME_DIR/docs"
