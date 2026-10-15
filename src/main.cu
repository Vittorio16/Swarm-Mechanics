#ifdef USE_SFML_GUI
    #include <SFML/Graphics.hpp>
#endif

#include <thread>
#include <iostream>
#include <string>
#include <random>
#include "Core/Config.h"
#include "Core/GpuConfig.h"
#include "Core/RunConfig.h"
#include "SimulationManager.h"

using namespace std;

static RunConfig parseArgs(int argc, char** argv) {
    RunConfig c;
    for (int i = 1; i < argc; i++) {
        string k = argv[i];
        if (k == "--json") { c.emitJson = true; continue; }
        if (i + 1 >= argc) { cerr << "missing value for " << k << endl; break; }
        string v = argv[++i];

        if      (k == "--prey")         c.numPrey        = stoi(v);
        else if (k == "--predators")    c.numPredators   = stoi(v);
        else if (k == "--worlds")       c.numWorlds      = stoi(v);
        else if (k == "--grid-mult")    c.gridMultiplier = stoi(v);
        else if (k == "--grass-radius") c.grassRadius    = stof(v);
        else if (k == "--pred-radius")  c.predViewRadius = stof(v);
        else if (k == "--gen-duration") c.genDuration    = stof(v);
        else if (k == "--seed")         c.seed           = stoull(v);
        else if (k == "--generations")  c.maxGenerations = stoi(v);
        else if (k == "--max-ticks")    c.maxTicks       = stoull(v);
        else if (k == "--tag")          c.tag            = v;
        else cerr << "unknown option: " << k << endl;
    }
    return c;
}

int main(int argc, char** argv) {
    RunConfig cfg = parseArgs(argc, argv);

    GpuConfig::init(cfg.gridMultiplier);

    uint64_t runSeed = cfg.seed ? cfg.seed : (uint64_t)std::random_device{}();

    int numWorlds = cfg.numWorlds;
    if (numWorlds <= 0) {
        int cores = std::thread::hardware_concurrency();
        if (cores == 0) cores = FALLBACK_CORE_NUMBER;
        numWorlds = cores - 1;
    }
    if (numWorlds < 1) numWorlds = 1;

    if (!cfg.emitJson) {
        cout << "GPU: " << GpuConfig::deviceName
             << " | SMs: " << GpuConfig::smCount
             << " | cma: " << GpuConfig::concurrentManagedAccess
             << " | grid: " << GpuConfig::persistentGrid << endl;
        cout << "Run seed: " << runSeed << " | worlds: " << numWorlds << endl;
    }

    SimulationManager simManager(numWorlds, runSeed, cfg);
    simManager.emitRunHeader();

    if (REPLAY_MODE_ENABLED) {
        simManager.loadPreTrainedBrains(51,
            "../elite_logs/weights_prey_gen_51.txt",
            "../elite_logs/weights_pred_gen_51.txt",
            "../elite_logs/hof_prey_gen_51.txt",
            "../elite_logs/hof_pred_gen_51.txt",
            "../elite_logs/elite_prey_gen_51.txt",
            "../elite_logs/elite_pred_gen_51.txt");
    }

#ifdef USE_SFML_GUI
    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Swarm Evolution");
    bool renderingEnabled = true;
    sf::Clock clock;
    window.setFramerateLimit(MAX_FPS);

    while (window.isOpen() && !simManager.finished()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::Resized) {
                sf::FloatRect visibleArea(0, 0, event.size.width, event.size.height);
                window.setView(sf::View(visibleArea));
                simManager.resizeTexture(event.size.width, event.size.height);
            }
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R) {
                renderingEnabled = !renderingEnabled;
            }
        }

        float dt;
        if (renderingEnabled) {
            dt = clock.restart().asSeconds();
            if (dt > MINIMUM_REAL_TIME_STEP) dt = MINIMUM_REAL_TIME_STEP;
        } else {
            dt = DEBUGGING_ENABLED ? 0 : FIXED_TIME_STEP;
            clock.restart();
        }

        simManager.update(dt, renderingEnabled);

        if (renderingEnabled) {
            window.clear(sf::Color(30, 30, 30));
            simManager.draw(window);
            window.display();
        }
    }
#else
    if (!cfg.emitJson) cout << "Running simulation in headless mode..." << endl;
    
    while (!simManager.finished()) {
        simManager.update(FIXED_TIME_STEP, false);
    }
#endif

    return 0;
}
