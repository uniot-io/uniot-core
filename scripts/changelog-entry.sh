#!/bin/bash
#
# Print the CHANGELOG section for one version, for use as a release body.
#
# Takes everything between "## <version>" and the next "## " heading. Exits 1 and prints
# nothing if there is no section for that version, so a caller can decide whether a
# missing entry should stop a release.
#
#   ./scripts/changelog-entry.sh 0.9.0
#   ./scripts/changelog-entry.sh 0.9.0 CHANGELOG.md

set -u

VERSION="${1:-}"
VERSION="${VERSION#v}"
CHANGELOG="${2:-CHANGELOG.md}"

if [ -z "$VERSION" ]; then
  echo "usage: $0 <version> [changelog]" >&2
  exit 2
fi

if [ ! -f "$CHANGELOG" ]; then
  echo "no such file: $CHANGELOG" >&2
  exit 2
fi

# -v rather than interpolating into the pattern: a version contains dots, which would
# otherwise match any character.
entry=$(awk -v want="## $VERSION" '
  $0 == want { found = 1; next }
  found && /^## / { exit }
  found { print }
' "$CHANGELOG")

# Trim blank lines from both ends.
entry=$(printf '%s\n' "$entry" | sed -e '/./,$!d' | tac | sed -e '/./,$!d' | tac)

if [ -z "$entry" ]; then
  exit 1
fi

printf '%s\n' "$entry"
