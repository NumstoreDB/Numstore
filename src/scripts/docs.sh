#!/usr/bin/env bash
#
# src/scripts/docs.sh
#
# Renders docs/**/*.md to standalone HTML via pandoc, mirroring the old
# CMake docs target. Incremental: a page is only rebuilt if its .md (or
# any shared pandoc input: template, css, lua filter, sidebar) is newer
# than the existing .html.
#
# Requires: pandoc on PATH
#
# Usage:
#   src/scripts/docs.sh            # build into target/html
#   FORCE=1 src/scripts/docs.sh    # rebuild everything
#
# Outputs:
#   target/html/**/*.html
#   target/html/assets/**         (copied images)

set -euo pipefail

ROOT_DIR="$(pwd)"
cd "$ROOT_DIR"

DOCS_DIR="docs"
OUT_DIR="target/html"

PANDOC_LUA="$DOCS_DIR/pandoc/md-links.lua"
PANDOC_CSS="$DOCS_DIR/pandoc/style.css"
PANDOC_TEMPLATE="$DOCS_DIR/pandoc/template.html"
PANDOC_SIDEBAR="$DOCS_DIR/pandoc/_sidebar.html"

PANDOC_ARGS=(
    --from=markdown
    --to=html5
    --standalone
    --embed-resources
    --lua-filter="$PANDOC_LUA"
    --css="$PANDOC_CSS"
    --template="$PANDOC_TEMPLATE"
    --include-before-body="$PANDOC_SIDEBAR"
)

command -v pandoc >/dev/null 2>&1 || {
    echo "error: pandoc not found on PATH" >&2
    exit 1
}

mkdir -p "$OUT_DIR"

# Returns 0 (true) if $1 needs rebuilding given output $2
needs_rebuild() {
    local src="$1" out="$2" dep
    [[ "${FORCE:-0}" == "1" ]] && return 0
    [[ ! -f "$out" ]] && return 0
    [[ "$src" -nt "$out" ]] && return 0
    for dep in "$PANDOC_LUA" "$PANDOC_CSS" "$PANDOC_TEMPLATE" "$PANDOC_SIDEBAR"; do
        [[ "$dep" -nt "$out" ]] && return 0
    done
    return 1
}

########################################################################
# Markdown -> HTML
########################################################################
built=0
skipped=0
while IFS= read -r -d '' md; do
    rel="${md#"$DOCS_DIR"/}"                 # path relative to docs/
    html="$OUT_DIR/${rel%.md}.html"
    if needs_rebuild "$md" "$html"; then
        mkdir -p "$(dirname "$html")"
        echo "pandoc: $rel -> ${rel%.md}.html"
        # Run from DOCS_DIR so relative links/images in the md resolve
        # the same way they did under CMake's WORKING_DIRECTORY.
        (cd "$DOCS_DIR" && pandoc "${PANDOC_ARGS[@]}" \
            --output "$ROOT_DIR/$html" "$rel")
        built=$((built + 1))
    else
        skipped=$((skipped + 1))
    fi
done < <(find "$DOCS_DIR" -name '*.md' -type f -print0)

########################################################################
# Copy image/asset files next to the HTML output
########################################################################
if [[ -d "$DOCS_DIR/assets" ]]; then
    while IFS= read -r -d '' asset; do
        rel="${asset#"$DOCS_DIR"/assets/}"
        dst="$OUT_DIR/assets/$rel"
        if [[ "${FORCE:-0}" == "1" || ! -f "$dst" || "$asset" -nt "$dst" ]]; then
            mkdir -p "$(dirname "$dst")"
            echo "asset:  $rel"
            cp "$asset" "$dst"
        fi
    done < <(find "$DOCS_DIR/assets" -type f \
        \( -name '*.png' -o -name '*.jpg' -o -name '*.jpeg' \
           -o -name '*.gif' -o -name '*.svg' -o -name '*.webp' \) -print0)
fi

echo
echo "Docs: $built built, $skipped up-to-date -> $OUT_DIR/"
