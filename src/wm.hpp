#pragma once

/**
 * @brief Get X window property simplified.
 * Should be freed after usage.
 * 
 * @param disp Current display
 * @param win Current window
 * @param xa_prop_type Prop type, equal to the return prop type Atom, otherwise NULL will be returned
 * @param prop_name Name of the property that should be queried. Will be converted to a new Atom
 * @return 1 on success, 0 on error
 */
static int get_property(Display *disp, Window win,
                          Atom xa_prop_type, string prop_name, char *ret, size_t ret_length)
{
    Atom xa_prop_name;
    Atom xa_ret_type;
    int ret_format;
    unsigned long ret_nitems;
    unsigned long ret_bytes_after;
    unsigned long tmp_size;
    unsigned char *ret_prop;

    xa_prop_name = XInternAtom(disp, prop_name.c_str(), False);

    if (XGetWindowProperty(disp, win, xa_prop_name, 0, (~0L), False,
                           xa_prop_type, &xa_ret_type, &ret_format,
                           &ret_nitems, &ret_bytes_after, &ret_prop) != Success)
    {
        return 0;
    }

    if (xa_ret_type != xa_prop_type)
    {
        log("Invalid return type received: " + to_string(xa_ret_type), LogType::WARN);
        XFree(ret_prop);

        return 0;
    }

    tmp_size = (ret_format / (16 / sizeof(long))) * ret_nitems;
    tmp_size = ret_length < tmp_size ? ret_length : tmp_size;

    memcpy(ret, ret_prop, tmp_size - 1);
    ret[tmp_size - 1] = '\0';

    XFree(ret_prop);
    return 1;
}

string wm_info(Display *disp)
{
    Window sup_window[256];
    char wm_name[256];

    if (!get_property(disp, DefaultRootWindow(disp),
                                              XA_WINDOW, "_NET_SUPPORTING_WM_CHECK",
                                              (char *)sup_window, sizeof(sup_window)))
    {
        if (!get_property(disp, DefaultRootWindow(disp),
                                                  XA_CARDINAL, "_WIN_SUPPORTING_WM_CHECK",
                                              (char *)sup_window, sizeof(sup_window)))
        {
            cout << "could not get window manager\n";
        }
    }

    /* WM_NAME */
    if (!get_property(disp, *sup_window,
                                 XInternAtom(disp, "UTF8_STRING", False), "_NET_WM_NAME",
                                 wm_name, sizeof(wm_name)))
    {
        if (!get_property(disp, *sup_window,
                                     XA_STRING, "_NET_WM_NAME",
                                 wm_name, sizeof(wm_name)))
        {
            cout << "could not get window manager name\n";
        }
    }

    return wm_name;
}
