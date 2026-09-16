#!/usr/bin/env bash
set -euo pipefail

export DEVKITPRO="${DEVKITPRO:-/opt/devkitpro}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_JOBS="${BUILD_JOBS:-$(nproc)}"
SWITCH_BUILD_DIR="${SWITCH_BUILD_DIR:-build_switch}"
RELEASE_VERSION="${RELEASE_VERSION:-1.2.0}"
case "${BUILD_JOBS}" in
	''|*[!0-9]*|0) echo "BUILD_JOBS must be a positive integer" >&2; exit 2 ;;
esac
if [[ ! "$RELEASE_VERSION" =~ ^([0-9]+)\.([0-9]+)\.([0-9]+)$ ]]; then
	echo "RELEASE_VERSION must use major.minor.patch format" >&2
	exit 2
fi
VERSION_MAJOR="${BASH_REMATCH[1]}"
VERSION_MINOR="${BASH_REMATCH[2]}"
VERSION_PATCH="${BASH_REMATCH[3]}"
cd "${ROOT}"

bash "${ROOT}/dist/switch/deps/prepare_submodules.sh"

# Mesa/NVK SDK: the newest dependencies/switch_mesa_sdk_* unless overridden. A
# stale root links an NVK without VK_EXT_external_memory_host, which then fails
# to boot on a 39-bit address space.
if [[ -z "${SWITCH_MESA_SDK_ROOT:-}" ]]; then
	# Newest by modification time, not by name: a glob sorts alphabetically, so
	# a descriptive directory outranks every date-stamped one and the build
	# silently links whichever driver happens to sort last.
	# Only fully packaged SDKs count. An interrupted or hand-made install has no
	# revision file, and picking one links a driver nothing else agrees with -
	# the shader cache it writes then fails to deserialize against a real build.
	newest_mtime=0
	for candidate in "${ROOT}"/dependencies/switch_mesa_sdk_*/opt/devkitpro/portlibs/switch; do
		[[ -d "${candidate}" ]] || continue
		[[ -f "${candidate}/share/mesa-switch/revision" ]] || continue
		[[ -f "${candidate}/lib/libnvk.a" ]] || continue
		candidate_mtime="$(stat -c %Y "${candidate}" 2>/dev/null || echo 0)"
		if (( candidate_mtime >= newest_mtime )); then
			newest_mtime="${candidate_mtime}"
			SWITCH_MESA_SDK_ROOT="${candidate}"
		fi
	done
fi
if [[ -z "${SWITCH_MESA_SDK_ROOT:-}" || ! -f "${SWITCH_MESA_SDK_ROOT}/lib/libnvk.a" ]]; then
	echo "No Mesa SDK found. Extract one into dependencies/switch_mesa_sdk_<date>/," >&2
	echo "or point SWITCH_MESA_SDK_ROOT at a portlibs/switch directory." >&2
	exit 2
fi
export SWITCH_MESA_SDK_ROOT
MESA_REVISION="$(tr -d '\r\n' < "${SWITCH_MESA_SDK_ROOT}/share/mesa-switch/revision" 2>/dev/null || true)"
echo ">> Mesa SDK: ${SWITCH_MESA_SDK_ROOT}${MESA_REVISION:+ (${MESA_REVISION})}"

# Objects built against another driver survive a reconfigure and can win the
# link, leaving two Mesa builds in one NRO.
CACHED_MESA_SDK_ROOT=""
if [[ -f "${SWITCH_BUILD_DIR}/CMakeCache.txt" ]]; then
	CACHED_MESA_SDK_ROOT="$(sed -n 's/^SWITCH_MESA_SDK_ROOT:[^=]*=//p' \
		"${SWITCH_BUILD_DIR}/CMakeCache.txt" | head -1)"
fi
if [[ -n "${CACHED_MESA_SDK_ROOT}" && "${CACHED_MESA_SDK_ROOT}" != "${SWITCH_MESA_SDK_ROOT}" ]]; then
	echo ">> Mesa SDK changed from ${CACHED_MESA_SDK_ROOT}; rebuilding from scratch"
	rm -rf "${SWITCH_BUILD_DIR}"
fi

echo ">> Localizing NVK driver symbols ..."
bash "${ROOT}/dist/switch/deps/prepare_nvk.sh"

cmake -S . -B "${SWITCH_BUILD_DIR}" -G Ninja \
	-DCMAKE_TOOLCHAIN_FILE="${ROOT}/cmake/Toolchain-Switch.cmake" \
	-DCMAKE_BUILD_TYPE=Release \
	-DEMULATOR_VERSION_MAJOR="${VERSION_MAJOR}" \
	-DEMULATOR_VERSION_MINOR="${VERSION_MINOR}" \
	-DEMULATOR_VERSION_PATCH="${VERSION_PATCH}" \
	-DENABLE_OPENGL=ON \
	-DENABLE_VULKAN=ON \
	-DSWITCH_MESA_SDK_ROOT="${SWITCH_MESA_SDK_ROOT}" \
	-DSWITCH_LTO_JOBS="${BUILD_JOBS}"

cmake --build "${SWITCH_BUILD_DIR}" --parallel "${BUILD_JOBS}" --target CemuNro

echo ">> Output: ${ROOT}/bin/cemu_core.nro"
