#!/bin/bash
#
# REF: https://github.com/docker/docs/pull/19638/files
# REF: https://stackoverflow.com/a/20293759/795690
#
sudo tee -a "/etc/apparmor.d/$(echo "$HOME/bin/rootlesskit" | sed -e s@^/@@ -e s@/@.@g)" > /dev/null << EOF
abi <abi/4.0>,
include <tunables/global>

$HOME/bin/rootlesskit flags=(unconfined) {
userns,

include if exists <local/$(echo "$HOME/bin/rootlesskit" | sed -e s@^/@@ -e s@/@.@g)>
}
EOF

sudo systemctl restart apparmor.service
