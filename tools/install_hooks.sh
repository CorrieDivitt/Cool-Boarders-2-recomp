#!/bin/sh
# Run once after cloning:  sh tools/install_hooks.sh
git config core.hooksPath .githooks && echo "IP guard hooks installed (.githooks)."
