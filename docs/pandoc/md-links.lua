-- Rewrite [text](foo.md) -> [text](foo.html), and fix up any link written
-- relative to the docs/ root so it resolves correctly no matter how deep
-- the current file is nested (e.g. docs/man/man1/foo.1.md).

-- Path of the CURRENT source file, relative to docs/, e.g. "man/man1/foo.1.md"
local src_rel = os.getenv("PANDOC_SRC_REL") or ""

-- How many directories deep is the current file (not counting the filename)?
local depth = 0
for _ in src_rel:gmatch("[^/]+/") do
    depth = depth + 1
end
local up_prefix = string.rep("../", depth)

function Link(el)
    local target = el.target

    -- Skip absolute URLs: http://, https://, mailto:, ftp:, etc.
    if target:match("^%a[%w+.-]*:") then
        return nil
    end
    -- Skip pure in-page anchors
    if target:match("^#") then
        return nil
    end

    -- Separate the path from any #fragment or ?query
    local path, suffix = target:match("^([^#?]*)(.*)$")

    if path and path:match("%.md$") then
        local html_path = path:gsub("%.md$", ".html")
        el.target = up_prefix .. html_path .. suffix
        return el
    end

    return nil
end
