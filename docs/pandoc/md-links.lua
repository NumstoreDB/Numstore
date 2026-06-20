-- md-links.lua
-- Rewrite [text](foo.md) -> [text](foo.html) so internal links
-- work after pandoc converts each .md file to .html.

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
        el.target = path:gsub("%.md$", ".html") .. suffix
        return el  -- returning the modified node tells pandoc to use it
    end

    return nil  -- nil means "leave this Link alone"
end
