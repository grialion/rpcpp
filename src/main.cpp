#include "rpcpp.hpp"

void *updateRPC(void *ptr)
{
    string windowName, lastWindow;
    WindowAsset windowAsset;
    DistroAsset distroAsset;
    DiscordState *state = (struct DiscordState *)ptr;

    log("Waiting for usages to load...", LogType::DEBUG);

    // wait for usages to load
    while (cpu == -1 || mem == -1)
    {
        usleep(1000);
    }

    log("Starting RPC loop.", LogType::DEBUG);
    distroAsset = getDistroAsset(distro);

    while (true)
    {
        string cpupercent = to_string((long)cpu);
        string rampercent = to_string((long)mem);

        usleep(options.updateSleep * 1000);

        if (!options.noSmallImage)
        {
            try
            {
                windowName = getActiveWindowClassName(disp);
            }
            catch (exception ex)
            {
                log(ex.what(), LogType::ERROR);
                continue;
            }

            if (windowName != lastWindow)
            {
                windowAsset = getWindowAsset(windowName);
                lastWindow = windowName;
            }
        }

        setActivity(*state, string("CPU: " + cpupercent + "% | RAM: " + rampercent + "%"), "WM: " + wm, windowAsset.image, windowAsset.text, distroAsset.image, distroAsset.text, startTime, discord::ActivityType::Playing);
    }
}

void *updateUsage(void *ptr)
{
    distro = getDistro();
    log("Distro: " + distro, LogType::DEBUG);

    startTime = time(0) - ms_uptime();
    wm = string(wm_info(disp));
    log("WM: " + wm, LogType::DEBUG);

    while (true)
    {
        mem = getRAM();
        cpu = getCPU();
        sleep(options.usageSleep / 1000.0);
    }
}

int main(int argc, char **argv)
{
    parseArgs(argc, argv);

    if (options.printHelp)
    {
        cout << helpMsg << endl;
        exit(0);
    }
    if (options.printVersion)
    {
        cout << "RPC++ version " << VERSION << endl;
        exit(0);
    }

    int waitedTime = 0;
    while (!processRunning("discord") && !options.ignoreDiscord)
    {
        if (waitedTime > 60)
        {
            log(string("Discord is not running for ") + to_string(waitedTime) + " seconds. Maybe ignore Discord check with --ignore-discord or -f?", LogType::INFO);
        }
        log("Waiting for Discord...", LogType::INFO);
        waitedTime += 5;
        sleep(5);
    }

    disp = XOpenDisplay(NULL);

    if (!disp)
    {
        cout << "Can't open display" << endl;
        return -1;
    }

    static int (*old_error_handler)(Display *, XErrorEvent *);
    trapped_error_code = 0;
    old_error_handler = XSetErrorHandler(error_handler);

    // Compile all regexes
    compileAllRegexes();

    pthread_t updateThread;
    pthread_t usageThread;
    pthread_create(&usageThread, 0, updateUsage, 0);
    log("Created usage thread", LogType::DEBUG);

    DiscordState state{};

    discord::Core *core{};
    auto result = discord::Core::Create(934099338374824007, DiscordCreateFlags_Default, &core); // change with your own app's id if you made one
    state.core.reset(core);
    if (!state.core)
    {
        cout << "Failed to instantiate discord core! (err " << static_cast<int>(result)
             << ")\n";
        exit(-1);
    }

    if (options.debug)
    {
        state.core->SetLogHook(
            discord::LogLevel::Debug, [](discord::LogLevel level, const char *message)
            { cerr << "Log(" << static_cast<uint32_t>(level) << "): " << message << "\n"; });
    }

    pthread_create(&updateThread, 0, updateRPC, ((void *)&state));
    log("Threads started.", LogType::DEBUG);
    log("Xorg version " + to_string(XProtocolVersion(disp)), LogType::DEBUG); // this is kinda dumb to do since it shouldn't be anything else other than 11, but whatever
    log("Connected to Discord.", LogType::INFO);

    signal(SIGINT, [](int)
           { interrupted = true; });

    do
    {
        state.core->RunCallbacks();

        this_thread::sleep_for(chrono::milliseconds(16));
    } while (!interrupted);

    cout << "Exiting..." << endl;

    XCloseDisplay(disp);

    pthread_kill(updateThread, 9);
    pthread_kill(usageThread, 9);

    return 0;
}
