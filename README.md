# term
Minimal code for full-screen text "painting" in the terminal with fast
full-screen refresh.

The current version uses the following extended unicode characters
for color dithering:

Light Shade	- U+2591
Shade				- U+2592
Dark Shade	- U+2593
Full Block	- U+2588

Tested and works for me on the following terminals:

XTerm
RoxTerm
AlacriTTY
TTY in Debian-based antiX-22

Works but is slow in:

URXVT
FBTerm

Tested and works with the following fonts:

Monospace
Liberation Mono
Ubuntu Mono
Less Perfect DOS VGA 437

For my TTY, I have to use "sudo dpkg-reconfigure console-setup"
and set the font to VGA at size 8x14.
