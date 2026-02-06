# Partitions

## Introduction

The `xso::partition<RNG>` class partitions a single stream of random numbers into a collection of non-overlapping sub-streams.
This is primarily useful for setting up parallel computations.

The template parameter `RNG` should have a working `xso::generator::characteristic_coefficients` class method.

## Construction

You construct an `xso::partition` by passing a parent generator, seeded to a starting state, and a number of partitions:

```c++
partition(const RNG& rng, std::size_t n_partitions);
```

Here, `rng` is the parent generator, and we want to partition its random state stream into `n_partitions` non-overlapping sub-streams.
Each of those sub-streams will be "owned" by a _copy_ of `rng`, seeded with the appropriate starting state for the sub-stream.

The constructor computes a jump size $J$ that depends on the `n_partitions` argument.
It then uses the generator's characteristic coefficients to compute the corresponding _jump polynomial_ for jumps of size $J$.

## Retrieving the Partitions

The `xso::partition::next` method returns a _new_ generator that is a copy of the parent `rng` but seeded at the start of the next partition.
The returned `RNG` will be the same as `rng`, with its state jumped along by some multiple of $J$ steps

**Example**

The `xso::partition` class is best illustrated with a _sketch_ example:

```c++
#include <xoshiro.h>
int main()
{
    auto           n_cores = available_cores(...);     // <1>
    xso::rng       rng;                                // <2>
    xso::partition partition(rng, n_cores);            // <3>
    for(std::size_t job = 0; job < n_cores; ++job) {
        spawn_job(partition.next(), ...);               // <4>
    }
    ...
}
```

1. This represents some method that returns the number of compute cores we have available.
2. Creates  a randomly seeded _parent_ generator.
3. This constructs a partition of the parent's random number stream into `n_cores` non-overlapping pieces.
4. And we're off spawning an independent job for each core, handing the job its own sub-stream generator to work with.

In the sketch, we first determine how many cores are available for the simulation.
That may be a fixed number, or some function determines the number on the fly.
For the sake of this example, suppose that `n_cores` comes back as 100.

We want to partition the random number stream from `rng` into 100 pieces and hand each core its own sub-stream to work with.
So, we set up an `xso::partition` object to do just that.

The loop spawns 100 jobs where each one is given its own random number generator.
The generators come from calling the `xso::partition.next` method.
The `xso::partition` class ensures that these generators are identical to the original `rng`, except that they are seeded appropriately far along to the next sub-stream.

The parent generator has 256 bits of state, so its random state stream has $2^{256}$ slots.
This means that all the sub-streams still have a vast number of random deviates to use up before there is any chance of getting overlaps.

## Implementation Note

In the example above, we use a random number generator `xso::rng`, which has 256 state bits.
Those generators will step through an enormous $2^{256}$ states before repeating the cycle.

We have 100 cores available for parallel computation.
Ideally, we want to partition the random number stream from `rng` into 100 pieces and hand each core its own non-overlapping sub-stream of size:

$$
\frac{2^{256}}{100}.
$$

However, that number will overflow any normal 32-bit or 64-bit integer!
Instead, the partition class will create $N =2^p$ sub-streams, where $N$ is a _power of two_ close to, but larger than 100.
In this case, that power will be 7 as $2^7 = 128 \ge 100$.

The number of created non-overlapping partitions is 128 instead of 100.

We only use 100 of those 128 as we spawn independent jobs, so each partition is therefore slightly smaller than the ideal, and of course, we are not using any of the final 28 partitions at all.
However, each of the sub-streams still has some $2^{256 - 7} = 2^{249}$ available random numbers.
That is vastly more than a job on any present-day computer will ever consume, so the "wastage" is completely insignificant.

> [!WARNING]
> The `xso::partition` class requires the `RNG` to have its pre-computed characteristic coefficients available.
> This is the case for all our [recommended generators](type-aliases.md).

## See Also

- [The Generator Class](generator.md)
- [Recommended Generators](type-aliases.md)
- [The Jump Technique](jump-technique.md)
