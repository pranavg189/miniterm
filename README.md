# Miniterm

A Minimal Linux Terminal 

It is a very basic linux terminal which uses UNIX terminal API(termios.h) for terminal functionality and GTK+ for drawing the UI.

Compile the miniterm.c with the command:

``gcc `pkg-config --cflags --libs gtk+-2.0 gtksourceview-2.0` miniterm.c -o miniterm``

To Execute , run :

`./miniterm`

V 1.0 - created a basic terminal

Known issues
