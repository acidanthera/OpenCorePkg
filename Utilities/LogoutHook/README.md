LogoutHook
===========

## Installation
```sudo defaults write com.apple.loginwindow LogoutHook /path/to/LogoutHook.command```

## Notes
`LogoutHook.command` highly depends on macOS `nvram` utility supporting `-x` option, which is unavailable on 10.12 and below. (Our `nvram.mojave` somehow fixes that issue by invoking it instead of system one)