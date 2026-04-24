# SwarmSim: Neuroevolution Ecosystem

**SwarmSim** is a multithreaded 2D artificial life simulation built in C++ using SFML. It explores neuroevolution and population dynamics by dropping two competing species—**Predators** and **Prey**—into a closed ecosystem. 

Agents are driven by **Feed-Forward Neural Networks (Perceptrons)**. Over successive generations, a Genetic Algorithm is used to "breed" the most successful agents, allowing complex behaviors to emerge.

## ✨ Key Features
* **Neuroevolution:** Agents possess simple brains - 3 layer perceptrons. The best-performing agents (based on energy gathered and time lived) pass their mutated neural weights to the next generation.
* **Realistic Physics & Locomotion:** Agents use **Differential Steering** (`Thrust` and `Turn Intent`). Moving and turning burn energy, punishing erratic behavior.
* **Dynamic Ecosystem Dynamics:** * **Prey** must migrate between dynamically growing islands of grass to avoid starving, balancing foraging with predator evasion.
  * **Predators** must hunt prey to survive. They possess high sprint speeds but suffer massive energy drains.
  * **Digestion Cooldowns** prevent "spawn camping" of resources and population explotions.
* **Performance Optimization:** Uses **Spatial Partitioning (Lattice Grids)** to optimize vision, collision checks, and grass detection; it implements **Multithreading** to simulate batch generations concurrently across multiple CPU cores.

---

## 🧠 How It Works

### The Brain
Each agent perceives the world through a set of normalized sensory inputs:
1. **Memory:** Previous Thrust, Previous Turn
2. **Self-Knowledge:** Current Speed, Fullness (Digestion status)
3. **Food Sense (Prey):** Distance to food, X/Y direction, Closing/Tangential Velocity - also given to predators to give them knowledge of grass
4. **Enemy Sense:** Distance to closest enemy, X/Y direction, Closing/Tangential Velocity

The network processes these inputs through a hidden layer and outputs two values via a `tanh` activation function:
* `Thrust Intent` (Clamped `0.0` to `1.0`): Forward acceleration.
* `Turn Intent` (`-1.0` to `1.0`): Steering rotation (yaw).

### The Genetic Algorithm
At the end of each generation, dead agents are sorted by their **Fitness Score**.
* **Prey Fitness:** Scaled by time lived and total grass eaten.
* **Predator Fitness:** Scaled strictly by the number of prey killed and eaten.

The top percentage of agents are saved in a "Hall of Fame." The next generation is spawned by cloning and mutating the weights of these elite agents, occasionally injecting random brains to maintain genetic diversity. HOF is used to prevent the Red Queen phenomenon - avoiding cyclic behaviour across generations.

---

## ⚙️ Installation & Building

This project uses CMake to automatically download and link SFML 2.6.1 at compile-time.

### Prerequisites
* **C++17** (or higher) compatible compiler
* **CMake** (3.14 or higher)

### Linux (Debian/Ubuntu)
When building SFML from source via CMake, you need its underlying graphical and audio dependencies:

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

## 🛠️ Configuration & Tuning
The entire simulation is highly parametric. You can alter the rules by editing `Core/Config.h`.


**Key areas to tweak:**
* **`World Settings`:** Adjust map size (`NUM_CELLE_X`, `NUM_CELLE_Y`), starting population, and grass growth rates.
* **`Agent Settings`:** Tweak max speed, field-of-view (FOV), sensing range, and metabolism costs. 
* **`Learning Settings`:** Alter the mutation rates (`STARTING_MUTATION_RATE`), mutation strength, and decay rates.
* **`Effort Multipliers`:** Control the energy drain of sprinting vs. resting to balance the Predator/Prey relationship.

### Debug Controls
For debugging purposes, toggle these constants:
* `SHOW_GRID = true` (Displays the environment grid)
* `SHOW_FOOD_LATTICE = true` (Displays spatial partitioning chunks)
* `SHOW_FOV = true` (Draws vision cones and proximity radii around agents)