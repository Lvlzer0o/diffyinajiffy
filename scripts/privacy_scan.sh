#!/usr/bin/env sh

set -eu

# Lightweight scanner for sensitive patterns in diffs/files.
# Modes:
#   --staged  : scan added staged lines (for pre-commit)
#   --tracked : scan tracked files in working tree (for pre-push)

MODE="--staged"
if [ "${1:-}" = "--tracked" ]; then
  MODE="--tracked"
fi

PATTERN='([A-Za-z]:\\Users\\|/Users/|/home/|AppData|workspaceStorage|\.ssh|id_rsa|BEGIN[[:space:]]+[A-Z ]*PRIVATE KEY|AKIA[0-9A-Z]{16}|ghp_[A-Za-z0-9]{20,}|github_pat_[A-Za-z0-9_]{20,}|AIza[0-9A-Za-z_-]{20,}|xox[baprs]-[A-Za-z0-9-]{10,}|password[[:space:]]*[=:]|secret[[:space:]]*[=:]|token[[:space:]]*[=:]|api[_-]?key[[:space:]]*[=:])'

echo "[privacy-scan] Running ${MODE} scan..."

if [ "$MODE" = "--staged" ]; then
  # Only inspect newly added staged lines to keep pre-commit fast and focused.
  STAGED_ADDITIONS="$(git diff --cached --unified=0 --no-color --text | grep -E '^\+[^+]' || true)"

  if [ -z "$STAGED_ADDITIONS" ]; then
    echo "[privacy-scan] No staged additions to scan."
    exit 0
  fi

  if printf '%s\n' "$STAGED_ADDITIONS" | grep -En "$PATTERN" >/dev/null 2>&1; then
    echo "[privacy-scan] Potential sensitive content found in staged additions:"
    printf '%s\n' "$STAGED_ADDITIONS" | grep -En "$PATTERN" || true
    echo "[privacy-scan] Commit blocked. Remove/redact or unstage the matched lines."
    exit 1
  fi

  echo "[privacy-scan] Staged additions look clean."
  exit 0
fi

TRACKED_FILES="$(git ls-files)"
if [ -z "$TRACKED_FILES" ]; then
  echo "[privacy-scan] No tracked files to scan."
  exit 0
fi

if git grep -n -I -E "$PATTERN" -- $TRACKED_FILES >/dev/null 2>&1; then
  echo "[privacy-scan] Potential sensitive content found in tracked files:"
  git grep -n -I -E "$PATTERN" -- $TRACKED_FILES || true
  echo "[privacy-scan] Push blocked. Remove/redact matched content before pushing."
  exit 1
fi

echo "[privacy-scan] Tracked files look clean."
