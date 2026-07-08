#!/usr/bin/env bash
# Installable-package smoke test (#84/#293). Installs the pinned tmech
# (and pegtl when the parser is on), installs NumSim_CAS, then builds and
# runs a throwaway downstream project that consumes it via
# find_package(NumSim_CAS). Exits non-zero on any failure.
#
# Usage: cmake/package_smoke_test.sh [ON|OFF]   # parser (default OFF)
set -euo pipefail

PARSER="${1:-OFF}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
PREFIX="$WORK/prefix"
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)"

echo "== smoke test (parser=$PARSER) : root=$ROOT =="

# 1. Configure once to populate the FetchContent deps (tmech, pegtl).
cmake -S "$ROOT" -B "$WORK/seed" \
  -DNUMSIM_CAS_BUILD_TESTS=OFF -DNUMSIM_CAS_BUILD_PARSER="$PARSER" >/dev/null

# 2. Install the pinned tmech from the fetched source (tests/examples off
#    so it doesn't drag in googletest at install time).
cmake -S "$WORK/seed/_deps/tmech-src" -B "$WORK/tmech" \
  -DCMAKE_INSTALL_PREFIX="$PREFIX" \
  -DTMECH_INSTALL_LIBRARY=ON -DTMECH_BUILD_TESTS=OFF \
  -DTMECH_BUILD_EXAMPLES=OFF >/dev/null
cmake --install "$WORK/tmech" >/dev/null

# 3. Parser needs pegtl findable by the consumer too.
if [ "$PARSER" = "ON" ]; then
  cmake -S "$WORK/seed/_deps/pegtl-src" -B "$WORK/pegtl" \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DPEGTL_BUILD_TESTS=OFF -DPEGTL_BUILD_EXAMPLES=OFF >/dev/null
  cmake --install "$WORK/pegtl" >/dev/null
fi

# 4. Configure + build + install NumSim_CAS with the export rules on.
cmake -S "$ROOT" -B "$WORK/ncas" \
  -DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_PREFIX_PATH="$PREFIX" \
  -DNUMSIM_CAS_INSTALL_LIBRARY=ON -DNUMSIM_CAS_ENABLE_PACKAGE_SMOKE_TEST=ON \
  -DNUMSIM_CAS_BUILD_PARSER="$PARSER" -DNUMSIM_CAS_BUILD_TESTS=OFF >/dev/null
cmake --build "$WORK/ncas" --parallel "$JOBS" >/dev/null
cmake --install "$WORK/ncas" >/dev/null

# 5. Downstream consumer: only knows the install prefix.
CONS="$WORK/consumer"
mkdir -p "$CONS"
if [ "$PARSER" = "ON" ]; then
  LINK_TGT="NumSim_CAS::NumSim_CAS_Parser"
  cat > "$CONS/main.cpp" <<'EOF'
#include <numsim_cas/numsim_cas.h>
#include <numsim_cas/parser/parser.h>
#include <cstdio>
int main() {
  using namespace numsim::cas;
  parser::symbol_table syms;
  auto e = parser::parse_scalar("sin(x)^2 + cos(x)^2", syms);
  std::printf("consumer OK (parser): %s\n", to_string(e).c_str());
  return 0;
}
EOF
else
  LINK_TGT="NumSim_CAS::NumSim_CAS"
  cat > "$CONS/main.cpp" <<'EOF'
#include <numsim_cas/numsim_cas.h>
#include <cstdio>
int main() {
  using namespace numsim::cas;
  auto x = make_expression<scalar>("x");
  auto d = diff(pow(x, 3), x);
  std::printf("consumer OK: %s\n", to_string(d).c_str());
  return 0;
}
EOF
fi
cat > "$CONS/CMakeLists.txt" <<EOF
cmake_minimum_required(VERSION 3.22)
project(ncas_smoke_consumer CXX)
set(CMAKE_CXX_STANDARD 23)
find_package(NumSim_CAS REQUIRED)
add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE $LINK_TGT)
EOF

cmake -S "$CONS" -B "$WORK/consumer-build" -DCMAKE_PREFIX_PATH="$PREFIX" >/dev/null
cmake --build "$WORK/consumer-build" --parallel "$JOBS" >/dev/null
"$WORK/consumer-build/consumer"

echo "== packaging smoke test (parser=$PARSER) PASSED =="
