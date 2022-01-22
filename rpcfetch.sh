#!/bin/bash

get_wm() {
    case $kernel_name in
        *OpenBSD*) ps_flags=(x -c) ;;
        *)         ps_flags=(-e) ;;
    esac

    if [[ -O "${XDG_RUNTIME_DIR}/${WAYLAND_DISPLAY:-wayland-0}" ]]; then
        if tmp_pid="$(lsof -t "${XDG_RUNTIME_DIR}/${WAYLAND_DISPLAY:-wayland-0}" 2>&1)" ||
           tmp_pid="$(fuser   "${XDG_RUNTIME_DIR}/${WAYLAND_DISPLAY:-wayland-0}" 2>&1)"; then
            wm="$(ps -p "${tmp_pid}" -ho comm=)"
        else
            # lsof may not exist, or may need root on some systems. Similarly fuser.
            # On those systems we search for a list of known window managers, this can mistakenly
            # match processes for another user or session and will miss unlisted window managers.
            wm=$(ps "${ps_flags[@]}" | grep -m 1 -o -F \
                               -e arcan \
                               -e asc \
                               -e clayland \
                               -e dwc \
                               -e fireplace \
                               -e gnome-shell \
                               -e greenfield \
                               -e grefsen \
                               -e hikari \
                               -e kwin \
                               -e lipstick \
                               -e maynard \
                               -e mazecompositor \
                               -e motorcar \
                               -e orbital \
                               -e orbment \
                               -e perceptia \
                               -e river \
                               -e rustland \
                               -e sway \
                               -e ulubis \
                               -e velox \
                               -e wavy \
                               -e way-cooler \
                               -e wayfire \
                               -e wayhouse \
                               -e westeros \
                               -e westford \
                               -e weston)
        fi

    elif [[ $DISPLAY && $os != "Mac OS X" && $os != "macOS" && $os != FreeMiNT ]]; then
        # non-EWMH WMs.
        wm=$(ps "${ps_flags[@]}" | grep -m 1 -o \
                           -e "[s]owm" \
                           -e "[c]atwm" \
                           -e "[f]vwm" \
                           -e "[d]wm" \
                           -e "[2]bwm" \
                           -e "[m]onsterwm" \
                           -e "[t]inywm" \
                           -e "[x]11fs" \
                           -e "[x]monad")

        [[ -z $wm ]] && type -p xprop &>/dev/null && {
            id=$(xprop -root -notype _NET_SUPPORTING_WM_CHECK)
            id=${id##* }
            wm=$(xprop -id "$id" -notype -len 100 -f _NET_WM_NAME 8t)
            wm=${wm/*WM_NAME = }
            wm=${wm/\"}
            wm=${wm/\"*}
        }

    else
        case $os in
            "Mac OS X"|"macOS")
                ps_line=$(ps -e | grep -o \
                    -e "[S]pectacle" \
                    -e "[A]methyst" \
                    -e "[k]wm" \
                    -e "[c]hun[k]wm" \
                    -e "[y]abai" \
                    -e "[R]ectangle")

                case $ps_line in
                    *chunkwm*)   wm=chunkwm ;;
                    *kwm*)       wm=Kwm ;;
                    *yabai*)     wm=yabai ;;
                    *Amethyst*)  wm=Amethyst ;;
                    *Spectacle*) wm=Spectacle ;;
                    *Rectangle*) wm=Rectangle ;;
                    *)           wm="Quartz Compositor" ;;
                esac
            ;;

            Windows)
                wm=$(
                    tasklist |
                    grep -Fom 1 \
                         -e bugn \
                         -e Windawesome \
                         -e blackbox \
                         -e emerge \
                         -e litestep
                )

                [[ $wm == blackbox ]] &&
                    wm="bbLean (Blackbox)"

                wm=${wm:+$wm, }DWM.exe
            ;;

            FreeMiNT)
                freemint_wm=(/proc/*)

                case ${freemint_wm[*]} in
                    *xaaes* | *xaloader*) wm=XaAES ;;
                    *myaes*)              wm=MyAES ;;
                    *naes*)               wm=N.AES ;;
                    geneva)               wm=Geneva ;;
                    *)                    wm="Atari AES" ;;
                esac
            ;;
        esac
    fi

    # Rename window managers to their proper values.
    [[ $wm == *WINDOWMAKER* ]] && wm=wmaker
    [[ $wm == *GNOME*Shell* ]] && wm=Mutter

    wm_run=1
}

fifo="/tmp/rpcfifo0"
rm $fifo
mkfifo $fifo;
log="false"
if [ ! $# -eq 0 ]; then
    log="true"
fi

get_wm
lastmsg=;
distro=$(( lsb_release -ds || cat /etc/*release || uname -om ) 2>/dev/null | head -n1);
discordrunning="( [ \"\$(pgrep Discord)\" = \"\" ] && echo false ) || echo true"
cpuu="cat <(grep 'cpu ' /proc/stat) <(sleep 1 && grep 'cpu ' /proc/stat) | awk -v RS=\"\" '{print (\$13-\$2+\$15-\$4)*100/(\$13-\$2+\$15-\$4+\$16-\$5) }'"
memu="free -t | awk 'NR == 2 {printf \"%.2f\", \$3/\$2*100}'"
cpup=$(eval $cpuu)
memp=$(eval $memu)
while true
do
    newmsg="wm: $wm\\nwindow: $(xdotool getactivewindow getwindowclassname)\\ndistro: $(lsb_release -is)\\ndiscord: $(eval $discordrunning)"
    if [ "$newmsg" = "$lastmsg" ]; then
        sleep 1
    else

        lastmsg=$newmsg
        newmsg="${newmsg}\\ncpu: $cpup\\nmem: $memp"
        if [ $log = "true" ]; then
            echo -e "Sending message:\\n$newmsg\\n\\n"
        fi
        echo -e $newmsg > $fifo
        cpup=$(eval $cpuu)
        memp=$(eval $memu)

        sleep 1
    fi
done
