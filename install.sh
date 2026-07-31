#!/bin/sh
#
# nvi installer for Linux and macOS.
#
#   curl -fsSL https://raw.githubusercontent.com/mattcarlotta/nvi/main/install.sh | sh
#   ./install.sh --version [tag|latest] --dir "$HOME/.local/bin"
#   ./install.sh --uninstall
#
# Env overrides: NVI_VERSION, NVI_INSTALL_DIR, NVI_LIBC (gnu|musl)

set -eu

REPO="mattcarlotta/nvi"
BIN="nvi"
VERSION="${NVI_VERSION:-latest}"
INSTALL_DIR="${NVI_INSTALL_DIR:-$HOME/.local/bin}"
LIBC="${NVI_LIBC:-auto}"
SHELL_NAME="$(basename "${SHELL:-sh}")"
BEGIN_MARKER="# >>> nvi >>>"
END_MARKER="# <<< nvi <<<"
NO_PROFILE=0
UNINSTALL=0
TMPDIR_NVI=""

write_info() { printf '%s\n' "$*"; }
fail() { printf 'error: %s\n' "$*" >&2; exit 1; }
installed() { command -v "$1" >/dev/null 2>&1; }

cleanup() { [ -n "$TMPDIR_NVI" ] && rm -rf "$TMPDIR_NVI"; }
trap cleanup EXIT INT TERM

usage() {
    cat <<EOF
Usage: install.sh [options]

  -v, --version <tag>   Release tag to install (default: latest)
  -d, --dir <path>      Install directory (default: \$HOME/.local/bin)
      --libc <gnu|musl> Linux libc flavor (default: auto-detected)
      --no-profile      Print the shell profile block instead of appending it
      --uninstall       Remove an installed $BIN binary and its profile block
  -h, --help            Show this help
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        -v|--version) [ $# -ge 2 ] || fail "--version requires a value"; VERSION="$2"; shift 2 ;;
        -d|--dir)     [ $# -ge 2 ] || fail "--dir requires a value"; INSTALL_DIR="$2"; shift 2 ;;
        --libc)       [ $# -ge 2 ] || fail "--libc requires a value"; LIBC="$2"; shift 2 ;;
        --no-profile) NO_PROFILE=1; shift ;;
        --uninstall)  UNINSTALL=1; shift ;;
        -h|--help)    usage; exit 0 ;;
        *)            fail "unknown option: $1 (try --help)" ;;
    esac
done

os="$(uname -s)"
arch="$(uname -m)"

# ------------------------------------------------------------------ profile

case "$SHELL_NAME" in
    zsh)
        profile="$HOME/.zshrc"
        ;;
    bash)
        if [ "$os" = "Darwin" ]; then
            profile="$HOME/.bash_profile"
        else
            profile="$HOME/.bashrc"
        fi
        ;;
    *)
        profile=""
        ;;
esac

# The zsh and bash 4.4+ forms build the same KEY=value vector xargs would,
# but run env as a direct child of the shell so ctrl+c behaves like running
# the command directly.
#
# mapfile -d and $! for a process substitution both landed in bash 4.4, so ask
# the login shell binary itself. macOS ships bash 3.2 as /bin/bash.
bash_has_mapfile_d() {
    "${SHELL:-bash}" -c \
        '[ "${BASH_VERSINFO[0]}" -gt 4 ] || { [ "${BASH_VERSINFO[0]}" -eq 4 ] && [ "${BASH_VERSINFO[1]}" -ge 4 ]; }' \
        >/dev/null 2>&1
}

# BSD/macOS xargs rejects -r, and it already skips execution on empty input.
if [ "$os" = "Darwin" ]; then
    xargs_fn='nvix() { nvi "$@" | xargs -0 env; }'
else
    xargs_fn='nvix() { nvi "$@" | xargs -0 -r env; }'
fi

case "$SHELL_NAME" in
    zsh)
        nvix_fn="$(cat <<'EOF'
nvix() {
  local out
  out="$(nvi "$@")" || return $?
  [[ -n "$out" ]] || return 0
  env ${(0)out}
}
EOF
)"
        ;;
    bash)
        if bash_has_mapfile_d; then
            nvix_fn="$(cat <<'EOF'
nvix() {
  local args=()
  mapfile -d '' -t args < <(nvi "$@")
  wait "$!" || return $?
  ((${#args[@]})) || return 0
  env "${args[@]}"
}
EOF
)"
        else
            nvix_fn="$xargs_fn"
        fi
        ;;
    *)
        nvix_fn="$xargs_fn"
        ;;
esac

case "$SHELL_NAME" in
    zsh)
        path_snippet="typeset -U path PATH
path=(\"$INSTALL_DIR\" \$path)"
        ;;
    *)
        path_snippet="case \":\$PATH:\" in
    *\":$INSTALL_DIR:\"*) ;;
    *) export PATH=\"$INSTALL_DIR:\$PATH\" ;;
esac"
        ;;
esac

strip_profile_block() {
    [ -n "$profile" ] && [ -f "$profile" ] || return 0
    grep -qF "$BEGIN_MARKER" "$profile" || return 0
    tmp="$profile.nvi.$$"
    sed "/^$BEGIN_MARKER\$/,/^$END_MARKER\$/d" "$profile" > "$tmp"
    cat "$tmp" > "$profile"
    rm -f "$tmp"
    write_info "removed the nvi block from $profile"
}

# ---------------------------------------------------------------- uninstall

if [ "$UNINSTALL" -eq 1 ]; then
    target="$INSTALL_DIR/$BIN"
    [ -e "$target" ] || fail "no $BIN found at $target"
    rm -f "$target"
    write_info "removed $target"
    strip_profile_block
    exit 0
fi

# ---------------------------------------------------------------- downloader

if installed curl; then
    http_get()  { curl -fsSL "$1"; }
    http_head() { curl -fsSLI -o /dev/null -w '%{url_effective}' "$1"; }
    http_dl()   { curl -fsSL -o "$2" "$1"; }
elif installed wget; then
    http_get()  { wget -qO- "$1"; }
    http_head() { wget -qS --max-redirect=10 -O /dev/null "$1" 2>&1 | sed -n 's/^ *Location: *//p' | tail -n 1; }
    http_dl()   { wget -qO "$2" "$1"; }
else
    fail "curl or wget is required"
fi

installed tar || fail "tar is required"

# ------------------------------------------------------------------- target

detect_libc() {
    if [ "$LIBC" != "auto" ]; then printf '%s' "$LIBC"; return; fi
    for f in /lib/ld-musl-*.so.1 /lib64/ld-musl-*.so.1; do
        [ -e "$f" ] && { printf 'musl'; return; }
    done
    if installed ldd && ldd --version 2>&1 | grep -qi musl; then printf 'musl'; return; fi
    printf 'gnu'
}

case "$os" in
    Linux)
        libc="$(detect_libc)"
        case "$arch" in
            x86_64|amd64)
                if [ "$libc" = "musl" ]; then
                    target="linux-x86_64-musl"
                else
                    target="linux-x86_64"
                fi
                ;;
            aarch64|arm64)
                [ "$libc" = "musl" ] && fail "no prebuilt musl aarch64 binary; build from source (NVI_LIBC=musl ./nob install <dir>)"
                target="linux-aarch64"
                ;;
            *) fail "unsupported Linux architecture: $arch" ;;
        esac
        ;;
    Darwin)
        # A shell running under Rosetta reports x86_64 on Apple Silicon.
        if [ "$arch" = "x86_64" ] && [ "$(sysctl -n sysctl.proc_translated 2>/dev/null || echo 0)" = "1" ]; then
            arch="arm64"
        fi
        case "$arch" in
            arm64|aarch64) target="macos-aarch64" ;;
            x86_64) fail "no prebuilt macOS x86_64 binary; build from source (see the README)" ;;
            *) fail "unsupported macOS architecture: $arch" ;;
        esac
        ;;
    *)
        fail "unsupported OS: $os (on Windows, use install.ps1)"
        ;;
esac

# ------------------------------------------------------------------ version

if [ "$VERSION" = "latest" ]; then
    tag="$(http_get "https://api.github.com/repos/$REPO/releases/latest" 2>/dev/null \
        | tr ',' '\n' \
        | sed -n 's/.*"tag_name"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' \
        | head -n 1)"
    if [ -z "${tag:-}" ]; then
        # Fall back to the redirect target of the /releases/latest page.
        tag="$(http_head "https://github.com/$REPO/releases/latest" | sed -n 's|.*/tag/||p')"
    fi
    [ -n "${tag:-}" ] || fail "could not resolve the latest release; pass --version <tag>"
else
    case "$VERSION" in v*) tag="$VERSION" ;; *) tag="v$VERSION" ;; esac
fi

asset="$BIN-$target.tar.gz"
url="https://github.com/$REPO/releases/download/$tag/$asset"

# ------------------------------------------------------------------ install

write_info "installing $BIN $tag ($target) -> $INSTALL_DIR"

TMPDIR_NVI="$(mktemp -d "${TMPDIR:-/tmp}/nvi-install.XXXXXX")"
http_dl "$url" "$TMPDIR_NVI/$asset" || fail "download failed: $url"
tar -xzf "$TMPDIR_NVI/$asset" -C "$TMPDIR_NVI" || fail "failed to extract $asset"

src="$TMPDIR_NVI/$BIN/bin/$BIN"
[ -f "$src" ] || fail "archive did not contain $BIN/bin/$BIN"

mkdir -p "$INSTALL_DIR" || fail "could not create $INSTALL_DIR"
[ -w "$INSTALL_DIR" ] || fail "$INSTALL_DIR is not writable by $(id -un)"

rm -f "$INSTALL_DIR/$BIN"
cp "$src" "$INSTALL_DIR/$BIN"
chmod 755 "$INSTALL_DIR/$BIN"

# macOS quarantine attribute on downloaded files.
installed xattr && xattr -d com.apple.quarantine "$INSTALL_DIR/$BIN" 2>/dev/null || true

write_info ""
"$INSTALL_DIR/$BIN" version || fail "installed binary failed to run"
write_info ""

# --------------------------------------------------------------- path notice

case ":$PATH:" in
    *":$INSTALL_DIR:"*) ON_PATH=1; not="" ;;
    *)                  ON_PATH=0; not=" not" ;;
esac

write_info "$INSTALL_DIR is$not a recognized path within your PATH."

# The PATH lines are only needed when the directory isn't recognized yet,
# but nvix is always needed.
block="$BEGIN_MARKER"
if [ "$ON_PATH" -eq 0 ]; then
    block="$block
$path_snippet"
fi
block="$block
$nvix_fn
$END_MARKER"

if [ "$NO_PROFILE" -eq 1 ] || [ -z "$profile" ]; then
    [ -n "$profile" ] || write_info "Could not identify a profile for shell: $SHELL_NAME"
    write_info ""
    write_info "Add the following to your shell profile:"
    write_info ""
    printf '%s\n' "$block" | sed 's/^/    /'
elif [ -f "$profile" ] && grep -qF "$BEGIN_MARKER" "$profile"; then
    write_info ""
    write_info "$profile already contains an nvi block, so it was left unchanged."
    write_info "Run --uninstall and reinstall to regenerate it."
else
    printf '\n%s\n' "$block" >> "$profile" || fail "could not write to $profile"
    write_info ""
    write_info "Appended an nvi block to $profile:"
    write_info ""
    printf '%s\n' "$block" | sed 's/^/    /'
    write_info ""
    write_info "Please reload your current shell for the changes to go into effect:"
    write_info ""
    write_info "    source $profile"
    write_info ""
fi
