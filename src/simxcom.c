#include <stdbool.h>
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

Window *get_inactive_windows(Display *dpy, Window root, Window active_window,
                            unsigned long *n_windows)
{
    Atom prop = XInternAtom(dpy, "_NET_CLIENT_LIST", True), type;
    Window *client_windows, *inactive_windows;
    int format;
    unsigned long extra;
    unsigned char *result = NULL;

    if(XGetWindowProperty(dpy, root, prop, 0, 1024, False, XA_WINDOW, &type,
        &format, n_windows, &extra, &result) == Success && result) {
            client_windows = (Window *)result;
            (*n_windows)--;
            inactive_windows = (Window *)malloc(*n_windows * sizeof(Window));

            for(int i = 0, j = 0; i < *n_windows + 1; i++) {
                if(client_windows[i] != active_window) {
                    inactive_windows[j] = client_windows[i];
                    j++;
                }
            }
            XFree(client_windows);
            return inactive_windows;
    }
    else
        return NULL;
}

char *get_window_name(Display *dpy, Window window)
{
    Atom prop = XInternAtom(dpy, "WM_NAME", False), type;
    int format;
    unsigned long extra, len;
    unsigned char *result;

    if(XGetWindowProperty(dpy, window, prop, 0, 1024, False, AnyPropertyType,
        &type, &format, &len, &extra, &result) == Success && result)
            return (char*)result;
    else
        return NULL;
}

int main()
{
    bool quit = false;
    int exit_code = 0;

    Display *dpy = XOpenDisplay(NULL);
    if(!dpy) {
        printf("simxcom: failed to open display");
        exit_code = 1;
    }

    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);

    do {
        Window active_window = get_active_window(dpy, root);
        int n_windows;
        Window *inactive_windows = get_inactive_windows(dpy, root,
            active_window, (unsigned long *)&n_windows);
        free(inactive_windows);
    } while(!quit);

    XCloseDisplay(dpy);
    return exit_code;
}
