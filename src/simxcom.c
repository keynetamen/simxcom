#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>


int main()
{
    Display *dpy = XOpenDisplay(NULL);
    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);

    return EXIT_SUCCESS;
}
