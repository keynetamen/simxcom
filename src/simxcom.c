#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

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
            /* In case of a virtual desktop with no active window */
            if(active_window)
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
    // int exit_code = 0;

    Display *dpy = XOpenDisplay(NULL);
    if(!dpy) {
        printf("simxcom: failed to open display");
        exit(EXIT_FAILURE);
    }

    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);

    XVisualInfo vinfo;
    if (!XMatchVisualInfo(dpy, screen, 32, TrueColor, &vinfo)) {
        printf("simxcom: 32-bit color not supported\n");
        exit(EXIT_FAILURE);
    }
    
    long event_mask = PropertyChangeMask;
    XSelectInput(dpy, root, event_mask);

    Atom net_active_window = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", True);
    
    Window active_window = get_active_window(dpy, root);
    Window aw_root;
    int aw_x, aw_y;
    unsigned int aw_width, aw_height, aw_border_width, aw_depth;

    int n_windows;
    Window *inactive_windows = get_inactive_windows(dpy, root,
        active_window, (unsigned long *)&n_windows);

    if(active_window)
        XGetGeometry(dpy, active_window, &aw_root, &aw_x, &aw_y,
            &aw_width, &aw_height, &aw_border_width, &aw_depth);

    do {
        XEvent event;
        XPropertyEvent property_event;
        XNextEvent(dpy, &event);

        if(event.type == PropertyNotify) {
            property_event = event.xproperty;
            if(property_event.atom == net_active_window) {
                active_window = get_active_window(dpy, root);
                if(inactive_windows) {
                    free(inactive_windows);
                    inactive_windows = get_inactive_windows(dpy, root,
                        active_window, (unsigned long *)&n_windows);
                }
                if(active_window)
                    XGetGeometry(dpy, active_window, &aw_root, &aw_x, &aw_y,
                        &aw_width, &aw_height, &aw_border_width, &aw_depth);
            }
        }
    } while(!quit);

    free(inactive_windows);
    XCloseDisplay(dpy);
    return EXIT_SUCCESS;
}
