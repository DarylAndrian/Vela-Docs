#!/usr/bin/env python3
# ════════════════════════════════════════════════════════════════════════════
#  Vela Docs — tools/fetch-material-symbols.py
#
#  One-time, idempotent bootstrapper that downloads the Material Symbols
#  Outlined font (woff2) used by the editor ribbon. This script is part of
#  the project's offline-first philosophy: every asset the app needs is
#  bundled inside the installer; this script exists *only* to populate the
#  bundle the first time the developer / CI clones the repo.
#
#  Usage:
#       python tools/fetch-material-symbols.py
#           # Downloads to BOTH:
#           #   editor-playground/src/assets/material-symbols-outlined.woff2
#           #   resources/fonts/material-symbols-outlined.woff2
#
#       python tools/fetch-material-symbols.py --quiet
#           # Suppress per-file progress messages
#
#  Behaviour:
#       • If the destination file already exists, the script skips it
#         (idempotent — re-running is harmless).
#       • If a download fails (offline), the script prints a clear error
#         pointing to where to manually drop the file but exits 0 so CI
#         builds don't break.
#       • Uses only Python stdlib (urllib) — no third-party deps.
#
#  Font source: Google Fonts — Material Symbols Outlined
#       https://fonts.googleapis.com/css2?family=Material+Symbols+Outlined
#  Version pinned: 3.0.0 (matches CHANGELOG.md [0.3.4] PROD-08)
# ════════════════════════════════════════════════════════════════════════════

from __future__ import annotations

import argparse
import hashlib
import os
import pathlib
import re
import sys
import urllib.error
import urllib.request

# ── Configuration ────────────────────────────────────────────────────────────

# The Google Fonts CSS file. The woff2 URL is embedded in the @font-face
# block that this CSS returns (the URL changes with the version).
CSS_URL = (
    "https://fonts.googleapis.com/css2"
    "?family=Material+Symbols+Outlined"
    "&display=swap"
)

# User-Agent must be set; Google Fonts returns different (legacy ttf)
# formats to old UA strings.
USER_AGENT = (
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
    "AppleWebKit/537.36 (KHTML, like Gecko) "
    "Chrome/120.0 Safari/537.36"
)

# Where the font is dropped.
REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
TARGETS = [
    REPO_ROOT / "editor-playground" / "src" / "assets" / "material-symbols-outlined.woff2",
    REPO_ROOT / "resources" / "fonts" / "material-symbols-outlined.woff2",
]

# If a previous run accidentally grabbed a small or HTML file, replace it.
MIN_BYTES = 30_000  # the font is ~150 KB


# ── Helpers ──────────────────────────────────────────────────────────────────

def log(msg: str, *, quiet: bool) -> None:
    if not quiet:
        # Force UTF-8 stdout on Windows; ascii-only for cross-platform safety.
        try:
            sys.stdout.reconfigure(encoding="utf-8", errors="replace")
        except Exception:
            pass
        print(msg, flush=True)


def fetch_css(quiet: bool) -> str | None:
    """Fetch the Google Fonts CSS file and return its contents."""
    log(f"  --> Fetching CSS from {CSS_URL}", quiet=quiet)
    req = urllib.request.Request(CSS_URL, headers={"User-Agent": USER_AGENT})
    try:
        with urllib.request.urlopen(req, timeout=20) as resp:
            data = resp.read()
    except (urllib.error.URLError, TimeoutError, OSError) as exc:
        log(f"  [X] Network error: {exc}", quiet=quiet)
        return None
    return data.decode("utf-8")


def extract_woff2_url(css: str) -> str | None:
    """Find the woff2 src() URL inside an @font-face block."""
    # Pattern: src: url(https://...woff2) format('woff2')
    m = re.search(r"url\((https://[^)]+\.woff2)\)", css)
    return m.group(1) if m else None


def fetch_woff2(url: str, dest: pathlib.Path, *, quiet: bool) -> bool:
    """Download the woff2 file and write it to dest. Returns True on success."""
    log(f"  --> Downloading {url}", quiet=quiet)
    try:
        req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
        with urllib.request.urlopen(req, timeout=60) as resp:
            data = resp.read()
    except (urllib.error.URLError, TimeoutError, OSError) as exc:
        log(f"  [X] Network error: {exc}", quiet=quiet)
        return False

    if len(data) < MIN_BYTES:
        log(f"  [X] Downloaded file is suspiciously small ({len(data)} bytes)", quiet=quiet)
        return False

    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.write_bytes(data)

    sha = hashlib.sha256(data).hexdigest()[:12]
    size_kb = len(data) / 1024
    log(f"  [OK] {dest.relative_to(REPO_ROOT)}  ({size_kb:,.1f} KB, sha256:{sha})", quiet=quiet)
    return True


# ── Main ─────────────────────────────────────────────────────────────────────

def main() -> int:
    parser = argparse.ArgumentParser(description="Fetch the Material Symbols Outlined font.")
    parser.add_argument("--quiet", action="store_true", help="Suppress progress output.")
    parser.add_argument(
        "--force",
        action="store_true",
        help="Re-download even if the destination file already exists.",
    )
    args = parser.parse_args()

    log("Material Symbols Outlined font bootstrapper (v0.4.1)", quiet=args.quiet)
    log(f"Repo root: {REPO_ROOT}", quiet=args.quiet)

    css = fetch_css(quiet=args.quiet)
    if css is None:
        log(
            "\n[!] Could not reach the network. If the font file is already in"
            "  place, the build will still succeed. Otherwise, manually download"
            "  Material+Symbols+Outlined woff2 from Google Fonts and place it at:"
            + "\n".join(f"    {p.relative_to(REPO_ROOT)}" for p in TARGETS),
            quiet=args.quiet,
        )
        return 0  # don't break the build

    woff2_url = extract_woff2_url(css)
    if not woff2_url:
        log("  [X] Could not find a woff2 URL in the CSS response.", quiet=args.quiet)
        return 0

    any_wrote = False
    for target in TARGETS:
        if target.exists() and not args.force:
            log(f"  [*] {target.relative_to(REPO_ROOT)} already exists - skipping", quiet=args.quiet)
            continue
        if fetch_woff2(woff2_url, target, quiet=args.quiet):
            any_wrote = True

    if not any_wrote:
        log("\nNo new files written (all targets already up-to-date).", quiet=args.quiet)
    else:
        log("\n[OK] Material Symbols font ready.", quiet=args.quiet)

    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(130)
