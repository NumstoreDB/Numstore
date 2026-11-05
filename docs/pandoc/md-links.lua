-- Rewrite [text](foo.md) -> [text](foo.html). Links in the docs are written
-- relative to the page they sit on, which is what GitHub renders too, so the
-- path needs no adjusting.

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
        return el
    end

    return nil
end
