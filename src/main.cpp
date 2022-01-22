#include "rpcpp.hpp"

Window getWindow(string w)
{
    Window window{};
    window.text = w;
    window.image = "file";
    w = lower(w);

    if (in_array(w, apps))
    {
        window.image = w;
    }
    else
    {
        for (const auto &kv : aliases)
        {
            regex r = regex(kv.first);
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

Dist getDistro(string d)
{
    Dist dist{};
    dist.text = d;
    dist.image = "tux";

    for (const auto &kv : distros)
    {
        regex r = regex(kv.first);
        smatch m;
        if (regex_match(d, m, r))
        {
            dist.image = kv.second;
            break;
        }
    }

    return dist;
}

void *updatefifo(void *ptr)
{
    DiscordState *state = (struct DiscordState *)ptr;

    std::ifstream fifo;
    fifo.open("/tmp/rpcfifo0", ifstream::in);
    if (!fifo.is_open())
    {
        std::cout << "Can't open the fifo file. Perhaps the fetch.sh isn't running yet?" << std ::endl;
        interrupted = true;
        exit(-1);
    }
    std::string line;
    while (true)
    {
        string lines = "";
        while (std::getline(fifo, line))
        {
            lines.append("\n").append(line);
        }
        if (fifo.eof())
        {
            fifo.clear();
        }
        if (lines.length() > 0)
        {
            cout << lines << endl;
            regex wm = regex("wm: (\\w+)");
            regex window("window: ([^\n]+)");
            regex distro("distro: ([^\n]+)");
            regex cpu("cpu: ([^\n]+)");
            regex mem("mem: ([^\n]+)");
            smatch windowmatch;
            smatch wmmatch;
            smatch distromatch;
            smatch cpum;
            smatch memm;

            if (regex_search(lines, windowmatch, window) &&
                regex_search(lines, distromatch, distro) &&
                regex_search(lines, wmmatch, wm) &&
                regex_search(lines, cpum, cpu) &&
                regex_search(lines, memm, mem))
            {
                Dist d = getDistro(distromatch[1]);
                Window w = getWindow(windowmatch[1]);
                string wms = "WM: " + string(wmmatch[1]);
                string cpus = cpum[1];
                string mems = memm[1];
                int cpui = round(atof(cpus.c_str()));
                int memi = round(atof(mems.c_str()));

                setActivity((*state), "CPU: " + to_string(cpui) + "%" + " | RAM: " + to_string(memi) + "%", wms, w.image, w.text, d.image, d.text, startTime, discord::ActivityType::Playing);
            }
        }
        sleep(0.1);
    }
    interrupted = true;
    return 0;
}

int main()
{
    startTime = ms_uptime();
    DiscordState state{};

    discord::Core *core{};
    auto result = discord::Core::Create(934099338374824007, DiscordCreateFlags_Default, &core);
    state.core.reset(core);
    if (!state.core)
    {
        std::cout << "Failed to instantiate discord core! (err " << static_cast<int>(result)
                  << ")\n";
        std::exit(-1);
    }
    state.core->SetLogHook(
        discord::LogLevel::Debug, [](discord::LogLevel level, const char *message)
        { std::cerr << "Log(" << static_cast<uint32_t>(level) << "): " << message << "\n"; });

    pthread_t updatethread;

    pthread_create(&updatethread, 0, updatefifo, ((void *)&state));
    std::cout << "Fifo thread started." << std::endl;

    std::signal(SIGINT, [](int)
                { interrupted = true; });

    do
    {
        state.core->RunCallbacks();

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    } while (!interrupted);

    pthread_kill(updatethread, 9);

    return 0;
}