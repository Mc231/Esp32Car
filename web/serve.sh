#!/usr/bin/env bash
# Local dev server for the rover web app.
#
# Browsers refuse to open WebSockets from `file://` origins, so the page
# has to be served by HTTP. This wrapper picks Python 3 (pre-installed on
# macOS / most Linuxes), serves the `web/` directory, and opens the page
# in your default browser.
#
# Usage:
#   ./serve.sh              # port 8000
#   ./serve.sh 8080         # custom port
#   PORT=8080 ./serve.sh    # custom port via env

set -euo pipefail

# Always serve from the directory this script lives in, regardless of cwd.
cd "$(dirname "$0")"

PORT="${1:-${PORT:-8000}}"
URL="http://localhost:${PORT}/"

if ! command -v python3 >/dev/null 2>&1; then
  echo "error: python3 not found on PATH" >&2
  echo "install Python 3, or use: npx serve -l ${PORT} ." >&2
  exit 1
fi

# Try to open the browser once the server is listening. Fire-and-forget
# subshell with a short delay so we don't beat python3 to the punch.
(
  sleep 0.6
  if   command -v open     >/dev/null 2>&1; then open     "${URL}"
  elif command -v xdg-open >/dev/null 2>&1; then xdg-open "${URL}"
  fi
) &

echo "Serving rover web app at ${URL}"
echo "Press Ctrl-C to stop."
exec python3 -m http.server "${PORT}" --bind 127.0.0.1
