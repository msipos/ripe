#!/bin/bash

# Install gedit syntax highlighting
sudo cp stuff/ripe.lang /usr/share/gtksourceview-2.0/language-specs/
sudo cp stuff/ripe.lang /usr/share/gtksourceview-3.0/language-specs/

# Install Ripe
sudo mkdir -p /opt/ripe
sudo cp -R product/* /opt/ripe/
sudo cp shell/ripe_profile.sh /etc/profile.d/
