#!/usr/bin/env bash

# Copyright (c) 2025-2026, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# -----------------------------------------------------------------------------
# Arm Toolchain for Linux (ATfL) — Non-root installer
# -----------------------------------------------------------------------------
# Usage (one-liner):
#   bash <(curl -fsSL https://developer.arm.com/-/cdn-downloads/permalink/Arm-Toolchain-for-Linux/Package/user_install.sh) /path/to/install
#
# What this script does
# - Detects your Linux distribution (same logic as ACfL script)
# - Downloads the LATEST released ATfL + Arm Performance Libraries packages
# - Performs a NON-ROOT install by extracting packages into a user-writable dir
# - Rewrites modulefiles with the correct install_prefix
# - Re-targets libamath symlinks so ATfL uses the co-installed ArmPL
# - Prints post-install "module" instructions
#
# Notes
# - This script is **only** for non-root installs. For system-wide installs,
#   please use your native package manager (apt/dnf/zypper).
# - "Latest only": the script intentionally does NOT support installing
#   previous releases.
# - OS override is supported: you can download packages for a different OS.
#   In override mode, the script **only downloads** packages and points you to
#   documentation; it will not install.
#
# Environment overrides (optional)
#   ATFL_DISTRIBUTION_OVERRIDE  : set to one of the supported distro labels to
#                                 download for a different OS (download-only)
#   ATFL_REPO_BASE_URL          : full repo base URL override (http(s)://.../arm-toolchains/)
#   ATFL_ATFL_PKG_URL           : full URL to ATfL package (deb/rpm) to use
#   ATFL_ARMPL_PKG_URL          : full URL to ArmPL package (deb/rpm) to use
#   ATFL_NONINTERACTIVE         : set to 1 to avoid interactive prompts
#   ATFL_DEBUG                  : set to 1 to enable set -x and verbose extract
#
# -----------------------------------------------------------------------------
set -euo pipefail
IFS=$'\n\t'
[[ "${ATFL_DEBUG:-0}" == "1" ]] && set -x

# ============================== Config =======================================
SUPPORTED_DISTRIBUTIONS=(
  "AmazonLinux-2023"
  "RHEL-8"
  "RHEL-9"
  "RHEL-10"
  "SLES-15"
  "SLES-16"
  "Ubuntu-22.04"
  "Ubuntu-24.04"
  "Ubuntu-26.04"
)

DEFAULT_REPO_BASE_URL="https://developer.arm.com/packages/arm-toolchains/"

# The directory layout inside extracted packages (used for symlink + modulefile)
MODULEFILES_DIR="opt/arm/modulefiles"
ATFL_MODULE_SUBDIR="atfl"  # e.g., opt/arm/modulefiles/atfl/<version>
ATFL_DIR="opt/arm/arm-toolchain-for-linux"
ARMPL_DIR="opt/arm/arm-performance-libraries"

# ============================== Helpers ======================================
usage() {
  cat <<'USAGE'
Usage:
  user_install.sh [OPTIONS] /path/to/install
  user_install.sh --install-root DIR [OPTIONS]

Description:
  Non-root installer for Arm Toolchain for Linux (ATfL) + Arm Performance Libraries.

Options:
  -y, --yes               Non-interactive; also runs a self-test
      --self-test         Run post-install self-test
      --no-cleanup        Keep downloaded artifacts
      --debug             Verbose mode (sets ATFL_DEBUG=1)
      --install-root DIR  Install path (alternative to positional)
  -h, --help              Show this help and exit

Env:
  ATFL_DISTRIBUTION_OVERRIDE=<DISTRO>    Download-only for another OS
                                         (AmazonLinux-2023|RHEL-8|RHEL-9|RHEL-10|SLES-15|SLES-16|Ubuntu-22.04|Ubuntu-24.04|Ubuntu-26.04)
  ATFL_REPO_BASE_URL=http(s)://...       Override repo base URL
  ATFL_ATFL_PKG_URL, ATFL_ARMPL_PKG_URL  Pin exact package URLs (skip index lookup)
  ATFL_NONINTERACTIVE=1                  Suppress prompts (path still required)
  ATFL_DEBUG=1                           Shell trace + verbose extraction

Examples:
  user_install.sh "$HOME/tools/atfl" --yes
  ATFL_DISTRIBUTION_OVERRIDE=RHEL-9 user_install.sh "$HOME/downloads"
USAGE
}

log()  { echo "[ATfL] $*" >&2; }
err()  { echo "[ATfL][ERROR] $*" >&2; }

# Run a command safely (no eval). If not in debug, suppress output but re-run
# with output if it fails. Pretty-print the command using %q quoting.
run_quiet() {
  local pretty=""
  for arg in "$@"; do pretty+=" $(printf '%q' "$arg")"; done
  pretty="${pretty# }"
  if [[ "${ATFL_DEBUG:-0}" == "1" ]]; then
    log "$pretty"
    "$@"
  else
    if "$@" >/dev/null 2>&1; then
      return 0
    else
      err "Command failed: $pretty"
      log "Re-running command with output for diagnostics:"
      "$@"
    fi
  fi
}

need() { command -v "$1" >/dev/null 2>&1 || { err "Missing required command: $1"; return 1; }; }

prompt() {
  local q="$1" default="${2:-}" ans=""
  if [[ "${ATFL_NONINTERACTIVE:-}" == "1" ]]; then echo "$default"; return 0; fi
  read -r -p "$q" ans || true
  [[ -z "$ans" ]] && echo "$default" || echo "$ans"
}

list_choices(){ local -n a=$1; local i=0; for c in "${a[@]}"; do i=$((i+1)); echo "    $i. $c"; done; }
index_of(){ local n="$1"; shift; local i=0; for x in "$@"; do i=$((i+1)); [[ "$x" == "$n" ]] && { echo "$i"; return; }; done; echo 0; }

is_supported_distro() {
  local d="$1"
  for x in "${SUPPORTED_DISTRIBUTIONS[@]}"; do
    [[ "$x" == "$d" ]] && return 0
  done
  return 1
}

print_supported_as_array() {
  echo "Supported=("
  for d in "${SUPPORTED_DISTRIBUTIONS[@]}"; do
    printf "  %q\n" "$d"
  done
  echo ")"
}

detect_distro() {
  local name="" id="" version_id="" id_norm=""
  if [[ -f /etc/redhat-release ]]; then
    version_id=$(sed -E 's/.*release ([0-9]+).*/\1/' /etc/redhat-release)
    name="RHEL-$version_id"
  elif [[ -f /etc/lsb-release ]]; then
    id=$(grep -E '^DISTRIB_ID=' /etc/lsb-release | sed 's/.*=//')
    version_id=$(grep -E '^DISTRIB_RELEASE=' /etc/lsb-release | sed 's/.*=//')
  elif [[ -f /etc/os-release ]]; then
    id=$(grep '^ID=' /etc/os-release | sed 's/.*=//' | tr -d '"')
    version_id=$(grep '^VERSION_ID=' /etc/os-release | sed 's/.*=//' | tr -d '"')
  fi

  if [[ -z "$name" && -n "$id" && -n "$version_id" ]]; then
    id_norm="$(printf '%s' "$id" | tr '[:upper:]' '[:lower:]')"
    case "$id_norm" in
      ubuntu)                    name="Ubuntu-$version_id" ;;
      amzn|amazonlinux)          name="AmazonLinux-$version_id" ;;
      rhel|redhatenterpriselinux) name="RHEL-${version_id%%.*}" ;;
      sles|sled|sle_hpc|sles_sap) name="SLES-${version_id%%.*}" ;;
      *)                         name="${id}-${version_id}" ;;
    esac
  fi
  echo "$name"
}

validate_repo_base_url() {
  local url="$1"
  if [[ "$url" != http://* && "$url" != https://* ]]; then
    err "ATFL_REPO_BASE_URL must start with http:// or https:// (got: $url)"
    return 1
  fi
}

repo_base_url() {
  local base="${ATFL_REPO_BASE_URL:-$DEFAULT_REPO_BASE_URL}"
  if [[ -n "${ATFL_REPO_BASE_URL:-}" ]]; then
    validate_repo_base_url "$base" || return 1
  fi
  echo "$base"
}

origin_from_url() {
  local url="$1"
  if [[ "$url" =~ ^(https?://[^/]+) ]]; then
    echo "${BASH_REMATCH[1]}"
    return 0
  fi
  return 1
}

join_url() {
  local base="$1" path="$2" clean_path
  clean_path="${path#./}"
  clean_path="${clean_path#/}"
  while [[ "$clean_path" == *"//"* ]]; do
    clean_path="${clean_path//\/\//\/}"
  done
  if [[ -z "$clean_path" ]]; then
    echo "${base%/}/"
  else
    echo "${base%/}/$clean_path"
  fi
}

legacy_encoded_distro_url() {
  local repo_base="$1" legacy_suffix="$2"
  echo "${repo_base%/}%3A${legacy_suffix}"
}

# Outputs:
#   line 1: package type (deb|rpm)
#   remaining lines: candidate base URLs in lookup order
distro_repo_candidates() {
  local distro="$1" repo_base="$2"
  case "$distro" in
    Ubuntu-22.04)
      echo "deb"
      join_url "$repo_base" "ubuntu/dists/jammy/"
      join_url "$repo_base" "ubuntu/"
      legacy_encoded_distro_url "$repo_base" "ubuntu-22/jammy/arm64/"
      ;;
    Ubuntu-24.04)
      echo "deb"
      join_url "$repo_base" "ubuntu/dists/noble/"
      join_url "$repo_base" "ubuntu/"
      legacy_encoded_distro_url "$repo_base" "ubuntu-24/noble/arm64/"
      ;;
    Ubuntu-26.04)
      echo "deb"
      join_url "$repo_base" "ubuntu/dists/resolute/"
      join_url "$repo_base" "ubuntu/"
      legacy_encoded_distro_url "$repo_base" "ubuntu-26/resolute/arm64/"
      ;;
    AmazonLinux-2023)
      echo "rpm"
      join_url "$repo_base" "amazonlinux/al2023/aarch64/"
      join_url "$repo_base" "amazonlinux/2023/"
      join_url "$repo_base" "amazonlinux/"
      legacy_encoded_distro_url "$repo_base" "amzn-2023/al2023/aarch64/"
      ;;
    RHEL-8)
      echo "rpm"
      join_url "$repo_base" "rhel/el8/aarch64/"
      join_url "$repo_base" "rhel/8/"
      join_url "$repo_base" "rhel/"
      legacy_encoded_distro_url "$repo_base" "rhel-8/el8/aarch64/"
      ;;
    RHEL-9)
      echo "rpm"
      join_url "$repo_base" "rhel/el9/aarch64/"
      join_url "$repo_base" "rhel/9/"
      join_url "$repo_base" "rhel/"
      legacy_encoded_distro_url "$repo_base" "rhel-9/el9/aarch64/"
      ;;
    RHEL-10)
      echo "rpm"
      join_url "$repo_base" "rhel/el10/aarch64/"
      join_url "$repo_base" "rhel/10/"
      join_url "$repo_base" "rhel/"
      legacy_encoded_distro_url "$repo_base" "rhel-10/el10/aarch64/"
      ;;
    SLES-15)
      echo "rpm"
      join_url "$repo_base" "sles/sles15/aarch64/"
      join_url "$repo_base" "sles/15/"
      join_url "$repo_base" "sles/"
      legacy_encoded_distro_url "$repo_base" "sles-15/sl15/aarch64/"
      ;;
    SLES-16)
      echo "rpm"
      join_url "$repo_base" "sles/sles16/aarch64/"
      join_url "$repo_base" "sles/16/"
      join_url "$repo_base" "sles/"
      legacy_encoded_distro_url "$repo_base" "sles-16/sl16/aarch64/"
      ;;
    *)
      return 1
      ;;
  esac
}

validate_install_root() {
  local p="$1"

  # No whitespace (space, tab, newline, etc.)
  if [[ "$p" =~ [[:space:]] ]]; then
    err "INSTALL_ROOT must not contain whitespace: '$p'"
    err "Choose a path without spaces (e.g., \$HOME/arm-atfl)."
    return 1
  fi

  # Only allow these characters: letters, digits, '/', '.', '_' and '-'
  # (Deliberately exclude ':' '&' '$' '*' '?' '{' '}' etc.)
  if [[ ! "$p" =~ ^[A-Za-z0-9._/-]+$ ]]; then
    err "INSTALL_ROOT contains unsupported characters: '$p'"
    err "Allowed characters: letters, digits, '/', '.', '_' and '-'"
    return 1
  fi

  return 0
}

# Return the last (naturally sorted) href that matches a regex from an HTML index
latest_href() {
  local html="$1" re="$2"
  echo "$html" \
    | grep -Eo "href=\"[^\"]*${re}\"" \
    | sed 's/^href="//; s/"$//' \
    | LC_ALL=C sort -V \
    | tail -n1
}

# Turn a relative/absolute-from-root href into an absolute URL based on $base
normalize_href() {
  local base="$1" href="$2"
  if [[ "$href" == http* ]]; then
    echo "$href"
  elif [[ "$href" == /* ]]; then
    local origin
    origin="$(origin_from_url "$base")" || return 1
    join_url "$origin" "$href"
  else
    join_url "$base" "$href"
  fi
}

package_pattern() {
  local pkgtype="$1" pkgid="$2"
  case "$pkgtype:$pkgid" in
    deb:atfl)  echo 'arm-toolchain-for-linux_[^"]*arm64\.deb' ;;
    deb:armpl) echo 'arm-performance-libraries_[^"]*arm64\.deb' ;;
    rpm:atfl)  echo 'arm-toolchain-for-linux-[^"]*\.aarch64\.rpm' ;;
    rpm:armpl) echo 'arm-performance-libraries-[^"]*\.aarch64\.rpm' ;;
    *)         return 1 ;;
  esac
}

deb_package_name() {
  case "$1" in
    atfl)  echo "arm-toolchain-for-linux" ;;
    armpl) echo "arm-performance-libraries" ;;
    *)     return 1 ;;
  esac
}

deb_repo_root_from_base() {
  local base="${1%/}"
  if [[ "$base" == *"/ubuntu/dists/"* ]]; then
    echo "${base%%/dists/*}/"
  elif [[ "$base" == *"/ubuntu" ]]; then
    echo "$base/"
  else
    echo "${base}/"
  fi
}

latest_from_deb_packages_metadata() {
  local base="$1" pkgid="$2" pkgname repo_root rel metadata entries best filename url
  pkgname="$(deb_package_name "$pkgid")" || return 1
  repo_root="$(deb_repo_root_from_base "$base")"

  local metadata_candidates=(
    "Packages.gz"
    "Packages"
    "main/binary-arm64/Packages.gz"
    "main/binary-arm64/Packages"
  )

  need curl
  for rel in "${metadata_candidates[@]}"; do
    url="$(join_url "$base" "$rel")"
    if [[ "$rel" == *.gz ]]; then
      metadata="$(curl -fsSL "$url" 2>/dev/null | gzip -dc 2>/dev/null || true)"
    else
      metadata="$(curl -fsSL "$url" 2>/dev/null || true)"
    fi
    [[ -n "$metadata" ]] || continue

    entries="$(
      awk -v target="$pkgname" '
        function emit() {
          if (pkg == target && ver != "" && file != "") print ver "\t" file
          pkg=""; ver=""; file=""
        }
        /^Package:[[:space:]]*/  { pkg=$0; sub(/^Package:[[:space:]]*/, "", pkg); next }
        /^Version:[[:space:]]*/  { ver=$0; sub(/^Version:[[:space:]]*/, "", ver); next }
        /^Filename:[[:space:]]*/ { file=$0; sub(/^Filename:[[:space:]]*/, "", file); next }
        /^[[:space:]]*$/         { emit(); next }
        END                      { emit() }
      ' <<< "$metadata"
    )"

    [[ -n "$entries" ]] || continue
    best="$(printf '%s\n' "$entries" | LC_ALL=C sort -t $'\t' -k1,1V | tail -n1)"
    filename="${best#*$'\t'}"
    [[ -n "$filename" ]] || continue
    join_url "$repo_root" "$filename"
    return 0
  done

  return 1
}

resolve_latest_url_for_package() {
  local base="$1" pkgtype="$2" pkgid="$3"
  local pat index href
  pat="$(package_pattern "$pkgtype" "$pkgid")" || return 1

  need curl
  index="$(curl -fsSL "$base" 2>/dev/null || true)"
  if [[ -z "$index" ]]; then
    index="$(curl -fsSL "${base%/}/" 2>/dev/null || true)"
  fi

  if [[ -n "$index" ]]; then
    href="$(latest_href "$index" "$pat")"
    if [[ -n "$href" ]]; then
      normalize_href "$base" "$href"
      return 0
    fi
  fi

  if [[ "$pkgtype" == "deb" ]]; then
    latest_from_deb_packages_metadata "$base" "$pkgid" && return 0
  fi

  return 1
}

resolve_url_from_candidates() {
  local pkgtype="$1" pkgid="$2"
  shift 2
  local base url
  for base in "$@"; do
    url="$(resolve_latest_url_for_package "$base" "$pkgtype" "$pkgid" || true)"
    if [[ -n "$url" ]]; then
      echo "$url"
      echo "$base"
      return 0
    fi
  done
  return 1
}

# Global list of files to delete (success: manual cleanup; failure: on_exit)
CLEANUP_FILES=()

on_exit() {
  local rc="${1:-0}"
  # Clean only on failure; respect --no-cleanup and download-only mode
  if (( rc != 0 )) && (( ${DO_CLEANUP:-1} == 1 )) && [[ -z "${ATFL_DISTRIBUTION_OVERRIDE:-}" ]]; then
    if ((${#CLEANUP_FILES[@]})); then
      log "Cleaning up downloaded packages due to failure: ${CLEANUP_FILES[*]}"
      rm -f -- "${CLEANUP_FILES[@]}" 2>/dev/null || true
    fi
  fi
}
trap 'rc=$?; on_exit "$rc"' EXIT

fetch() {
  need curl
  local url="$1" out="${2:-}" label="${3:-}"
  [[ -z "$out" ]] && out=$(basename "$url")
  if [[ -n "$label" ]]; then
    log "Downloading $label package: $url"
  else
    log "Downloading: $url"
  fi
  # Fail on HTTP errors (-f), silent but show errors (-sS), follow redirects (-L),
  # and retry on transient issues and conn refused.
  curl -fLsS -o "$out" \
       --retry 5 \
       --retry-delay 2 \
       --retry-max-time 120 \
       --retry-connrefused \
       --connect-timeout 20 \
       "$url"
  printf '%s\n' "$out"
}

# Decide cpio flags based on debug (verbose only in debug)
cpio_flags() {
  if [[ "${ATFL_DEBUG:-0}" == "1" ]]; then echo "idmv"; else echo "idm"; fi
}

# Extract package with a friendly label and show full command in --debug
extract_pkg() {
  local pkg="$1" type="$2" root="$3" label="${4:-Package}"
  log "Extracting $label package to '$root'"
  case "$type" in
    deb)
      need dpkg
      run_quiet dpkg --extract "$pkg" "$root"
      ;;
    rpm)
      local rpm_keep_glob="./opt/*"
      tar_extract_opt_only() {
        local archive="$1" dest="$2"
        tar -xf "$archive" -C "$dest" --wildcards "$rpm_keep_glob" >/dev/null 2>&1
      }
      if [[ "$CHOSEN_DISTRIBUTION" == "SLES-"* ]]; then
        need rpm2cpio; need cpio
        run_quiet mkdir -p "$root"
        local abspkg; abspkg="$(cd "$(dirname "$pkg")" && pwd -P)/$(basename "$pkg")"
        local CPIOF;  CPIOF="$(cpio_flags)"
        local shown="(cd '$(printf '%q' "$root")' && rpm2cpio '$(printf '%q' "$abspkg")' | cpio -$CPIOF '$(printf '%q' "$rpm_keep_glob")')"
        run_cpio_extract_filtered() {
          ( set -euo pipefail; cd "$root" && rpm2cpio "$abspkg" | cpio -$CPIOF "$rpm_keep_glob" )
        }
        if [[ "${ATFL_DEBUG:-0}" == "1" ]]; then
          log "$shown"
          run_cpio_extract_filtered
        elif ! run_cpio_extract_filtered >/dev/null 2>&1; then
          err "Command failed: $shown"
          log "Re-running command with output for diagnostics:"
          run_cpio_extract_filtered
        fi
        if [[ -d "$root/bin" ]]; then
          log "Removing extraneous SLES-created '$root/bin'"
          rm -rf "$root/bin"
        fi
      else
        if command -v rpm2archive >/dev/null 2>&1; then
          need tar
          run_quiet mkdir -p "$root"

          # Resolve the RPM path to an absolute path for safety
          local abspkg
          abspkg="$(cd "$(dirname "$pkg")" && pwd -P)/$(basename "$pkg")"

          case "$CHOSEN_DISTRIBUTION" in
            RHEL-10)
              # RHEL10: rpm2archive streams to stdout; capture it to a file
              local out_archive="${abspkg}.tar"
              # Redirect inside the subshell so run_quiet's outer redirection doesn't swallow it
              run_quiet bash -c 'rpm2archive "$1" > "$2"' _ "$abspkg" "$out_archive"

              if [[ ! -s "$out_archive" ]]; then
                err "rpm2archive produced no archive for: $abspkg"
                return 1
              fi

              tar_extract_opt_only "$out_archive" "$root"
              CLEANUP_FILES+=("$out_archive")
              ;;
            *)
              # Standard behavior (RHEL 8/9, AmazonLinux-2023)
              run_quiet rpm2archive "$abspkg"
              local tgz="$abspkg.tgz"
              tar_extract_opt_only "$tgz" "$root"
              CLEANUP_FILES+=("$tgz")
              ;;
          esac
        else
          err "rpm2archive not found in PATH"; return 1
        fi
      fi
      ;;
    *) err "Unknown package type: $type"; return 1 ;;
  esac
}

patch_modulefiles(){
  local inst_root="${1-}"
  [[ -n "$inst_root" ]] || { err "patch_modulefiles: missing install root"; return 1; }
  local mfdir="$inst_root/$MODULEFILES_DIR/$ATFL_MODULE_SUBDIR"
  [[ -d "$mfdir" ]] || { err "Modulefiles directory not found: $mfdir"; return 1; }

  local atfl_ver="" mf toolchain_dir="$inst_root/$ATFL_DIR"
  for mf in "$mfdir"/*; do
    [[ -f "$mf" ]] || continue
    atfl_ver="$(basename "$mf")"
    # Replace the WHOLE install_prefix line with the custom path
    sed -Ei "s|^set[[:space:]]+install_prefix.*$|set install_prefix $toolchain_dir|" "$mf"
    # Ensure PATH includes $install_prefix/bin (insert if missing)
    grep -qE '^[[:space:]]*prepend-path[[:space:]]+PATH[[:space:]]+\$install_prefix/bin' "$mf" || \
      sed -i '/^[[:space:]]*# Standard environment variables/i prepend-path PATH $install_prefix/bin' "$mf"
  done
  [[ -n "$atfl_ver" ]] || { err "Could not deduce ATfL module version under $mfdir"; return 1; }
  echo "$atfl_ver"
}

retarget_libamath_symlinks() {
  local inst_root="${1-}" triple_dir
  [[ -n "$inst_root" ]] || { err "retarget_libamath_symlinks: missing install root"; return 1; }
  case "$CHOSEN_DISTRIBUTION" in
    AmazonLinux-2023) triple_dir="aarch64-amazon-linux" ;;
    *)                 triple_dir="aarch64-unknown-linux-gnu" ;;
  esac
  local to_dir="$inst_root/$ATFL_DIR/lib/$triple_dir"
  local from_glob="$inst_root/$ARMPL_DIR/lib/libamath.*"
  [[ -d "$to_dir" ]] || { err "Expected ATfL lib dir missing: $to_dir"; return 1; }
  local f
  for f in $from_glob; do
    [[ -e "$f" ]] || continue
    ln -sfn "$f" "$to_dir/"
  done
}

print_post_install() {
  local root="$1" atfl_ver="$2"
  echo
  echo "Installation complete. To use ATfL in your shell, run:"
  cat <<EOF
  # Add modulefiles to your MODULEPATH
  module use $root/$MODULEFILES_DIR

  # Load the ATfL compiler and Arm Performance Libraries
  module load atfl/$atfl_ver
  module load arm-performance-libraries

  # Verify the compiler is available
  armclang --version
  armclang++ --version
EOF
  echo
}

print_docs_link() {
  echo "For installation instructions of downloaded packages, please see:"
  echo "  https://developer.arm.com/downloads/-/arm-toolchain-for-linux"
}

self_test() {
  local root="$1" ver="$2"
  if ! type module >/dev/null 2>&1; then
    log "Self-test skipped: 'module' command not found."
    return 0
  fi
  log "Running self-test..."
  (
    set -e
    module use "$root/$MODULEFILES_DIR"
    module load "atfl/$ver"
    module load "arm-performance-libraries"

    echo "[ATfL] Running: armclang --version"
    armclang --version || true

    echo "[ATfL] Running: armclang++ --version"
    armclang++ --version || true

    if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists armpl-lp64-seq; then
      echo "[ATfL] pkg-config 'armpl-lp64-seq' found; compiling hello-world..."
      tmpdir="$(mktemp -d -t atfl-hello-XXXXXX)"
      src="$tmpdir/hello.cpp"
      bin="$tmpdir/hello"
      cat > "$src" <<'CPP'
#include <iostream>
int main() {
  std::cout << "Hello from Arm Toolchain for Linux (ATfL)!" << std::endl;
  return 0;
}
CPP
      flags="$(pkg-config armpl-lp64-seq --cflags --libs || true)"; [[ -n "$flags" ]] || flags=""
      echo "[ATfL] Running: armclang++ \"$src\" -o \"$bin\" $flags"
      [[ "${ATFL_DEBUG:-0}" == "1" ]] && set -x
      armclang++ "$src" -o "$bin" $flags
      [[ "${ATFL_DEBUG:-0}" == "1" ]] && set +x
      echo "[ATfL] Running: \"$bin\""
      "$bin" || true
      if [[ "${ATFL_DEBUG:-0}" != "1" ]]; then rm -rf "$tmpdir"; else echo "[ATfL] Keeping test files in $tmpdir (debug mode)"; fi
    else
      echo "[ATfL] pkg-config not available or 'armpl-lp64-seq' .pc not found; skipping compile test."
      echo "       Tip: Ubuntu: 'sudo apt-get install pkg-config'; RHEL-like: 'sudo dnf install pkgconf-pkg-config'."
    fi
  )
}

# ============================ CLI Parsing ====================================
INSTALL_ROOT="${INSTALL_ROOT:-}"
SELF_TEST=0
YES=0
DO_CLEANUP=1
DOWNLOAD_ONLY=0     # set to 1 when target OS != detected OS (or host unsupported)

POSITIONALS=()
while [[ $# -gt 0 ]]; do
  case "$1" in
    -y|--yes)        YES=1; ATFL_NONINTERACTIVE=1; SELF_TEST=1; shift ;;
    --self-test)     SELF_TEST=1; shift ;;
    --no-cleanup)    DO_CLEANUP=0; shift ;;
    --debug)         ATFL_DEBUG=1; shift ;;
    --install-root)  INSTALL_ROOT="$2"; shift 2 ;;
    -h|--help)       usage; exit 0 ;;
    --)              shift; while [[ $# -gt 0 ]]; do POSITIONALS+=("$1"); shift; done; break ;;
    -*)              err "Unknown option: $1"; usage; exit 1 ;;
    *)               POSITIONALS+=("$1"); shift ;;
  esac
done

# Accept a single positional as INSTALL_ROOT
if [[ -z "${INSTALL_ROOT:-}" && ${#POSITIONALS[@]} -ge 1 ]]; then
  INSTALL_ROOT="${POSITIONALS[0]}"
fi
if [[ ${#POSITIONALS[@]} -gt 1 ]]; then
  err "Unexpected extra argument(s): ${POSITIONALS[*]:1}"; usage; exit 1
fi

# Allow debug after flag parsing too
[[ "${ATFL_DEBUG:-0}" == "1" ]] && set -x

# ================================ Main =======================================
if [[ $(id -u) -eq 0 ]]; then
  err "This script is for NON-ROOT installs only. Please run as a regular user."
  exit 1
fi

# Resolve install root (REQUIRED)
if [[ -z "${INSTALL_ROOT:-}" ]]; then
  err "INSTALL_ROOT is required. Provide it as a positional argument or with --install-root <dir>."
  usage
  exit 1
fi

# Validate install root
if ! validate_install_root "$INSTALL_ROOT"; then
  usage
  exit 1
fi


mkdir -p "$INSTALL_ROOT"
INSTALL_ROOT="$(cd "$INSTALL_ROOT" && pwd -P)"

# Detect distro
DETECTED_DISTRIBUTION="$(detect_distro)"
DEFAULT_IDX="$(index_of "$DETECTED_DISTRIBUTION" "${SUPPORTED_DISTRIBUTIONS[@]}")"
echo "Detected host: ${DETECTED_DISTRIBUTION:-unknown}"

# Choose target distro (or override)
if [[ -n "${ATFL_DISTRIBUTION_OVERRIDE:-}" ]]; then
  CHOSEN_DISTRIBUTION="$ATFL_DISTRIBUTION_OVERRIDE"
  DOWNLOAD_ONLY=1
  log "Override requested: $CHOSEN_DISTRIBUTION (download-only)"
else
  CHOSEN_DISTRIBUTION=""
  if [[ "${ATFL_NONINTERACTIVE:-}" == "1" && "$DEFAULT_IDX" -gt 0 ]]; then
    CHOSEN_DISTRIBUTION="${SUPPORTED_DISTRIBUTIONS[$((DEFAULT_IDX-1))]}"
  fi
  if [[ -z "$CHOSEN_DISTRIBUTION" ]]; then
    echo "Choose target OS for ATfL:"
    list_choices SUPPORTED_DISTRIBUTIONS
    if [[ "$DEFAULT_IDX" -gt 0 ]]; then
      echo "    Default: $DEFAULT_IDX (${SUPPORTED_DISTRIBUTIONS[$((DEFAULT_IDX-1))]})"
    else
      echo "    Default: N/A (host unsupported)"
    fi
    ans="$(prompt "Your choice [1-${#SUPPORTED_DISTRIBUTIONS[@]}] ? " "$DEFAULT_IDX")"
    [[ "$ans" =~ ^[0-9]+$ ]] || { err "Invalid selection"; exit 1; }
    [[ "$ans" -ge 1 && "$ans" -le ${#SUPPORTED_DISTRIBUTIONS[@]} ]] || { err "Invalid selection"; exit 1; }
    CHOSEN_DISTRIBUTION="${SUPPORTED_DISTRIBUTIONS[$((ans-1))]}"
  fi

  # If host is unsupported OR chosen != detected host, treat as override (download-only)
  if [[ "$DEFAULT_IDX" -eq 0 ]]; then
    DOWNLOAD_ONLY=1
    log "Host '${DETECTED_DISTRIBUTION:-unknown}' unsupported; performing download-only for $CHOSEN_DISTRIBUTION"
  elif [[ -n "$DETECTED_DISTRIBUTION" && "$CHOSEN_DISTRIBUTION" != "$DETECTED_DISTRIBUTION" ]]; then
    DOWNLOAD_ONLY=1
    log "OS override via prompt: target=$CHOSEN_DISTRIBUTION, host=$DETECTED_DISTRIBUTION (download-only)"
  fi
fi

# Guard: chosen distribution must be supported
if ! is_supported_distro "$CHOSEN_DISTRIBUTION"; then
  err "Unsupported distribution: $CHOSEN_DISTRIBUTION"
  print_supported_as_array
  exit 1
fi

# Resolve effective repo base, package type, and ordered candidate bases
REPO_BASE="$(repo_base_url)" || exit 1
log "Repo base URL: $REPO_BASE"

readarray -t MAP < <(distro_repo_candidates "$CHOSEN_DISTRIBUTION" "$REPO_BASE" 2>/dev/null || true)
if (( ${#MAP[@]} < 2 )); then
  err "Unsupported distribution: $CHOSEN_DISTRIBUTION"
  print_supported_as_array
  exit 1
fi
PKGTYPE="${MAP[0]}"
REPO_CANDIDATES=("${MAP[@]:1}")

# Resolve package URLs (supports mixed source: ATfL and ArmPL can come from different candidates)
ATFL_SELECTED_BASE=""
ARMPL_SELECTED_BASE=""
if [[ -n "${ATFL_ATFL_PKG_URL:-}" && -n "${ATFL_ARMPL_PKG_URL:-}" ]]; then
  ATFL_URL="$ATFL_ATFL_PKG_URL"
  ARMPL_URL="$ATFL_ARMPL_PKG_URL"
  ATFL_SELECTED_BASE="pinned"
  ARMPL_SELECTED_BASE="pinned"
else
  readarray -t ATFL_RES < <(resolve_url_from_candidates "$PKGTYPE" "atfl" "${REPO_CANDIDATES[@]}" || true)
  readarray -t ARMPL_RES < <(resolve_url_from_candidates "$PKGTYPE" "armpl" "${REPO_CANDIDATES[@]}" || true)

  if (( ${#ATFL_RES[@]} >= 2 )); then
    ATFL_URL="${ATFL_RES[0]}"
    ATFL_SELECTED_BASE="${ATFL_RES[1]}"
  else
    ATFL_URL=""
  fi

  if (( ${#ARMPL_RES[@]} >= 2 )); then
    ARMPL_URL="${ARMPL_RES[0]}"
    ARMPL_SELECTED_BASE="${ARMPL_RES[1]}"
  else
    ARMPL_URL=""
  fi
fi

if [[ -z "${ATFL_URL:-}" || -z "${ARMPL_URL:-}" ]]; then
  err "Failed to resolve package URLs for $CHOSEN_DISTRIBUTION from configured candidates."
  exit 1
fi

log "ATfL selected base : $ATFL_SELECTED_BASE"
log "ArmPL selected base: $ARMPL_SELECTED_BASE"
log "ATfL package URL : $ATFL_URL"
log "ArmPL package URL: $ARMPL_URL"

# Download artifacts
ATFL_PKG="$(fetch "$ATFL_URL" "" "ATfL")";  CLEANUP_FILES+=("$ATFL_PKG")
ARMPL_PKG="$(fetch "$ARMPL_URL" "" "ArmPL")"; CLEANUP_FILES+=("$ARMPL_PKG")

# Download-only mode (any override or unsupported host)
if (( DOWNLOAD_ONLY == 1 )); then
  echo
  log "Download-only mode. No installation performed."
  print_docs_link
  exit 0
fi

# Extract/install (with friendly labels; full commands shown in --debug)
extract_pkg "$ATFL_PKG" "$PKGTYPE" "$INSTALL_ROOT" "ATfL"
extract_pkg "$ARMPL_PKG" "$PKGTYPE" "$INSTALL_ROOT" "ArmPL"

# Patch modulefiles + fix symlinks
ATFL_VERSION="$(patch_modulefiles "$INSTALL_ROOT")"
retarget_libamath_symlinks "$INSTALL_ROOT"

# Post-install info
print_post_install "$INSTALL_ROOT" "$ATFL_VERSION"

# Cleanup on success (unless --no-cleanup)
if (( ${DO_CLEANUP:-1} == 1 )) && ((${#CLEANUP_FILES[@]})); then
  log "Removing downloaded packages: ${CLEANUP_FILES[*]}"
  rm -f -- "${CLEANUP_FILES[@]}" 2>/dev/null || true
fi

# Optional self-test
if (( SELF_TEST == 1 )); then
  self_test "$INSTALL_ROOT" "$ATFL_VERSION" || true
  if (( YES == 1 )); then
    log "Running self-test completed!"
  fi
fi
