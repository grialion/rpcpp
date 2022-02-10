#pragma once

static char *get_property(Display *disp, Window win, /*{{{*/
                          Atom xa_prop_type, string prop_name, unsigned long *size)
{
    Atom xa_prop_name;
    Atom xa_ret_type;
    int ret_format;
    unsigned long ret_nitems;
    unsigned long ret_bytes_after;
    unsigned long tmp_size;
    unsigned char *ret_prop;
    char *ret;

    xa_prop_name = XInternAtom(disp, prop_name.c_str(), False);

    if (XGetWindowProperty(disp, win, xa_prop_name, 0, 1024, False,
                           xa_prop_type, &xa_ret_type, &ret_format,
                           &ret_nitems, &ret_bytes_after, &ret_prop) != Success)
    {
        return NULL;
    }

    if (xa_ret_type != xa_prop_type)
    {
        XFree(ret_prop);
        return NULL;
    }

    tmp_size = (ret_format / (32 / sizeof(long))) * ret_nitems;
    ret = (char *)malloc(tmp_size + 1);
    memcpy(ret, ret_prop, tmp_size);
    ret[tmp_size] = '\0';

    if (size)
    {
        *size = tmp_size;
    }

    XFree(ret_prop);
    return ret;
}

string wm_info(Display *disp)
{
    Window *sup_window = NULL;
    char *wm_name = NULL;
    char *name_out;

    if (!(sup_window = (Window *)get_property(disp, DefaultRootWindow(disp),
                                              XA_WINDOW, "_NET_SUPPORTING_WM_CHECK", NULL)))
    {
        if (!(sup_window = (Window *)get_property(disp, DefaultRootWindow(disp),
                                                  XA_CARDINAL, "_WIN_SUPPORTING_WM_CHECK", NULL)))
        {
            cout << "could not get window manager";
        }
    }

    /* WM_NAME */
    if (!(wm_name = get_property(disp, *sup_window,
                                 XInternAtom(disp, "UTF8_STRING", False), "_NET_WM_NAME", NULL)))
    {
        if (!(wm_name = get_property(disp, *sup_window,
                                     XA_STRING, "_NET_WM_NAME", NULL)))
        {
            cout << "could not get window manager name";
        }
    }

    return wm_name;
}