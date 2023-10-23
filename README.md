# term
Minimal code for full-screen text "painting" in the terminal.

The current version uses the following extended unicode characters
for color dithering:

Light Shade	- U+2591
Shade				- U+2592
Dark Shade	- U+2593

The terminal needs to be set to UTF-8 encoding, and the font needs to
have these Block Element characters available.

On my system, I use the font named "VGA" in my TTY sessions, by running
sudo dpkg-reconfigure console-setup.
