#include <iostream>
#include <array>
#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <time.h>
#include <regex>
#include <fstream>
#include <math.h>
#include "discord/discord.h"

struct DiscordState
{
    discord::User currentUser;

    std::unique_ptr<discord::Core> core;
};

struct Dist
{
    std::string image;
    std::string text;
};

struct Window
{
    std::string image;
    std::string text;
};

std::string lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c)
                   { return std::tolower(c); });
    return s;
}

double ms_uptime(void)
{
    FILE *in = fopen("/proc/uptime", "r");
    double retval = 0;
    char tmp[256] = {0x0};
    if (in != NULL)
    {
        fgets(tmp, sizeof(tmp), in);
        retval = atof(tmp);
        fclose(in);
    }
    return retval;
}

namespace
{
    volatile bool interrupted{false};
}
using namespace std;
int startTime;

void setActivity(DiscordState &state, string details, string sstate, string smallimage, string smallimagetext, string largeimage, string largeimagetext, long uptime, discord::ActivityType type)
{
    time_t now = time(nullptr);
    long ts = now - uptime;
    discord::Activity activity{};
    activity.SetDetails(details.c_str());
    activity.SetState(sstate.c_str());
    activity.GetAssets().SetSmallImage(smallimage.c_str());
    activity.GetAssets().SetSmallText(smallimagetext.c_str());
    activity.GetAssets().SetLargeImage(largeimage.c_str());
    activity.GetAssets().SetLargeText(largeimagetext.c_str());
    activity.GetTimestamps().SetStart(ts);
    activity.SetType(type);
    state.core->ActivityManager().UpdateActivity(activity, [](discord::Result result)
                                                 { std::cout << ((result == discord::Result::Ok) ? "Succeeded" : "Failed")
                                                             << " updating activity!\n"; });
}

bool in_array(const std::string &value, const std::vector<std::string> &array)
{
    return std::find(array.begin(), array.end(), value) != array.end();
}

std::vector<std::string> apps = {"blender", "chrome", "discord", "firefox", "gimp", "st", "surf", "vscode"}; // currently supported app icons on discord rpc (replace if you made your own discord application)
std::map<std::string, std::string> aliases = { {"chromium", "chrome"}, {"vscodium", "vscode"}, {"code", "vscode"}, {"code - [a-z]+", "vscode"} }; // for apps with different names
std::map<std::string, std::string> distros = {{"Arch|Artix", "archlinux"}, {"Mint", "lmint"}, {"Gentoo", "gentoo"}, {"Ubuntu", "ubuntu"}};
