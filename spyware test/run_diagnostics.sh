#!/usr/bin/env bash
set -euo pipefail

folder="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
program="$folder/memory_probe"

g++ -std=c++17 -Wall -Wextra -Wpedantic "$folder/memory_probe.cpp" -o "$program"
exec "$program" "${1:-64}" "${2:-2}" "${3:-5}"
