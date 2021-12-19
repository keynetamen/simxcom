#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

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

    if(window == None)
        return NULL;

    if(XGetWindowProperty(dpy, window, prop, 0, 1024, False, AnyPropertyType,
        &type, &format, &len, &extra, &result) == Success && result)
            return (char*)result;
    else
        return NULL;
}

void draw_rectangle(cairo_t *cr, int x, int y, int width, int height)
{
    cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 1.0);
    cairo_rectangle(cr, x, y, width, height);
    cairo_fill(cr);
}

Window overlay_active(Display *dpy, Window root, XVisualInfo vinfo,
    Window active, cairo_surface_t* surf, cairo_t* cr, int width, int height)
{
    Window r;
    int x, y;
    unsigned int w, h, bw, d;
    XGetGeometry(dpy, active, &r, &x, &y, &w, &h, &bw, &d);

    XSetWindowAttributes attrs;
    attrs.colormap = XCreateColormap(dpy, root, vinfo.visual, AllocNone);
    attrs.border_pixel = 0;
    unsigned long value_mask = CWColormap | CWBorderPixel;
    
    x = w - width;
    y = 0;

    Window overlay = XCreateWindow(dpy, active, x, y, width, height, 0,
        vinfo.depth, InputOutput, vinfo.visual, value_mask, &attrs);
    XMapWindow(dpy, overlay);

    surf = cairo_xlib_surface_create(dpy, overlay, vinfo.visual, width, height);
    cr = cairo_create(surf);

    draw_rectangle(cr, 0, 0, width, height);
    XFlush(dpy);

    return overlay;
}

int main()
{
    bool quit = false;
    /* int exit_code = 0; */

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
    
    long root_event_mask = PropertyChangeMask;
    XSelectInput(dpy, root, root_event_mask);

    Atom net_active_window = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", True);
    
    Window active_window = get_active_window(dpy, root);

    int n_windows;
    Window *inactive_windows = get_inactive_windows(dpy, root,
        active_window, (unsigned long *)&n_windows);

    Window aw_overlay;
    cairo_surface_t *aw_surf = NULL;
    cairo_t *aw_cr = NULL;

    int width  = 50;
    int height = 50;
    if(active_window)
        aw_overlay = overlay_active(dpy, root, vinfo, active_window,
            aw_surf, aw_cr, width, height);

    do {
        XEvent event;
        XPropertyEvent property_event;
        XNextEvent(dpy, &event);

        if(event.type == PropertyNotify) {
            property_event = event.xproperty;
            if(property_event.atom == net_active_window) {
                if(active_window) { /* destroy previous aw_overlay window */
                    cairo_destroy(aw_cr);
                    cairo_surface_destroy(aw_surf);
                    XDestroyWindow(dpy, aw_overlay);
                }
                active_window = get_active_window(dpy, root);
                if(active_window)
                    aw_overlay = overlay_active(dpy, root, vinfo,
                        active_window, aw_surf, aw_cr, width, height);

                if(inactive_windows) {
                    free(inactive_windows);
                    inactive_windows = get_inactive_windows(dpy, root,
                        active_window, (unsigned long *)&n_windows);
                }
            }
        }
    } while(!quit);

    cairo_destroy(aw_cr);
    cairo_surface_destroy(aw_surf);
    XDestroyWindow(dpy, aw_overlay);
    
    free(inactive_windows);
    XCloseDisplay(dpy);
    return EXIT_SUCCESS;
}
