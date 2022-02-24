#include "rpcpp.hpp"
#include "wm.hpp"

void *updateRPC(void *ptr)
{
    DiscordState *state = (struct DiscordState *)ptr;

    string lastWindow;

    debug("Waiting for usages to load...");
    sleep(3); // wait for usages to load
    debug("Starting RPC loop.");
    string windowName;

    while (true)
    {
        usleep(options.updateSleep * 1000);

        try
        {
            windowName = getActiveWindowClassName(disp);
        }
        catch (exception ex)
        {
            debug(string("Error: ") + ex.what());
            continue;
        }

        if (windowName != lastWindow)
        {
            lastWindow = windowName;

            DistroAsset distroAsset = getDistroAsset(distro);
            WindowAsset windowAsset = getWindowAsset(windowName);
            string cpupercent = to_string((long)cpu);
            string rampercent = to_string((long)mem);

            setActivity(*state, string("CPU: " + cpupercent + "% | RAM: " + rampercent + "%"), "WM: " + wm, windowAsset.image, windowAsset.text, distroAsset.image, distroAsset.text, startTime, discord::ActivityType::Playing);
        }
    }
}

void *updateUsage(void *ptr)
{
    distro = getDistro();

    startTime = time(0) - ms_uptime();
    wm = string(wm_info(disp));
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
            cout << "Discord is not running for " << waitedTime << " seconds. Maybe ignore Discord check with --ignore-discord or -f? ";
        }
        cout << "Waiting for Discord..." << endl;
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

    pthread_t updateThread;
    pthread_t usageThread;
    pthread_create(&usageThread, 0, updateUsage, 0);
    debug("Created usage thread");

    DiscordState state{};

    discord::Core *core{};
    auto result = discord::Core::Create(934099338374824007, DiscordCreateFlags_Default, &core);
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
    debug("Threads started.");
    cout << "Connected to Discord." << endl;

    signal(SIGINT, [](int)
           { interrupted = true; });

    do
    {
        state.core->RunCallbacks();

        this_thread::sleep_for(chrono::milliseconds(16));
    } while (!interrupted);

    cout << "Exiting..." << endl;

    pthread_kill(updateThread, 9);
    pthread_kill(usageThread, 9);

    return 0;
}