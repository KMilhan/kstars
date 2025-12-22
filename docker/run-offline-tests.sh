#!/usr/bin/env bash
set -euo pipefail

SRC_DIR=${1:-/work}
BUILD_DIR=${BUILD_DIR:-/work/build}

if [[ ! -d "$SRC_DIR" ]]; then
  echo "Source directory not found: $SRC_DIR" >&2
  exit 1
fi

export QT_QPA_PLATFORM=${QT_QPA_PLATFORM:-offscreen}
export QT_QUICK_BACKEND=${QT_QUICK_BACKEND:-software}
export LIBGL_ALWAYS_SOFTWARE=${LIBGL_ALWAYS_SOFTWARE:-1}
if [[ "${KSTARS_BUILD_WITH_QT6:-OFF}" == "ON" ]]; then
  export QT_QPA_PLATFORMTHEME=${QT_QPA_PLATFORMTHEME:-qt6ct}
  unset QT_PLUGIN_PATH
else
  export QT_QPA_PLATFORMTHEME=${QT_QPA_PLATFORMTHEME:-qt5ct}
  export QT_PLUGIN_PATH=${QT_PLUGIN_PATH:-/usr/lib/x86_64-linux-gnu/qt5/plugins}
fi

export HOME=${HOME:-/root}
export XDG_RUNTIME_DIR=${XDG_RUNTIME_DIR:-/tmp/runtime-kstars}
export XDG_DATA_HOME=${XDG_DATA_HOME:-/tmp/kstars-xdg/data}
export XDG_CONFIG_HOME=${XDG_CONFIG_HOME:-/tmp/kstars-xdg/config}
export XDG_CACHE_HOME=${XDG_CACHE_HOME:-/tmp/kstars-xdg/cache}
mkdir -p "$XDG_RUNTIME_DIR" "$XDG_DATA_HOME" "$XDG_CONFIG_HOME" "$XDG_CACHE_HOME"
chmod 700 "$XDG_RUNTIME_DIR"

export KSTARS_TEST_DATADIR=${KSTARS_TEST_DATADIR:-$SRC_DIR/kstars/data}
if [[ -z "${XDG_DATA_DIRS:-}" ]]; then
  export XDG_DATA_DIRS="$KSTARS_TEST_DATADIR:/usr/local/share:/usr/share"
elif [[ ":${XDG_DATA_DIRS}:" != *":${KSTARS_TEST_DATADIR}:"* ]]; then
  export XDG_DATA_DIRS="$KSTARS_TEST_DATADIR:$XDG_DATA_DIRS"
fi

if [[ -f "$BUILD_DIR/CMakeCache.txt" ]]; then
  cached_source=$(awk -F= '/^CMAKE_HOME_DIRECTORY:INTERNAL=/{print $2}' "$BUILD_DIR/CMakeCache.txt" | head -n1)
  if [[ -n "$cached_source" && "$cached_source" != "$SRC_DIR" ]]; then
    rm -rf "$BUILD_DIR"
  fi
fi
mkdir -p "$BUILD_DIR"

extra_cmake_args=()
if [[ -n "${KSTARS_CMAKE_ARGS:-}" ]]; then
  read -r -a extra_cmake_args <<< "${KSTARS_CMAKE_ARGS}"
fi

cmake -S "$SRC_DIR" -B "$BUILD_DIR" -G Ninja \
  -DBUILD_WITH_QT6="${KSTARS_BUILD_WITH_QT6:-OFF}" \
  -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-RelWithDebInfo} \
  -DCMAKE_INSTALL_PREFIX=/usr \
  "${extra_cmake_args[@]}"

cmake --build "$BUILD_DIR" --target "${KSTARS_BUILD_TARGET:-all}"

cd "$BUILD_DIR"

umask 077

CTEST_ARGS_DEFAULT="--output-on-failure"
ctest_args=()
if [[ -n "${KSTARS_CTEST_ARGS:-}" ]]; then
  read -r -a ctest_args <<< "${KSTARS_CTEST_ARGS}"
else
  read -r -a ctest_args <<< "${CTEST_ARGS_DEFAULT}"
fi

if command -v dbus-run-session >/dev/null 2>&1; then
  dbus-run-session -- ctest "${ctest_args[@]}"
else
  ctest "${ctest_args[@]}"
fi
