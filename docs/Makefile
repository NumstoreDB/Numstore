PANDOC := $(shell command -v pandoc 2>/dev/null)
ifeq ($(PANDOC),)
$(error pandoc not found. Install it or set PANDOC explicitly, e.g. make PANDOC=/path/to/pandoc)
endif

PANDOC_LUA      := pandoc/md-links.lua
PANDOC_CSS      := pandoc/style.css
PANDOC_TEMPLATE := pandoc/template.html
PANDOC_SIDEBAR  := pandoc/_sidebar.html

PANDOC_ARGS := \
	--from=markdown \
	--to=html5 \
	--standalone \
	--embed-resources \
	--lua-filter=$(PANDOC_LUA) \
	--css=$(PANDOC_CSS) \
	--template=$(PANDOC_TEMPLATE) \
	--include-before-body=$(PANDOC_SIDEBAR)

PANDOC_DEPS := $(PANDOC_CSS) $(PANDOC_TEMPLATE) $(PANDOC_LUA) $(PANDOC_SIDEBAR)

MD_FILES := $(shell find . -name '*.md' | sed 's|^\./||')
$(info $(MD_FILES))

# Map src/foo/bar.md -> build/html/foo/bar.html
HTML_OUTPUTS := $(patsubst %.md,$(HTML_DIR)/%.html,$(MD_FILES))
$(info $(HTML_OUTPUTS))

all: $(HTML_OUTPUTS)

$(HTML_DIR)/%.html: %.md $(PANDOC_DEPS)
	$(PANDOC) $(PANDOC_ARGS) --output $@ $<

clean:
	rm -rf $(HTML_OUTPUTS)
