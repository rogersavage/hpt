# term
Minimal code for full-screen text "painting" in the terminal with fast
full-screen refresh.

The current version uses the following unicode characters
for color dithering:

Light Shade	- U+2591
Shade       - U+2592
Dark Shade	- U+2593
Full Block	- U+2588

I have tested on Debian-based antiX 22 in the following terminals:

XTerm, URXVT, RoxTerm, AlacriTTy, KiTTY, FBTerm

This works with all of them, although the clarity of the colors and
dithering is best in KiTTY or in a real TTY. URXVT and RoxTerm have
some slowdown on my machine.

In a TTY session (ctrl-alt-function key), I use the "VGA" font by running:

sudo dpkg-reconfigure console-setup

This gives me the best overall results, with KiTTY being a close
second. The colors will look much better with a font size in which the
dithering characters are not distorted. This is "VGA" 8x16 or 8x14 in a TTY,
or for example Less Perfect DOS VGA 437 at 12pt.
