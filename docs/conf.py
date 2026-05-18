project = "Numstore"
author  = "Theo Lincke"
release = open("../version.txt").read().strip()
version = ".".join(release.split(".")[:2])   

extensions = ["sphinx.ext.autosectionlabel"]
autosectionlabel_prefix_document = True

root_doc = "docs/index"   

# Anything that isn't part of the docs tree must be excluded so Sphinx
# doesn't try to parse it as an .rst file.
exclude_patterns = [
    "_build", "**/_build", "**/_build/**",
    "libs/*/include/**", "libs/*/src/**",
    "apps/*/*.c", "apps/*/CMakeLists.txt",
    "samples/*.c",
    "**/CMakeLists.txt",
    "build", "**/build/**",
    ".git", ".venv", "venv",
]

html_theme = "furo"
