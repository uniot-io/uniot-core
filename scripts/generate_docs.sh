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

# Filter versions to include only 0.8.1 and above
filter_versions() {
  local min_version="0.8.1"

  while IFS= read -r tag; do
    # Skip empty lines
    [[ -z "$tag" ]] && continue

    # Remove 'v' prefix if present for comparison
    version=${tag#v}

    # Split version into parts (major.minor.patch)
    IFS='.' read -ra VERSION_PARTS <<< "$version"
    IFS='.' read -ra MIN_PARTS <<< "$min_version"

    # Ensure we have at least 3 parts, pad with zeros if needed
    while [ ${#VERSION_PARTS[@]} -lt 3 ]; do
      VERSION_PARTS+=("0")
    done
    while [ ${#MIN_PARTS[@]} -lt 3 ]; do
      MIN_PARTS+=("0")
    done

    # Compare version numbers
    major=${VERSION_PARTS[0]}
    minor=${VERSION_PARTS[1]}
    patch=${VERSION_PARTS[2]}
    min_major=${MIN_PARTS[0]}
    min_minor=${MIN_PARTS[1]}
    min_patch=${MIN_PARTS[2]}

    # Check if version is >= 0.8.1
    if [[ "$major" -gt "$min_major" ]] || \
       [[ "$major" -eq "$min_major" && "$minor" -gt "$min_minor" ]] || \
       [[ "$major" -eq "$min_major" && "$minor" -eq "$min_minor" && "$patch" -ge "$min_patch" ]]; then
      echo "$tag"
    fi
  done
}

FILTERED_TAGS=$(echo "$TAGS" | filter_versions)
cd "$THEME_DIR"
# Use jq to generate a JSON array (drop the trailing empty element)
JSON=$(echo "$FILTERED_TAGS" | jq -R -s -c 'split("\n") | map(select(length > 0))')
echo "{\"versions\": $JSON}" > docs/versions.json
echo "Generated docs/versions.json:"
cat docs/versions.json

echo "Documentation generated successfully in: $THEME_DIR/docs"
