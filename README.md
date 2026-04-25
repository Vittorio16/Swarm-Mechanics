# SwarmSim: Neuroevolution Ecosystem

**SwarmSim** is a multithreaded 2D artificial life simulation built in C++ using SFML. It explores neuroevolution and population dynamics by dropping two competing species—**Predators** and **Prey**—into a closed ecosystem. 

Agents are driven by **Feed-Forward Neural Networks (Perceptrons)**. Over successive generations, a Genetic Algorithm is used to "breed" the most successful agents.

* **Neuroevolution:** Agents possess simple brains - 3 layer perceptrons. The best-performing agents (based on energy gathered and time lived) pass their mutated neural weights to the next generation.
* **Digestion Cooldowns** prevent "spawn camping" of resources and population explotions.
* **Performance Optimization:** Uses **Spatial Partitioning (Lattice Grids)** to optimize vision, collision checks, and grass detection; it implements **Multithreading** to simulate batch generations concurrently across multiple CPU cores.

---

### Neural Network Parameters
Each agent perceives the world through a set of sensory inputs:
1. **Memory:** Previous Thrust, Previous Turn
2. **Self-Knowledge:** Current Speed, Fullness (Digestion status)
3. **Food Sense (Prey):** Distance to food, X/Y direction, Closing/Tangential Velocity - also given to predators to give them knowledge of grass
4. **Enemy Sense:** Distance to closest enemy, X/Y direction, Closing/Tangential Velocity

The network outputs two values:
* `Thrust Intent` (Clamped `0.0` to `1.0`): Forward acceleration.
* `Turn Intent` (`-1.0` to `1.0`): Steering rotation (yaw).

### Fitness Function
At the end of each generation, dead agents are sorted by their **Fitness Score**.
* **Prey Fitness:** Scaled by time lived and total grass eaten.
* **Predator Fitness:** Scaled strictly by the number of prey killed and eaten.

The top percentage of agents are saved in a "Hall of Fame." The next generation is spawned by cloning and mutating the weights of these elite agents, occasionally injecting random brains to maintain genetic diversity. HOF is used to prevent the Red Queen phenomenon - avoiding cyclic behaviour across generations.

---

## Installation

### Prerequisites
* **C++17** (or higher)
* **CMake**

You need SFML's graphical and audio dependencies:

```bash
# Install SFML build dependencies
sudo apt-get update
sudo apt-get install libfreetype6-dev libx11-dev libxrandr-dev libudev-dev libopengl-dev libflac-dev libogg-dev libvorbis-dev libopenal-dev libpthread-stubs0-dev

# Clone and build
git clone [https://github.com/yourusername/SwarmSim.git](https://github.com/yourusername/SwarmSim.git)
cd SwarmSim
mkdir build && cd build
cmake --build .

# Run the simulation
./SwarmSim

```

---

## Usage

### Training and Rendering Modes
By default, the simulation starts in Real-Time rendering mode. You can press the **`R` key** at any time to toggle between rendering and training:
* **Real-Time Mode:** Capped to a set framerate. Renders the world, agents, and debug overlays so you can observe the behaviors visually.
* **Training Mode:** Headless mode. Rendering is disabled, the framerate is uncapped, and the simulation dynamically batches ticks across CPU cores to evolve generations faster.

### Running a Pre-Trained Population
The simulation automatically saves the best weights, Hall of Fame, and elite pools to the `logs/` directory every few generations. To watch a previously trained generation:
1. Open `Core/Config.h` and ensure `REPLAY_MODE_ENABLED = true;`.
2. Open `main.cpp` and locate the `simManager.loadPreTrainedBrains(...)` function block.
3. Update the starting generation integer and the string file paths to point to your desired `.txt` log files (e.g., `../logs/weights_prey_gen_50.txt`).
4. Recompile and run

---

## Configuration & Tuning
You can alter the simulation's parameters by editing `Core/Config.h`.

**Key areas to tweak:**
* **`World Settings`:** Adjust map size, starting population, and food amount.
* **`Agent Settings`:** Tweak max speed, field-of-view, sensing range, and metabolism costs. 
* **`Learning Settings`:** Alter the mutation rates, mutation strength, and decay rates.
* **`Effort Multipliers`:** Control the energy drain of sprinting vs. resting

### Debug Controls
For debugging purposes, toggle these constants:
* `SHOW_GRID = true` (Displays the environment grid)
* `SHOW_FOOD_LATTICE = true` (Displays spatial partitioning chunks)
* `SHOW_FOV = true` (Draws vision cones and proximity radii around agents)