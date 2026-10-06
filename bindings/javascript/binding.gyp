{
  # Point these at your numstore build, e.g.
  #   NUMSTORE_INCLUDE_DIR=/path/to/numstore/include \
  #   NUMSTORE_LIB_DIR=/path/to/numstore/build npm install
  "variables": {
    "numstore_include%": "<!(node -p \"process.env.NUMSTORE_INCLUDE_DIR || 'include'\")",
    "numstore_lib_dir%": "<!(node -p \"process.env.NUMSTORE_LIB_DIR || ''\")",
    "numstore_lib%": "<!(node -p \"process.env.NUMSTORE_LIB || 'numstore'\")"
  },
  "targets": [
    {
      "target_name": "numstore",
      "sources": ["src/binding.c"],
      "include_dirs": ["<(numstore_include)"],
      "defines": ["NAPI_VERSION=8"],
      "conditions": [
        ["OS!='win' and numstore_lib_dir!=''", {
          "libraries": ["-L<(numstore_lib_dir)", "-Wl,-rpath,<(numstore_lib_dir)"]
        }],
        ["OS!='win'", {
          "libraries": ["-l<(numstore_lib)"],
          "cflags": ["-std=c11", "-Wall", "-Wextra"],
          "xcode_settings": { "OTHER_CFLAGS": ["-std=c11", "-Wall", "-Wextra"] }
        }],
        ["OS=='win'", {
          "libraries": ["<(numstore_lib_dir)/<(numstore_lib).lib"]
        }]
      ]
    }
  ]
}
