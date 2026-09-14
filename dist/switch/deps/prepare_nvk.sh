#!/usr/bin/env bash
set -euo pipefail
source "$(cd "$(dirname "$0")" && pwd)/common.sh"
parse_check_only "$@"

NVK_DIR="${SWITCH_MESA_SDK_ROOT:-${ROOT}/dependencies/switch_mesa_vulkan}"
CHECKSUMS="${NVK_DIR}/share/nvk-switch/SHA256SUMS"
LOCAL_OBJECT="${NVK_DIR}/libnvk_local.o"
LOCAL_ARCHIVE="${NVK_DIR}/libnvk_local.a"

[[ -d "${NVK_DIR}/lib" ]] || die "NVK directory is missing: ${NVK_DIR}/lib"
if [[ ! -f "${CHECKSUMS}" && ! -f "${NVK_DIR}/lib/pkgconfig/vulkan.pc" ]]; then
	die "Mesa SDK metadata is missing under ${NVK_DIR}"
fi

if (( CHECK_ONLY )); then
	if [[ -f "${CHECKSUMS}" ]]; then
		(
			cd "${NVK_DIR}"
			tr -d '\r' < "${CHECKSUMS}" | sha256sum --check --strict -
		)
	else
		mesa_version="$(sed -n 's/^Version:[[:space:]]*//p' \
			"${NVK_DIR}/lib/pkgconfig/vulkan.pc" | head -n 1)"
		[[ -n "${mesa_version}" ]] || die "Mesa SDK version is missing"
		[[ "$(printf '%s\n' 26.2.0 "${mesa_version}" | sort -V | head -n 1)" == 26.2.0 ]] ||
			die "Mesa SDK 26.2.0 or newer is required"
	fi
	[[ -f "${LOCAL_OBJECT}" ]] || die "localized NVK object is missing; run dist/switch/deps/prepare_nvk.sh"
	[[ -f "${LOCAL_ARCHIVE}" ]] || die "localized NVK archive is missing; run dist/switch/deps/prepare_nvk.sh"
	echo "Ready: NVK driver"
	exit 0
fi

bash "${ROOT}/dist/switch/localize_nvk.sh" "${NVK_DIR}"
