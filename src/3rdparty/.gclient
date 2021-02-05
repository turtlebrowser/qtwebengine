solutions = [
  { "name"        : 'chromium',
    "url"         : 'https://github.com/chromium/chromium.git',
    "deps_file"   : 'DEPS',
    "managed"     : False,
    "custom_deps" : {
    },
    "custom_vars": {'checkout_nacl': False},
  },
]
target_os = ["linux", "mac", "win"]
