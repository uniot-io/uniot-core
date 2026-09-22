#!/bin/bash
#
# Fail unless the authenticated PlatformIO account may publish to a package.
#
# pio pkg publish falls back to the token's own account when --owner is omitted, which is how
# a release lands in a personal namespace instead of the organisation's. This answers "may
# this account write to <owner>/<package>?" without uploading anything.
#
#   ./scripts/check_registry_access.sh uniot-io/uniot-core

set -u

TARGET="${1:-}"

if [ -z "$TARGET" ] || [ "${TARGET#*/}" = "$TARGET" ]; then
  echo "usage: $0 <owner>/<package>" >&2
  exit 2
fi

owner="${TARGET%%/*}"
name="${TARGET##*/}"

if ! command -v pio >/dev/null 2>&1; then
  echo "pio is not on PATH" >&2
  exit 2
fi

# Distinguish "no access" from "could not ask": an unauthenticated CLI or a network failure
# must not look like a permission answer.
if ! access=$(pio access list 2>&1); then
  echo -n -e '\e[1;31m[ERROR]\e[0m ' >&2
  echo "could not read package access:" >&2
  echo "$access" >&2
  exit 2
fi

level=$(printf '%s\n' "$access" | awk -v want_name="$name" -v want_owner="$owner" '
  /^[A-Za-z0-9._][A-Za-z0-9._-]*$/ { pkg = $1 }   # a name line, not the ---- underline
  /^Owner:/                  { pkg_owner = $2 }
  /^Access level\(s\):/      { if (pkg == want_name && pkg_owner == want_owner) print $3 }
')

if [ -z "$level" ]; then
  echo -n -e '\e[1;31m[ERROR]\e[0m '
  echo "this account has no access to $TARGET; publishing would fall back to its own namespace"
  echo "accessible packages:" >&2
  printf '%s\n' "$access" | grep -E "^(Owner:|[A-Za-z0-9._-]+$)" >&2
  exit 1
fi

case "$level" in
  Admin|Maintainer)
    echo "$TARGET: $level"
    ;;
  *)
    echo -n -e '\e[1;31m[ERROR]\e[0m '
    echo "access to $TARGET is '$level', which cannot publish"
    exit 1
    ;;
esac
