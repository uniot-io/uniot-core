#!/bin/bash
#
# The version is stated in two places that nothing keeps in agreement: UNIOT_CORE_VERSION in
# lib/Core/Common.h, which is what a device reports in its status packet, and "version" in
# library.json, which is what a consumer resolves a dependency against. A release tag is a
# third statement of the same thing.
#
# Neither file can be generated from the other: library.json has to be a literal file for
# PlatformIO to read, and Common.h is compiled directly by every consumer, so making it
# generated would push a codegen step into their builds. Checking is cheaper.
#
#   ./scripts/check_version.sh          the two files agree
#   ./scripts/check_version.sh 0.9.0    ... and both match this (the release tag)

set -u

HEADER="lib/Core/Common.h"
MANIFEST="library.json"
FAILED=0

fail() {
  echo -n -e '\e[1;31m[ERROR]\e[0m '
  echo "$1"
  FAILED=1
}

header_version=$(sed -nE 's/^#define UNIOT_CORE_VERSION UNIOT_SEMVER_TO_INT\(([0-9]+), *([0-9]+), *([0-9]+)\).*/\1.\2.\3/p' "$HEADER")
manifest_version=$(sed -nE 's/.*"version"[[:space:]]*:[[:space:]]*"([0-9]+\.[0-9]+\.[0-9]+)".*/\1/p' "$MANIFEST" | head -1)

[ -n "$header_version" ]   || fail "no UNIOT_CORE_VERSION found in $HEADER"
[ -n "$manifest_version" ] || fail "no version found in $MANIFEST"

if [ -n "$header_version" ] && [ -n "$manifest_version" ] && [ "$header_version" != "$manifest_version" ]; then
  fail "$HEADER says $header_version but $MANIFEST says $manifest_version"
fi

if [ $# -ge 1 ]; then
  expected="${1#v}"
  [ "$header_version" = "$expected" ]   || fail "$HEADER says $header_version, expected $expected"
  [ "$manifest_version" = "$expected" ] || fail "$MANIFEST says $manifest_version, expected $expected"
fi

if [ $FAILED -ne 0 ]; then
  exit 1
fi

echo "version $manifest_version"
