## 5. Simulation and learning design

### 5.1 Your two selection mechanisms are fighting each other — **the most consequential issue**

You have *both* continuous Darwinian reproduction (energy > threshold → child with a mutated brain) *and* a generational GA (fitness ranking → elite pool → reset → reseed). They disagree:

- `childCount` appears in **neither** fitness function. So reproducing costs 50 energy, shortens `timeLived`, and yields exactly zero fitness credit. **Reproduction is actively selected against.** The GA will systematically favour agents that never breed, which means your Darwinian layer only contributes noise and population churn.
- Conversely, an agent that reproduces prolifically floods the population with its lineage, and then the generational reset discards all of that in favour of a hand-designed fitness score.

Pick one, or reconcile them:

- **Pure Darwinian** (my recommendation for this codebase). Delete generations, delete the fitness function, delete `resetSimulation`. Selection is just "who has surviving offspring". No fitness function to design and mis-tune, no host-side harvest, no reallocation, and the whole thing becomes 100% GPU-resident and runs continuously. The graveyard becomes pure logging. This also plays perfectly with §3.2/§3.3.
- **Pure GA.** Remove in-sim reproduction, fix the population per generation, evaluate at the end. Easier to run controlled experiments and to compare runs.
- **Reconcile.** Keep both but make fitness `= childCount` (possibly plus a small survival term). Then the two layers agree, since offspring count *is* Darwinian fitness.

### 5.2 Standing still is the optimal prey strategy

Do the arithmetic on your current constants:

| | idle | full thrust |
|---|---|---|
| prey drain | `0.05/s` | `0.05 + 1.0*5*0.4 = 2.05/s` |
| survival from 66.7 energy | ~1330 s | ~33 s |

A generation is 60–180 s. An idle prey survives the entire generation and scores `timeLived ≈ 60`. A foraging prey burns out in 33 s unless it eats, and each grass is worth only `10 * PREY_ENERGY_FITNESS_MULTIPLIER = 20` fitness. So an idle prey scores 60 for doing nothing, and a forager needs 3+ grass just to break even while also risking predators. You noticed this (the comment "Rewards gaining energy, to discourage standstill") but 2× isn't nearly enough.

Two fixes, and I'd do both:

- **Raise `METABOLISM_COST` a lot and lower `MAX_EFFORT_COST`.** Effort is currently 40× metabolism for prey. Something like 5–10× makes movement a real but affordable investment, and makes idling genuinely lethal. This is the single highest-leverage balance change you can make.
- **Make fitness energy-dominated.** `fitness = energyGained * W + timeLived * small`, or use foraging *rate* (`energyGained / timeLived`), or — best — switch to offspring count per §5.1, which makes the whole question moot because you can't have offspring without a large energy surplus.

### 5.3 Food teleports, so there's nothing spatial to learn

`foodToSpawn` is incremented once per bite and `growKernel` respawns exactly that many cells at **uniformly random positions across the whole map**. Total food is conserved at `CONSTANT_FOOD_AMOUNT = 500` cells, but each eaten cell reappears somewhere completely unrelated. Patches never form and never persist, so there is no gradient to follow, no patch to exploit, no reason to evolve area-restricted search or any spatial memory. You've given agents a 50-unit food sensor and then made the food landscape white noise.

Also: 500 filled cells out of 960×540 = 518,400 is a **0.1% density**. With `MAX_FOOD` clamping commented out in `growKernel`, cells are binary 0-or-10 rather than a continuous field.

Replace the retry-based spawner with **local logistic growth plus diffusion**, one thread per cell:

```
food[c] += r * food[c] * (1 - food[c]/MAX_FOOD) * dt   // logistic regrowth
food[c] += D * (mean(neighbours) - food[c]) * dt        // spreads into empty cells
```

Seed a handful of patches at generation start. This gives you:

- persistent, depletable patches → real spatial structure worth learning
- genuine resource competition (eating locally depletes the neighbourhood, so crowding hurts)
- prey-density feedback on food, which in turn feeds back on predators — actual population dynamics instead of a fixed prey energy budget
- it deletes the `atomicCAS`-on-float hack and the bounded-retry loop entirely
- 518,400 independent cells is *perfect* GPU work, unlike the current spawner which launches 100k threads to do ~30 units of work

You'd still maintain the per-chunk `totalFood`/COM sums, but now as a cheap reduction over the tile rather than incrementally — which also removes the float-drift fragility noted in §1.12.

### 5.4 Give the two species genuinely different sensing

Right now prey and predators differ only in speed, force, view radius, metabolism and digestion. Both see 360°, both get exactly the same 14 inputs, and the FOV machinery is dead (§1.5). That's a symmetric game with asymmetric constants, which is much less interesting than it could be.

Ideas that create real strategic asymmetry:

- **Predator: narrow cone (~120°), long range. Prey: full 360°, short range.** Suddenly approach angle matters, prey have a blind-spot-free defence, predators must commit to a heading. This is where the FOV code you've already written earns its keep.
- **Give prey a "nearest conspecific" input** (you already have the lattice). Flocking/selfish-herd behaviour emerges almost immediately and it's one of the most satisfying things to watch in this kind of sim.
- **Give predators the target's velocity** — you already compute `enemyClosingSpeed`/`enemyTangentialSpeed`, so interception (leading the target) is learnable. Check whether predators actually discover it; if not, the turning rate is probably the limiter (§5.6).
- **Multiple prey visible, not just the closest.** Currently a predator surrounded by five prey sees one. Two or three nearest slots would let flocking-vs-isolation strategies develop on both sides.

### 5.5 Genetic operators

- **No crossover.** You maintain elite pools of up to 500 brains and only ever mutate a single parent. For a 138-parameter genome, uniform crossover between two elites is ~10 lines and usually helps substantially. Worth trying single-point too, since your genome has natural layer boundaries.
- **Uniform mutation piles weights up at the clamps.** `change = uniform(-strength, +strength)` followed by a hard clamp to [−1,1] creates an accumulation at ±1. Use `curand_normal` for a Gaussian perturbation, and consider a soft bound (`tanh`) or a wider range.
- **Weight range [−1,1] is tight for a 14→8 layer.** With 14 inputs each in [−1,1] and weights near ±1, pre-activations reach ±15 and `tanhf` saturates flat, so most of the search space is plateau. Either scale layer 1 by `1/sqrt(INPUT_LAYER_SIZE)`, or widen the range to ±3.
- **HOF brains are injected unmutated.** That's defensible if you intend them as fixed benchmark opponents, but it means 10% of the population can never improve. Consider injecting a mutated copy instead, or keeping them as a genuinely frozen evaluation set.
- **The injection bands in `resetSimulation` are confusing.** `roll < 0.10` → HOF; `0.10 ≤ roll < 0.20` → falls through both branches and keeps its random init; `roll ≥ 0.20` → mutated elite. The "random injection" band is `[0.1, 0.2)`, not `[0, 0.2)`. It works, but name the bands explicitly — this is exactly the kind of thing that breaks silently when you tune the rates.
- **No recurrence.** `inputs[0..1]` are the previous outputs, which is a minimal form of memory. A proper recurrent hidden state (feed `hiddenValues` back in) costs 8 more inputs and would let agents integrate over time — useful for tracking a target that goes out of view, and for the patch-following behaviour that §5.3 enables.

### 5.6 Movement feel

- **Thrust is forward-only** (`max(0, out[0])`) and friction is 3.0, so retention per tick is 0.985 and speed halves in ~0.23 s. Reasonable — braking by releasing thrust works.
- **`MAXIMUM_TURNING_SPEED = 3.14 rad/s`** means a 180° turn takes a full second while moving at 30 units/s. That's a large turning circle, and it's probably why interception is hard to learn. Consider making turn rate depend inversely on speed (a real steering constraint, and it makes the speed/agility tradeoff something to learn) rather than a flat cap.
- **Predators have lower force (400) than prey (600) but higher top speed (35 vs 30).** So predators out-run but out-accelerate worse — a pursuit/evasion asymmetry that's actually nice. Keep it, and make sure `PREDATOR_EFFORT_MULTIPLIER = 1.0` vs prey `0.4` isn't cancelling the intent: predators pay 2.5× more per unit of effort *and* have 20× metabolism, so they're on a very short clock. With `STARTING_ENERGY = 66.7` and 1.0/s idle drain, a predator must kill within 66 seconds or die. Early generations with random brains will produce almost no successful predators, which means almost no predator learning signal. Consider a warmup: cheaper predator metabolism for the first N generations, or seed predators with a simple hand-coded pursuit brain as a baseline to mutate away from.

### 5.7 Reproduction thresholds are species-blind

`handleBirthsKernel` uses `energy > MAX_ENERGY` for everyone. A predator gains 50 per kill against a 150 cap, so 2 kills → a child; prey need 4 grass. Combined with the 20× predator metabolism you'll get very spiky population dynamics. Separate thresholds and costs per species would give you a knob to stabilise the classic Lotka-Volterra oscillation you presumably want to see.

Also note there's no carrying capacity other than `MAX_SWARM_CAPACITY` and food throughput — and because food respawns instantly on being eaten (§5.3), the *flux* of food energy is effectively unbounded even though the *stock* is fixed at 5000. Fixing food growth fixes this too.
