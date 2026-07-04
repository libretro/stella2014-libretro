#!/bin/sh
# Compile-configuration guard.
#
# The desktop Makefile always defines THUMB_SUPPORT, but the libretro
# platform makefiles (Android.mk and the console targets) do NOT -- on
# those builds the Thumbulator type is compiled out, and the ARM-cartridge
# classes must still compile (detect/construct only, ARM execution elided),
# exactly as CartDPCPlus always has. That no-THUMB_SUPPORT configuration is
# never exercised by the desktop build or the runtime tests, so a header
# that references Thumbulator unguarded breaks only at the platform
# buildbot. This script compiles the emucore translation units in BOTH
# configurations so that break is caught locally.
#
# It is compile-only (-fsyntax-only): it checks that every unit builds
# warning- and error-free, not that the result runs (the runtime tests
# cover the THUMB_SUPPORT build separately).
#
# Usage: test/build_configs.sh
# Exit 0 if both configurations compile clean, non-zero otherwise.

set -e
cd "$(dirname "$0")/.."

CXX="${CXX:-g++}"
STD="-std=c++98"
WARN="-Wall -Wextra -Wno-unused-parameter"

INC="-I. -Istella -Istella/src -Istella/stubs -Istella/src/emucore \
     -Istella/src/common -Istella/src/gui -Ilibretro-common/include \
     -D__LIBRETRO__"

# The emucore sources are where the ARM-cart / Thumbulator coupling lives,
# and where cross-file base-class breakage shows up. Compiling the whole
# directory in each config catches both the direct "unknown type" error and
# the cascade into subclasses that do not derive from Cartridge any more.
SRCS=$(ls stella/src/emucore/*.cxx)

check_config()
{
    label="$1"; extra="$2"
    printf 'compiling emucore (%s)... ' "$label"
    fails=0
    for f in $SRCS; do
        if ! $CXX $STD $WARN $INC $extra -fsyntax-only "$f" 2>/tmp/bc_err; then
            [ "$fails" -eq 0 ] && echo "FAILED"
            echo "  --- $f ---"
            grep -E 'error:|warning:' /tmp/bc_err | head -5
            fails=$((fails + 1))
        elif [ -s /tmp/bc_err ] && grep -q 'warning:' /tmp/bc_err; then
            [ "$fails" -eq 0 ] && echo "FAILED"
            echo "  --- $f (warning) ---"
            grep -E 'warning:' /tmp/bc_err | head -5
            fails=$((fails + 1))
        fi
    done
    if [ "$fails" -eq 0 ]; then
        echo "OK ($(echo "$SRCS" | wc -w) units)"
        return 0
    fi
    echo "  $fails unit(s) failed in the $label configuration"
    return 1
}

rc=0
check_config "with THUMB_SUPPORT"    "-DTHUMB_SUPPORT" || rc=1
check_config "without THUMB_SUPPORT"  ""               || rc=1

if [ "$rc" -eq 0 ]; then
    echo "build configs: both configurations compile clean"
fi
exit "$rc"
