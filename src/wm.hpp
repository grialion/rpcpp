#pragma once

/**
 * @brief Get X window property simplified.
 * Should be freed after usage.
 * 
 * @param disp Current display
 * @param win Current window
 * @param xa_prop_type Prop type, equal to the return prop type Atom, otherwise NULL will be returned
 * @param prop_name Name of the property that should be queried. Will be converted to a new Atom
 * @return char* 
 */
static char *get_property(Display *disp, Window win,
                          Atom xa_prop_type, string prop_name)
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

    if (XGetWindowProperty(disp, win, xa_prop_name, 0, (~0L), False,
                           xa_prop_type, &xa_ret_type, &ret_format,
                           &ret_nitems, &ret_bytes_after, &ret_prop) != Success)
    {
        return NULL;
    }

    if (xa_ret_type != xa_prop_type || ret_prop == nullptr)
    {
        if (xa_ret_type != xa_prop_type)
        {
            log("Invalid return type received: " + to_string(xa_ret_type), LogType::WARN);
        }
        
        if (ret_prop != nullptr)
        {
            XFree(ret_prop);
        }
        return NULL;
    }

    tmp_size = (ret_format / (16 / sizeof(long))) * ret_nitems;
    ret = (char *)malloc(tmp_size + 1);
    memcpy(ret, ret_prop, tmp_size);
    ret[tmp_size] = '\0';

    XFree(ret_prop);
    return ret;
}

string wm_info(Display *disp)
{
    Window *sup_window = NULL;
    char *wm_name = NULL;
    char *name_out;

    if (!(sup_window = (Window *)get_property(disp, DefaultRootWindow(disp),
                                              XA_WINDOW, "_NET_SUPPORTING_WM_CHECK")))
    {
        if (!(sup_window = (Window *)get_property(disp, DefaultRootWindow(disp),
                                                  XA_CARDINAL, "_WIN_SUPPORTING_WM_CHECK")))
        {
            cout << "could not get window manager\n";
        }
    }

    /* WM_NAME */
    if (!(wm_name = get_property(disp, *sup_window,
                                 XInternAtom(disp, "UTF8_STRING", False), "_NET_WM_NAME")))
    {
        if (!(wm_name = get_property(disp, *sup_window,
                                     XA_STRING, "_NET_WM_NAME")))
        {
            cout << "could not get window manager name\n";
        }
    }

    return wm_name;
}
