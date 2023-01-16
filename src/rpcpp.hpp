#include <iostream>
#include <csignal>
#include <thread>
#include <unistd.h>
#include <time.h>
#include <regex>
#include <fstream>
#include <filesystem>

// Discord RPC
#include "discord/discord.h"

// X11 libs
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

// variables
#define VERSION "2.1.0"

namespace
{
    volatile bool interrupted{false};
}
namespace fs = std::filesystem;
using namespace std;

int startTime;
Display *disp;
float mem = -1, cpu = -1;
string distro;
static int trapped_error_code = 0;
string wm;

vector<string> apps = {"blender", "chrome", "chromium", "discord", "dolphin", "firefox", "gimp", "hl2_linux", "hoi4", "konsole", "lutris", "st", "steam", "surf", "vscode", "worldbox", "xterm"}; // currently supported app icons on discord rpc (replace if you made your own discord application)
map<string, string> aliases = {
    {"vscodium", "vscode"}, {"code", "vscode"}, {"code - [a-z]+", "vscode"}, {"stardew valley", "stardewvalley"}, {"minecraft [a-z0-9.]+", "minecraft"}, {"lunar client [a-z0-9\\(\\)\\.\\-\\/]+", "minecraft"}, {"telegram(desktop)?", "telegram"}, {"terraria\\.bin\\.x86_64", "terraria"}, {"vivaldi(-stable)?", "vivaldi"}}; // for apps with different names
map<string, string> distros_lsb = {{"Arch|Artix", "archlinux"}, {"LinuxMint", "lmint"}, {"Gentoo", "gentoo"}, {"Ubuntu", "ubuntu"}, {"ManjaroLinux", "manjaro"}};                                                                                                                                                                // distro names in /etc/lsb_release
map<string, string> distros_os = {{"Arch Linux", "archlinux"}, {"Linux Mint", "lmint"}, {"Gentoo", "gentoo"}, {"Ubuntu", "ubuntu"}, {"Manjaro Linux", "manjaro"}};                                                                                                                                                               // same but in /etc/os-release (fallback)
string helpMsg = string(
                     "Usage:\n") +
                 " rpcpp [options]\n\n" +
                 "Options:\n" +
                 " -f, --ignore-discord   don't check for discord on start\n" +
                 " --debug                print debug messages\n" +
                 " --usage-sleep=5000     sleep time in milliseconds between updating cpu and ram usages\n" +
                 " --update-sleep=100     sleep time in milliseconds between updating the rich presence and focused application\n\n" +
                 " -h, --help             display this help and exit\n" +
                 " -v, --version          output version number and exit";

// regular expressions

regex memavailr("MemAvailable: +(\\d+) kB");
regex memtotalr("MemTotal: +(\\d+) kB");
regex processRegex("\\/proc\\/\\d+\\/cmdline");

vector<pair<regex, string>> aliases_regex = {};
vector<pair<regex, string>> distros_lsb_regex = {};
vector<pair<regex, string>> distros_os_regex = {};

struct DiscordState
{
    discord::User currentUser;

    unique_ptr<discord::Core> core;
};

struct DistroAsset
{
    string image;
    string text;
};

struct WindowAsset
{
    string image;
    string text;
};

struct StartOptions
{
    bool ignoreDiscord = false;
    bool debug = false;
    int usageSleep = 5000;
    int updateSleep = 300;
    bool printHelp = false;
    bool printVersion = false;
};

StartOptions options;

// local imports

#include "logging.hpp"
#include "wm.hpp"

// methods

static int error_handler(Display *display, XErrorEvent *error)
{
    trapped_error_code = error->error_code;
    return 0;
}

string lower(string s)
{
    transform(s.begin(), s.end(), s.begin(),
              [](unsigned char c)
              { return tolower(c); });
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

float getRAM()
{
    ifstream meminfo;
    meminfo.open("/proc/meminfo");

    long total = 0;
    long available = 0;

    smatch matcher;
    string line;

    while (getline(meminfo, line))
    {
        if (regex_search(line, matcher, memavailr))
        {
            available = stoi(matcher[1]);
        }
        else if (regex_search(line, matcher, memtotalr))
        {
            total = stoi(matcher[1]);
        }
    }

    meminfo.close();

    if (total == 0)
    {
        return 0;
    }
    return (float)(total - available) / total * 100;
}

void setActivity(DiscordState &state, string details, string sstate, string smallimage, string smallimagetext, string largeimage, string largeimagetext, long uptime, discord::ActivityType type)
{
    time_t now = time(nullptr);
    discord::Activity activity{};
    activity.SetDetails(details.c_str());
    activity.SetState(sstate.c_str());
    activity.GetAssets().SetSmallImage(smallimage.c_str());
    activity.GetAssets().SetSmallText(smallimagetext.c_str());
    activity.GetAssets().SetLargeImage(largeimage.c_str());
    activity.GetAssets().SetLargeText(largeimagetext.c_str());
    activity.GetTimestamps().SetStart(uptime);
    activity.SetType(type);

    state.core->ActivityManager().UpdateActivity(activity, [](discord::Result result)
                                                 { if(options.debug) log(string((result == discord::Result::Ok) ? "Succeeded" : "Failed")  + " updating activity!", LogType::DEBUG); });
}

string getActiveWindowClassName(Display *disp)
{
    Window root = XDefaultRootWindow(disp);

    char *prop = get_property(disp, root, XA_WINDOW, "_NET_ACTIVE_WINDOW");

    if (prop == NULL)
    {
        return "";
    }

    XClassHint hint;
    int hintStatus = XGetClassHint(disp, *((Window *)prop), &hint);

    if (hintStatus == 0)
    {
        return "";
    }

    XFree(hint.res_name);
    string s(hint.res_class);
    XFree(hint.res_class);

    return s;
}

static unsigned long long lastTotalUser, lastTotalUserLow, lastTotalSys, lastTotalIdle;

void getLast()
{
    FILE *file = fopen("/proc/stat", "r");
    fscanf(file, "cpu %llu %llu %llu %llu", &lastTotalUser, &lastTotalUserLow,
           &lastTotalSys, &lastTotalIdle);
    fclose(file);
}

double getCPU()
{
    getLast();
    sleep(1);
    double percent;
    FILE *file;
    unsigned long long totalUser, totalUserLow, totalSys, totalIdle, total;

    file = fopen("/proc/stat", "r");
    fscanf(file, "cpu %llu %llu %llu %llu", &totalUser, &totalUserLow,
           &totalSys, &totalIdle);
    fclose(file);

    if (totalUser < lastTotalUser || totalUserLow < lastTotalUserLow ||
        totalSys < lastTotalSys || totalIdle < lastTotalIdle)
    {
        // Overflow detection. Just skip this value.
        percent = -1.0;
    }
    else
    {
        total = (totalUser - lastTotalUser) + (totalUserLow - lastTotalUserLow) +
                (totalSys - lastTotalSys);
        percent = total;
        total += (totalIdle - lastTotalIdle);
        percent /= total;
        percent *= 100;
    }

    lastTotalUser = totalUser;
    lastTotalUserLow = totalUserLow;
    lastTotalSys = totalSys;
    lastTotalIdle = totalIdle;

    return percent;
}

bool processRunning(string name, bool ignoreCase = true)
{

    string strReg = "\\/" + name + " ?";
    regex nameRegex;
    smatch progmatcher;

    if (ignoreCase)
        nameRegex = regex(strReg, regex::icase);

    else
        nameRegex = regex(strReg);

    string procs;
    smatch isProcessMatcher;

    std::string path = "/proc";
    for (const auto &entry : fs::directory_iterator(path))
    {
        if (fs::is_directory(entry.path()))
        {
            for (const auto &entry2 : fs::directory_iterator(entry.path()))
            {
                string path = entry2.path();
                if (regex_search(path, isProcessMatcher, processRegex))
                {
                    ifstream s;
                    s.open(entry2.path());
                    string line;
                    while (getline(s, line))
                    {
                        if (regex_search(line, progmatcher, nameRegex))
                        {
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

bool in_array(const string &value, const vector<string> &array)
{
    return find(array.begin(), array.end(), value) != array.end();
}

void parseArgs(int argc, char **argv)
{
    smatch matcher;
    regex usageRegex("--usage-sleep=(\\d+)");
    regex updateRegex("--update-sleep=(\\d+)");

    for (int i = 0; i < argc; i++)
    {
        string carg = string(argv[i]);
        if (carg == "-h" || carg == "--help")
        {
            options.printHelp = true;
        }
        if (carg == "-v" || carg == "--version")
        {
            options.printVersion = true;
        }
        if (carg == "-f" || carg == "--ignore-discord")
        {
            options.ignoreDiscord = true;
        }
        if (carg == "--debug")
        {
            options.debug = true;
        }
        if (regex_search(carg, matcher, usageRegex))
        {
            options.usageSleep = stoi(matcher[1]);
        }
        if (regex_search(carg, matcher, updateRegex))
        {
            options.updateSleep = stoi(matcher[1]);
        }
    }
}

string getDistro()
{
    string distro = "";
    string line;
    ifstream release;
    regex distroreg;
    smatch distromatcher;
    if (fs::exists("/etc/lsb-release"))
    {
        distroreg = regex("DISTRIB_ID=\"?([a-zA-Z0-9 ]+)\"?");
        release.open("/etc/lsb-release");
    }
    else if (fs::exists("/etc/os-release"))
    {
        distroreg = regex("NAME=\"?([a-zA-Z0-9 ]+)\"?");
        release.open("/etc/os-release");
    }
    else
    {
        log("Warning: Neither /etc/lsb-release nor /etc/os-release was found. Please install lsb_release or ask your distribution's developer to support os-release.", LogType::DEBUG);
        return distro;
    }
    while (getline(release, line))
    {
        if (regex_search(line, distromatcher, distroreg))
        {
            distro = distromatcher[1];
            break;
        }
    }
    return distro;
}

WindowAsset getWindowAsset(string w)
{
    WindowAsset window{};
    window.text = w;
    if (w == "")
    {
        window.image = "";
        return window;
    }
    window.image = "file";
    w = lower(w);

    if (in_array(w, apps))
    {
        window.image = w;
    }
    else
    {
        for (const auto &kv : aliases_regex)
        {
            regex r = kv.first;
            smatch m;
            if (regex_match(w, m, r))
            {
                window.image = kv.second;
                break;
            }
        }
    }

    return window;
}

DistroAsset getDistroAsset(string d)
{
    DistroAsset dist{};
    dist.text = d + " / RPC++ " + VERSION;
    dist.image = "tux";

    for (const auto &kv : distros_lsb_regex)
    {
        regex r = kv.first;
        smatch m;
        if (regex_match(d, m, r))
        {
            dist.image = kv.second;
            break;
        }
    }
    if (dist.image == "tux")
    {
        for (const auto &kv : distros_os_regex)
        {
            regex r = kv.first;
            smatch m;
            if (regex_match(d, m, r))
            {
                dist.image = kv.second;
                break;
            }
        }
    }

    return dist;
}

/**
 * @brief Compile strings to regular expressions
 */
void compileRegexes(map<string, string> *from, vector<pair<regex, string>> *to, bool ignoreCase)
{

    for (const auto &kv : *from)
    {
        const regex r = regex(kv.first);
        to->push_back({r, kv.second});
    }
}

/**
 * @brief Compile all strings to regular expressions
 */
void compileAllRegexes()
{
    compileRegexes(&aliases, &aliases_regex, false);
    compileRegexes(&distros_lsb, &distros_lsb_regex, true);
    compileRegexes(&distros_os, &distros_os_regex, true);
}
