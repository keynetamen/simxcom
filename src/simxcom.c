#include <stdarg.h>
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

Window *overlay_inactive(Display *dpy, Window root, XVisualInfo vinfo,
    Window *windows, int n_windows, cairo_surface_t **surfs, cairo_t **crs)
{
    Window *inactive_overlays = (Window *)malloc(n_windows * sizeof(Window));
    surfs = (cairo_surface_t **)malloc(n_windows * sizeof(cairo_surface_t*));
    crs = (cairo_t **)malloc(n_windows * sizeof(cairo_t*));

    XSetWindowAttributes attrs;
    attrs.colormap = XCreateColormap(dpy, root, vinfo.visual, AllocNone);
    attrs.border_pixel = 0;
    unsigned long value_mask = CWColormap | CWBorderPixel;

    for(int i = 0; i < n_windows; i++) {
        Window current = windows[i];
        Window r;
        int x, y;
        unsigned int w, h, bw, d;
        XGetGeometry(dpy, current, &r, &x, &y, &w, &h, &bw, &d);

        Window overlay = XCreateWindow(dpy, current, 0, 0, w, h, 0,
        vinfo.depth, InputOutput, vinfo.visual, value_mask, &attrs);

        XMapWindow(dpy, overlay);

        surfs[i] = cairo_xlib_surface_create(dpy, overlay, vinfo.visual, w, h);
        crs[i] = cairo_create(surfs[i]);

        draw_rectangle(crs[i], 0, 0, w, h);
        XFlush(dpy);

        inactive_overlays[i] = overlay;
    }

    return inactive_overlays;
}

void die(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    fprintf(stderr, "%s: ", PROGRAM_NAME);
    vfprintf(stderr, format, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}

int main()
{
    bool quit = false;
    /* int exit_code = 0; */

    Display *dpy = XOpenDisplay(NULL);
    if(!dpy)
        die("failed to open display\n");

    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);

    XVisualInfo vinfo;
    if (!XMatchVisualInfo(dpy, screen, 32, TrueColor, &vinfo))
        die("32-bit color not supported\n");
    
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

    Window *iw_overlays;
    cairo_surface_t **iw_surfs = NULL;
    cairo_t **iw_crs = NULL;

    int width  = 50;
    int height = 50;
    if(active_window)
        aw_overlay = overlay_active(dpy, root, vinfo, active_window,
            aw_surf, aw_cr, width, height);

    if(inactive_windows)
        iw_overlays = overlay_inactive(dpy, root, vinfo, inactive_windows,
            n_windows, iw_surfs, iw_crs);

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
                    for(int i = 0; i < n_windows; i++)
                        XDestroyWindow(dpy, iw_overlays[i]);
                    free(iw_crs);
                    free(iw_surfs);
                    free(iw_overlays);
                    free(inactive_windows);
                    inactive_windows = get_inactive_windows(dpy, root,
                        active_window, (unsigned long *)&n_windows);
                    iw_overlays = overlay_inactive(dpy, root, vinfo,
                        inactive_windows, n_windows, iw_surfs, iw_crs);
                }
            }
        }
    } while(!quit);

    cairo_destroy(aw_cr);
    cairo_surface_destroy(aw_surf);
    XDestroyWindow(dpy, aw_overlay);

    for(int i = 0; i < n_windows; i++)
        cairo_destroy(iw_crs[i]);
    free(iw_crs);
    free(iw_surfs);
    free(iw_overlays);

    free(inactive_windows);
    XCloseDisplay(dpy);
    return EXIT_SUCCESS;
}
