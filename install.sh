#!/bin/bash

# Install gedit syntax highlighting
sudo cp stuff/ripe.lang /usr/share/gtksourceview-2.0/language-specs/

# Install Ripe
sudo cp -R product /opt/ripe
sudo cp shell/ripe_profile.sh /etc/profile.d/
