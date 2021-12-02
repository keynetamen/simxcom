#include <stdio.h>
#include <stdlib.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>

Window get_active_window(Display *dpy, Window root)
{
    Atom prop = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", True), type;
    Window active;
    int format;
    unsigned long len, extra;
    unsigned char *result = NULL;

    if(XGetWindowProperty(dpy, root, prop, 0L, sizeof(active), False, XA_WINDOW,
        &type, &format, &len, &extra, &result) == Success && result) {
            active = *(Window *)result;
            XFree(result);
            return active;
    }
    else
        return None;
}

int main()
{
    Display *dpy = XOpenDisplay(NULL);
    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);

    if(!(dpy = XOpenDisplay(NULL))) {
        printf("simxcom: failed to open display");
        exit(EXIT_FAILURE);
    }

    Window active_window = get_active_window(dpy, root);

    XCloseDisplay(dpy);
    return EXIT_SUCCESS;
}
