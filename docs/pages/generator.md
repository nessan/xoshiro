# The Generator Class

## Introduction

The `xso::generator` template class can be used to make _any variant_ from the xoshiro/xoroshiro family of random number generators.
It connects a [state type](#state-types) with a [scrambler functor](#scramblers) to form a pseudorandom number generator.
It is in the `xso` namespace and is declared as:

```cpp
template<typename State, typename Scrambler>
class generator {
 ...
};
```

- The `State` holds the state bits and provides methods to get, set, and advance it.
- The `Scrambler` type reduces the state to a single 32-bit or 64-bit unsigned integer output word.

> [!TIP]
> The provided [state types](#state-types) and [scramblers](#scramblers) have many settable parameters.
> This means that the `xso::generator` template class is very general.
> We recommend you instead use one of the [_predefined instantiations_](type-aliases.md), such as `xso::rng64` and `xso::rng32,` to access the recommended default state engines and output functions for 64-bit and 32-bit outputs.
> For most purposes, use the overall default `xso::rng`, which is a synonym for `xso::rng64`.

### Design

The heart of all random number generators is the method that produces a stream of _uniformly_ distributed output words --- usually 32-bit or 64-bit unsigned integers.
For many generators, including those in the xoshiro/xoroshiro family, the core step can be broken down into two stages.

In the first, the generator’s current _state_ $\mathbf{s}$ is iteratively advanced in some fashion, and, in the second, that state is filtered/scrambled to get a single output word:

$$
\begin{gather}
    \mathbf{s} \gets t(\mathbf{s}),     \\\
 o          \gets \phi(\mathbf{s}).
\end{gather}
$$

Here $t$ is the _transition function_ which advances the state, and $\phi$ is the output function, sometimes referred to as the _scrambler_, that reduces the state to a single output word $o$.

The two template parameters in our implementation reflect the two stages.
The `State` holds the state data and is responsible for advancing it from step to step (i.e. for the transition function $t$).
The `Scrambler` is the $\phi$ function that takes all the bits of state and distils them into a single unsigned output value.

### State Requirements

In practical applications, generators must be efficient as they are often expected to produce a vast number of outputs.

For this reason, most RNGs internally treat the state as some finite collection of computer words (for a lot of architectures, that will be 64-bit words or perhaps 32-bit words), and transition functions are designed to mix those words up in a manner that makes use of a small number of fundamental computer operations like `XOR` instructions and so on. This is how the xoshiro and xoroshiro generators are implemented.

The `xso::generator` class expects its `State` to expose the type of words used to hold the bits of state and to provide a specific API to access and advance the state.

Here are the required types and methods:

| Type/Class Method             | Description                                                                                                                   |
| ----------------------------- | ----------------------------------------------------------------------------------------------------------------------------- |
| `word_type`                   | Exposes the word type used to store the bits of state.                                                                        |
| `word_count`                  | Class method that returns the number of words in the state.                                                                   |
| `bit_count`                   | Class method that returns the number of bits in the state.                                                                    |
| `array_type`                  | A convenience container type that can hold the full state, characteristic polynomial coefficients, etc.                       |
| `seed`                        | Instance method to set the state.                                                                                             |
| `get_state`                   | Instance method to copy the current state to a destination iterator.                                                          |
| `operator[]`                  | Instance method to provide read-only access to the individual words of state.                                                 |
| `step`                        | Instance method to advance the state by one step.                                                                             |
| `type_string`                 | Class method that returns a name for the _type_ of the state engine.                                                          |
| `characteristic_coefficients` | Class method that returns the coefficients of the _precomputed_ characteristic polynomial in word form if they are available. |

**Notes:**

- The `seed` method is basically the `set_state` method---RNG literature uses "seed" for this function.
- The `step` is the _transition function_ that advances the state and serves as the key to the generator.
- The `type_string` method returns a string for the state _type_. <br> There is no instance-specific information in the returned string (i.e. no state data).
- The `characteristic_coefficients` method is discussed below in the [Jumping](#jumping) section.

The `xso::xoroshiro` and `xso::xoshiro` state engines primarily differ in how they implement the `step` method---the secret sauce, if you will.

### State Types

We define two state engines, each with several template parameters:

| State                       | Description                                                                                                                                                      |
| --------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `xso::xoroshiro<N,T,A,B,C>` | The state is packed into `N` unsigneds of type `T`. <br> The state implements the _xoroshiro_ style of state advancement using the parameters `A`, `B`, and `C`. |
| `xso::xoshiro<N,T,A,B>`     | The state is packed into `N` unsigneds of type `T`. <br> The state implements the _xoshiro_ style of state advancement using the parameters `A` and `B`.         |

The type `T` is either a 32-bit or 64-bit unsigned.

Some specific state types instantiations were analysed in the [original paper], and we have predefined type-aliases for those in `xoshiro.h`:

| Type-alias             | Definition                                 |
| ---------------------- | ------------------------------------------ |
| `xso::xoshiro_4x32`    | `xso::xoshiro<4, uint32_t, 9,  11>`        |
| `xso::xoshiro_4x64`    | `xso::xoshiro<4, uint64_t, 17, 45>`        |
| `xso::xoshiro_8x64`    | `xso::xoshiro<8, uint64_t, 11, 21>;`       |
| `xso::xoroshiro_2x32`  | `xso::xoroshiro<2,  uint32_t, 26, 9,  13>` |
| `xso::xoroshiro_2x64`  | `xso::xoroshiro<2,  uint64_t, 24, 16, 37>` |
| `xso::xoroshiro_2x64b` | `xso::xoroshiro<2,  uint64_t, 49, 21, 28>` |
| `xso::xoroshiro_16x64` | `xso::xoroshiro<16, uint64_t, 25, 27, 36>` |

### Scrambler Requirements

The scramblers are very simple _functors_ that implement one class method and one instance method:

| Method        | Description                                                     |
| ------------- | --------------------------------------------------------------- |
| `operator()`  | Instance method to reduce the passed state to a single word.    |
| `type_string` | Class method that should return a name for the output function. |

The `type_string` method returns a string for the scrambler _type_.
There is no instance-specific information in the returned string (i.e. no state data).

See the details on the [scramblers](scramblers.md) page.

### Scramblers

We define four scrambler functors, each with several template parameters:

In the following table, $i$ and $j$ are specific indices for the words in the state array $a$ that is passed as an argument to the function call.
The $S$ and $T$ are simple scaling parameters, $R$ is a rotation index, and we use $\rho_R(w)$ to denote the left rotation of the word $w$ by $R$.

|         Scrambler         | Template Parameters | Return from the scrambler's function call |
| :-----------------------: | :-----------------: | :---------------------------------------: |
|     `xso::star<S,i>`      |      $S$, $i$       |              $a_i \times S$               |
| `xso::star_star<S,T,R,i>` | $S$, $T$, $R$, $i$  |      $\rho_R(a_i \times S) \times T$      |
|     `xso::plus<i,j>`      |      $i$, $j$       |                $a_i + a_j$                |
|  `xso::plus_plus<R,i,j>`  |    $R$, $i$, $j$    |         $\rho_R(a_i + a_j) + a_i$         |

## Generator API Overview

Here is an overview of the `xso::generator` API:

| Category                                                      | Description                                                                          |
| ------------------------------------------------------------- | ------------------------------------------------------------------------------------ |
| [Class Types and Constants](#class-types-and-constants)       | Class level methods and exposed types.                                               |
| [Constructors](#constructors)                                 | Methods to create and seed generators.                                               |
| [Seeding Methods](#seeding-methods)                           | Methods to seed generators to a new state.                                           |
| [Output Methods](#output-methods)                             | Methods to advance the generator and extract uniform unsigned outputs.               |
| [State Access](#state-access)                                 | Methods that provide read-only access to the state words.                            |
| [Sampling with Replacement](#sampling-with-replacement)       | Methods to extract single random samples from various sources.                       |
| [Sampling without Replacement](#sampling-without-replacement) | Methods to extract random samples without replacement from various sources.          |
| [Shuffling](#shuffling)                                       | Methods to shuffle items in iterations and containers.                               |
| [Discarding](#discarding)                                     | Method to advance the generator, ignoring the output.                                |
| [Jumping](#jumping)                                           | Method to efficiently jump a generator very far forward in its random number stream. |
| [Extra Methods](#extra-methods)                               | Extra methods that are available if we can use the optional [`gf2`] library.         |

## Class Types and Constants

The `xso::generator` class defines the following types and class methods:

| Typename                         | Description                                                                                               |
| -------------------------------- | --------------------------------------------------------------------------------------------------------- |
| `xso::generator::state_type`     | The specific `State` for this generator type.                                                             |
| `xso::generator::scrambler_type` | The specific `Scrambler` for this generator type.                                                         |
| `xso::generator::word_type`      | The state bits are packed into words of this type (either 32-bit or 64-bit unsigneds).                    |
| `xso::generator::array_type`     | A convenience container type to hold the full state of this generator, jump polynomial coefficients, etc. |
| `xso::generator::word_count`     | A class method that returns the number of words in the state.                                             |
| `xso::generator::bit_count`      | A class method that returns the number of bits of state.                                                  |
| `xso::generator::type_string`    | A class method that returns a name for this _type_ of generator.                                          |

Most of these are forwarded from the static types and data in the underlying `State` and `Scrambler` template parameters, or are trivial computations from those.
For example, the `xso::generator::type_string` method basically concatenates the type strings it gets from its `State` and `Scrambler`.

The `xso::generator` class also defines a type and a couple of class methods that are required to satisfy the [`std::uniform_random_bit_generator`] concept.

| Constant                      | Description                                                    |
| ----------------------------- | -------------------------------------------------------------- |
| `xso::generator::result_type` | Each `generator()` call returns a single integer of this type. |
| `xso::generator::min`         | The smallest value the generator can produce.                  |
| `xso::generator::max`         | The largest value the generator can produce.                   |

- For our family, `xso::generator::result_type` is always identical to the `xso::generator::word_type`.
- The `xso::generator::min` method always returns `0`.
- The `xso::generator::max` method always returns the largest value that can be represented by the `xso::generator::result_type`. <br>

Satisfying the [`std::uniform_random_bit_generator`] concept allows us to use, e.g. `xso::rng` as the driver for the many methods defined in the standard [`<random>`] library.

## Constructors

We provide the following three constructors in the `xso::generator` class:

### Default Construction

The default constructor uses calls to [`std::random_device`] to randomly seed _all_ the state words underlying the generator.
Reportedly, some implementations of that standard facility are not great, so we also mix a _scrambled_ call to a high-resolution clock for one of the state words.

### Construction from a Single Word

Seeding any pseudorandom generator with multiple words of state from a single word is never ideal.

Nevertheless, it is often handy to be able to do exactly this when prototyping a simulation.
For that stage of development, being able to run and rerun things with an easily constructed "fixed" random number stream is very useful.

To mitigate some of the worst features of this type of construction, our implementation uses a scrambled version of the single input word as a seed for a simple, small-state, but pretty effective [SplitMix] random number generator. That generator then creates a full state array for the _xoshiro/xoshiro_ in question.

**Example**

```c++
#include <xoshiro.h>
int main()
{
    xso::rng64 g0(1);           // <1>
    xso::rng64 g1(2);
    std::println("Calls to two nearby seeded xso::rng64 generators:");
    for (auto i = 0uz; i < 5; ++i)
        std::println("Call {}: {}\t{}", i, g0(), g1());

    g0.seed(123);               // <2>
    g1.seed(123);
    std::println("\nCalls to two identically seeded xso::rng64 generators:");
    for (auto i = 0uz; i < 5; ++i)
        std::println("Call {}: {}\t{}", i, g0(), g1());
}
```

1. Create two _xoshiro_ generators with very similar seeds, and print the results of a few calls to each.
2. Reseed both generators with the same seed and again print the results of a few calls.

Here is the program output from one run (output will vary for each run):

```txt
Calls to two nearby seeded xso::rng64 generators:           # <1>
Call 0: 17411061279518156131    4890025243076846858
Call 1: 17949023749619734469    9084667301668848462
Call 2: 175122747298038362      1296031388660499858
Call 3: 5372970870250148463     5968655099240147287
Call 4: 10829287877746345362    18045567523022708517

Calls to two identically seeded xso::rng64 generators:      # <2>
Call 0: 12981287971670330679    12981287971670330679
Call 1: 7116868237474176081     7116868237474176081
Call 2: 7843178069876964882     7843178069876964882
Call 3: 2023445265056700755     2023445265056700755
Call 4: 2199654306148732348     2199654306148732348
```

1. Note that while the seeds were very close, the generated outputs are not!
2. With identical seeds, we get identical outputs.

### Construction from Full State

We have a method that lets you fully specify the seed state by passing in an iteration of appropriate numbers.

> [!NOTE]
> If you do construct a generator by passing in an iteration of words, then be sure that the values are not all zero, as that is a fixed point for these generators.

**Example**

```c++
#include <xoshiro.h>
int main()
{
    xso::rng f, g;                              // <1>
    std::println("f(): {}", f());               // <2>
    std::println("g(): {}", g());

    xso::rng::array_type state;
    f.get_state(state.begin());                 // <3>

    xso::rng h(state.cbegin(), state.cend());   // <4>
    std::println("f(): {}", f());               // <5>
    std::println("h(): {}", h());
}
```

1. Two default-constructed generators f and g are fully and randomly seeded.
2. We print one output from each generator and expect those to be different.
3. We then capture the current state from the `f` generator.
4. We construct a third generator `h` seeded from that captured state.
5. We print one output from `f` and one from h, and expect them to be identical.

Here is the program output from one run (output will vary for each run):

```txt
f(): 1887568754843356615        # <1>
g(): 287576570925641124
f(): 16125400203545409679       # <2>
h(): 16125400203545409679
```

1. Different values are expected from calls to `f()` and `g()`.
2. Identical values as expected from calls to `f()` and `h()`.

## Seeding Methods

The term "seeding" is very commonly used in the context of random number generation.
It just means setting the state to some given values.

You can create a generator and seed it later if you wish.
The `xso::generator::seed` methods mirror the three constructors above:

- The no-argument version fully seeds the state from [`std::random_device`] as described in the [Default Construction](#default-construction) section.
- The single-unsigned-argument version seeds the state from that value, as described in the [Unsigned Integer Construction](#unsigned-integer-construction) section.
- The version that is passed an iteration of words, fully seeds the state by copying those words as described in the [Full State Construction](#full-state-construction) section.

## Output Methods

The core of any random number generator is the method that outputs the random variates:

| Method                         | Description                                                                                                                |
| ------------------------------ | -------------------------------------------------------------------------------------------------------------------------- |
| `xso::generator::operator()()` | Scrambles the current state to produce a single output word, and steps the state forward in preparation for the next call. |
| `xso::generator::step()`       | Advances the `State` by forwarding the call to its particular implementation.                                              |

The output is either a 32-bit or 64-bit unsigned integer, depending on the particular xoshiro/xoroshiro variant you are using.

For example, calls to `xso::rng32()` produces _uniformly distributed_ 32-bit unsigneds, while calls to `xso::rng64()` or `xso::rng()` produce _uniformly distributed_ 64-bit unsigneds.

These bit blocks aren't particularly useful by themselves!
They have to be shaped in some manner if, for example, you want to do something as simple as simulate the roll of a dice.
We have other methods, described below, that perform many common tasks based on the output of the `xso::generator::operator()()` method.

> [!NOTE]
> The existence of the `xso::generator::operator()()` method means that our generators satisfy the [`std::uniform_random_bit_generator`] concept.
> That, in turn, opens up all the functionality available in the standard [`<random>`] header.

The `xso::generator::step()` method is forwarded to `xso::xoshiro::step()` or `xso::xoroshiro::step()` as appropriate.
Those implement the algorithms described in the [original paper].

## State Access

We have methods that provide read-only access to the state:

| Method                         | Description                                                 |
| ------------------------------ | ----------------------------------------------------------- |
| `xso::generator::operator[]()` | Returns a copy of a single word of state at a passed index. |
| `xso::generator::get_state`    | Copies the entire state into a user-provided iterator.      |

The `xso::generator::get_state` method assumes your iterator can accept values of the appropriate type and has enough room for all the words of state.

**Example**

```c++
#include <xoshiro.h>
int main()
{
    xso::rng g0(1), g1(2); // <1>
    std::println("After seeding with nearby seeds:");
    for (auto i = 0uz; i < xso::rng::word_count(); ++i)
        std::println("g0[{}] = {}\tg1[{}] = {}", i, g0[i], i, g1[i]); // <2>
}
```

1. Create a pair of generators that are seeded from neighbouring integers.
2. Print each word of the state for the two generators.

Sample output (varies from run to run):

```txt
After seeding with nearby seeds:
g0[0] = 2973750756608955175     g1[0] = 17576013672004619970    # <1>
g0[1] = 11413743089567126834    g1[1] = 16442790642303457438
g0[2] = 4383533935485149688     g1[2] = 3610580237369810524
g0[3] = 3535799440905119283     g1[3] = 11046704898693758421
```

1. Seeded `g0` and `g1` with neighbouring values. <br>
   ['SplitMix'] was used to mix those seeds up and get them to very different underlying state arrays.

## Sampling with Replacement

You can use the `xso::generator::sample` method to extract a single sample from:

- A closed _integer_ interval `[a, b]`.
- A half-open _real_ interval `[a, b)`.
- An iteration $[b, e)$, where all elements are equally likely to be returned. <br> No error checks are performed, and the behaviour is undefined if $e < b$.
- Any container `c`, where all elements are equally likely to be returned. <br> The method will work on any container `c` which supports `std::cbegin(c)`. <br> No error checks are performed, and the behaviour is undefined if the container is empty.
- Any of the many distributions defined in the standard [`<random>`] library.

There are also appropriately named calls for some common sampling tasks:

| Method                  | Description                                                                      |
| ----------------------- | -------------------------------------------------------------------------------- |
| `xso::generator::index` | Returns a random index in the range `[0, len)`.                                  |
| `xso::generator::flip`  | Returns the result of flipping a coin that can be biased.                        |
| `xso::generator::roll`  | Returns the result of rolling a dice that can have an arbitrary number of sides. |

The `xso::generator::flip` can be passed a probability of "heads" in the range `[0, 1]` --- the default value is `0.5`.

The `xso::generator::roll` method can be passed the number of die faces --- the default value is `6`.

**Example**

```c++
#include <xoshiro.h>
int main()
{
    xso::rng    rng;
    std::size_t n, n_samples = 3;

    std::println("Characters from ['a','z']");
    for (n = 0; n < n_samples; ++n) std::print("'{:c}' ", rng.sample(std::uint8_t{'a'}, std::uint8_t{'z'}));
    std::print("\n");

    std::println("Integers from [1,10]");
    for (n = 0; n < n_samples; ++n) std::print("{} ", rng.sample(1, 10));
    std::print("\n");

    std::println("Reals from [1,10)");
    for (n = 0; n < n_samples; ++n) std::print("{:4.2f} ", rng.sample(1., 10.));
    std::print("\n");

    std::normal_distribution nd{70., 15.};
    std::println("Normals with mean {} and std-dev {}", nd.mean(), nd.stddev());
    for (n = 0; n < n_samples; ++n) std::print("{:4.2f} ", rng.sample(nd));
    std::print("\n");

    std::binomial_distribution bd{6, 0.5};
    std::println("Binomials with {} trials and P[success] = {}", bd.t(), bd.p());
    for (n = 0; n < n_samples; ++n) std::print("{} ", rng.sample(bd));
    std::print("\n");

    std::array<int, 10> array;
    std::iota(array.begin(), array.end(), 0);
    std::println("Random from the array {}", array);
    for (n = 0; n < n_samples; ++n) std::print("{} ", rng.sample(array));
    std::print("\n");

    return 0;
}
```

Sample output (varies from run to run):

```txt
Characters from ['a','z']
'm' 'b' 'x'
Integers from [1,10]
9 1 5
Reals from [1,10)
6.33 7.51 8.36
Normals with mean 70 and std-dev 15
68.52 57.54 65.25
Binomials with 6 trials and P[success] = 0.5
3 3 4
Random from the array [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
2 7 5
```

## Sampling without Replacement

You can also use the `xso::generator::sample` method to extract `n` samples _without replacement_ from:

- An iteration $[b, e)$, where all elements are equally likely to be picked. <br> No error checks are performed, and the behaviour is undefined if $e < b$.
- Any container `c`, where all elements are equally likely to be picked. <br> The method will work on any container `c` which supports `std::cbegin(c)`. <br> No error checks are performed, and the behaviour is undefined if the container is empty.

These methods will preserve the order of the selected elements.
You can use the `xso::generator::shuffle` method to mix those up.
View [`std::sample`] for more details.

There is also a method to copy samples from an arbitrary distribution, as defined in the standard [<random>] header, into a destination.

These methods are passed a destination iterator `dst` that must be able to accept at least `n` elements of the type stored in the iteration/container, etc.

**Example**

```c++
#include <xoshiro.h>
int main()
{
    xso::rng rng;

    constexpr std::size_t K = 5;

    std::array<int, 2*K> u;
    std::iota(u.begin(), u.end(), 0);

    std::array<int, K> u_samples;
    rng.sample(u, u_samples.begin(), K); // <1>
    std::println("Array of values:           {}", u);
    std::println("Samples from array:        {}", u_samples);

    std::normal_distribution nd{70., 15.};
    std::array<double, K> nd_samples;
    rng.sample(nd, nd_samples.begin(), K); // <2>
    std::println("Samples from distribution: {::4.2f}", nd_samples);
}
```

1. Generate 5 samples from the population of 10 elements.
2. Generate 5 samples from a normal distribution.

Sample output (varies from run to run):

```txt
Array of values:           [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
Samples from array:        [1, 2, 4, 7, 9]
Samples from distribution: [61.94, 89.22, 67.06, 90.72, 73.79]
```

## Shuffling

We have methods to shuffle the elements of:

- An iteration $[b, e)$. <br> No error checks are performed, and the behaviour is undefined if $e < b$.
- Any container `c` that supports `std::cbegin(c)`. <br> No error checks are performed, and the behaviour is undefined if the container is empty.

**Example**

```c++
#include <xoshiro.h>
int main()
{
    std::array<int, 10> u;
    std::iota(u.begin(), u.end(), 0);
    std::println("Original: {}", u);
    xso::rng rng;
    rng.shuffle(u);
    std::println("Shuffled: {}", u);
}
```

Sample output (varies from run to run):

```txt
Original: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
Shuffled: [4, 5, 7, 9, 0, 8, 2, 1, 3, 6]
```

## Discarding

The `xso::generator::discard` method advances the state while _ignoring_ the generated outputs.
All of the generators in the standard [`<random>`] have a similar method, and they all appear to use the identical slow and simple-minded approach of simply calling their equivalent of the `xso::generator::step` method the appropriate number of times.

For large numbers of discards, the objective is achieved _much_ more efficiently by using the `xso::generator::jump` method described below.

We keep the `xso::generator::discard` method for compatibility with the standard library generators.

**Example**

```c++
#include <xoshiro.h>
int main()
{
    xso::rng rng0; // <1>
    xso::rng rng1 = rng0; // <2>

    std::size_t n = 10;
    std::println("Calling rng0() a total of {} times:", n);
    for (auto i = 0; i < 10; ++i) std::println("Call {}: rng0() = {}.", rng0());

    std::println("Making sure that rng1, the jumped forward copy of rng0 matches");
    rng1.discard(n); // <3>
    std::println("Next call rng0() yields: {}", rng0()); // <4>
    std::println("Next call rng1() yields: {}", rng1()); // <4>
}
```

1. Create a randomly seeded RNG.
2. Make a copy of that generator --- one with the exact same state.
3. We have called `rng0()` multiple times and now get `rng1` to the same point by using the `discard` method.
4. Make one more call to `rng0()` and `rng1()` to demonstrate they have reached the identical point in their random number streams.

Sample output (varies from run to run)

```txt
Calling f() 10 times:
Call 0: f() = 14005126867685016518
Call 1: f() = 10640180379987258470
Call 2: f() = 16367042443805637715
Call 3: f() = 10852054829793099863
Call 4: f() = 17355665557991136519
Call 5: f() = 17884768122497745551
Call 6: f() = 8981322931967380562
Call 7: f() = 9724290714337168340
Call 8: f() = 11553202802824749703
Call 9: f() = 5222239817378406001
Making sure the jumped forward copy of rng0 matches:
rng0() = 14624851349943862646
rng1() = 14624851349943862646
```

## Jumping

Our implementation of xoshiro/xoroshiro is distinguished by having the ability to _efficiently jump_ a generator _arbitrarily_ very far forward in its random number stream.

> [!NOTE]
> This is in contrast to the generators in the standard [`<random>`] library, which all use the simplistic discard method discussed above.
> It is also an improvement over the original C versions of the generators---those only support a few specific choices of jump size.

| Method                                        | Description                                                                                                               |
| --------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------- |
| `xso::generator::jump`                        | Instance method that jumps a generator forward in its random number stream.                                               |
| `xso::jump_coefficients<State>`               | Free function that returns the _jump polynomial_ that can be used for a particular jump size.                             |
| `xso::generator::characteristic_coefficients` | Class method that returns the state's _precomputed_ characteristic polynomial coefficients packed into an array of words. |

In this hierarchy, the bottom of the pyramid is the state's _characteristic polynomial_, whose coefficients can be packed into an array of words.
The `xoshiro.h` header file has those _precomputed_ coefficients embedded in it for _all_ its [type-aliased](type-aliases.md) generators.
Retrieving them is a zero-cost operation.

The `xso::jump_coefficients<State>` free function uses the precomputed characteristic polynomial coefficients to find a _jump polynomial_ for any particular choice of jump size `J`, where `J` can be a huge number of steps (e.g. $J = 2^100$).
This is a relatively expensive calculation that is described in detail on the [jump technique][] page.

Once you have the jump polynomial for a particular `J`, it can be reused again and again to perform repeated jumps further and further along a single stream of random numbers.

Repeated jumping by a fixed _large_ number of steps is the key to efficiently partitioning the random number stream for parallel computation.
See the [partition class](partition.md) page for more details.

**Example**

```c++
#include <xoshiro.h>
int main()
{
    xso::rng r0;                                                // <1>
    xso::rng r1 = r0;
    auto J = 1'000'000uz;                                       // <2>

    auto jump_coefficients = xso::jump_coefficients<xso::rng::state_type>(J);    // <3>
    r0.jump(jump_coefficients);                                 // <4>
    r1.discard(J);

    for (auto i = 0uz; i < xso::rng::word_count(); ++i) {       // <5>
        if(r0[i] != r1[i]) {
            std::println("Oops we expected the states to be identical!");
            exit(1);
        }
    }
}
```

1. Construct two identical generators `r0` and `r1`.
2. Here we using a very modest jump size so the `discard` method has no issues.
3. Precompute the _jump polynomial_ coefficients for this $J$.
4. Jump the generators by $J$ steps two different ways
5. Check that the states are identical in the two generators.

You can also call the `xso::generator::jump` method with just the jump size.
In that scenario, it will perform the jump and return the jump polynomial you can then reuse---computing the jump polynomial is more expensive that using it to perform a jump.

Here is an example:

```c++
#include <xoshiro.h>
int main()
{
    xso::rng r0;                                        // <1>

    auto J = 100uz;
    auto J_is_log2 = true;                              // <2>

    auto r1 = r0;                                       // <3>
    auto jump_coefficients = r1.jump(J, J_is_log2);     // <4>

    auto r2 = r1;
    r2.jump(jump_coefficients);                         // <5>

    ...
}
```

1. `r0` is the "parent" generator.
2. The jump size here is $2^100$, which is not achievable using the `xso::generator::discard` method.
3. `r1` starts as a copy of `r0`.
4. We jump `r1` to be $2^100$ steps further along the parent random number stream. <br> Used like this the `jump` method is sower but it returns the jump polynomial for a fast return visit.
5. `r2` starts as a copy of `r1`, and we then quickly jump it forward by another $2^100$ step.

> [!NOTE]
> The `xoshiro.h` header file has the characteristic polynomial coefficients embedded in it for _all the recommended_ xoshiro and xoroshiro generators.
> If you want to roll your own variant, you can use the [`gf2`] library to achieve the same end as described in the next section.

## Extra Methods

The `xoshiro.h` header file is designed to be self-contained.
It has everything you need when you are using one of the [recommended generators](type-aliases.md).
That includes having the ability to perform arbitrarily distant jumps along a random number stream.

But perhaps, you are interested in exploring some other xoshiro or xoroshiro variant?
The key thing of interest will be the transition matrix for that variant, and its associated characteristic polynomial.
Extracting those is easy if the [`gf2`] library is available.

> [!NOTE]
> Most users will just want to drop the single `xoshiro.h` file into their project and then use `xso::rng` as an excellent, well-proven pseudorandom number generator. There is nothing extra to download or link to. Everything is provided in the single header file. The extra methods described in this section are gated behind a conditional `GF2` flag---if it is set at compile time, then we assume that the [`gf2`] library is available to be called on.

[`gf2`] is a C++ library for doing linear algebra over [GF(2)] the simplest field of two elements $\{0,1\}$, where the usual arithmetic operations are performed mod 2.
The [`gf2`] library is header only, so it is easily incorporated into any application.

If it is available, then `xoshiro.h` defines some extra functions:

| Function                                | Description                                                                                                                                        |
| --------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------- |
| `xso::transition_matrix<State>`         | A free function that returns the _transition matrix_ for a generator or state as a [`gf2::BitMatrix`].                                             |
| `xso::characteristic_polynomial<State>` | A free function that returns the transition matrix's [characteristic polynomial] as a [`gf2::BitPolynomial`].                                      |
| `xso::jump_polynomial<State>`           | A free function that returns a _jump polynomial_ as a [`gf2::BitPolynomial`] that can be used to jump a generator ahead by a huge number of steps. |
| `xso::generator::jump`                  | A version of the `jump` instance method that uses a jump-polynomial passed as a [`gf2::BitPolynomial`].                                            |

**Example:**

In the following example, we use the [`gf2`] library to compute some jump polynomials for a non-standard xoshiro generator:

```c++
#include <xoshiro.h>
#include <gf2/gf2.h>

int
main() {
    using state = xso::xoshiro<4, std::uint64_t, 17, 47>;               // <1>

    // We will pack our polynomials into an array of words.
    typename state::array_type poly_words;

    // Print the name of the state class we are working on.
    std::println("Jump coefficients for: {}", state::type_string());

    // Precompute the engine's characteristic bit-polynomial.
    auto c = xso::characteristic_polynomial<state>();                   // <2>
    auto p = c.sub(state::bit_count() - 1);                             // <3>
    p.coefficients().to_words(poly_words.begin());                      // <4>
    std::println("char-poly: {::#x}", poly_words);

    // Jump amount: N = 2^(0.25*n_bits) where the RNG has n_bits bits of state.
    std::size_t power = state::bit_count() / 4;
    xso::jump_polynomial<state>(c, power, true).coefficients().to_words(poly_words.begin());    // <5>
    std::println("jump25:    {::#x}", poly_words);

    // Jump amount: N = 2^(0.50*n_bits) where the RNG has n_bits bits of state.
    power = state::bit_count() / 2;
    xso::jump_polynomial<state>(c, power, true).coefficients().to_words(poly_words.begin());
    std::println("jump50:    {::#x}", poly_words);

    // Jump amount: N = 2^(0.75*n_bits) where the RNG has n_bits bits of state.
    power = 3 * state::bit_count() / 4;
    xso::jump_polynomial<state>(c, power, true).coefficients().to_words(poly_words.begin());
    std::println("jump75:    {::#x}", poly_words);
}
```

1. We create a specific xoshiro state.
2. Use [`gf2`] to extract the transition matrix's characteristic polynomial $c(x)$.
3. We drop the highest order term in $c(x)$ to get $p(x)$.
4. Then get the coefficients of $p(x)$ as a collection of four 64-bit words.
5. For "extra credit" we use the characteristic polynomial to compute some "standard" jumps polynomials.

Here is the output:

```txt
Coefficients words for: xoshiro<4x64,17,47>
char-poly: [0xea92534c00000001, 0x5df4d1e52535df5d, 0xe535df5de535de52, 0xfa682b9cfb678d0]
jump25:    [0xad5cbfb98349db7c, 0xb12354c699d256c9, 0xabeeb75108114638, 0x2d8d89a5c04c5e49]
jump50:    [0x5dfe507554dcb914, 0x2cf9597b1c226e74, 0xfb53f33b2cc87058, 0x66de33fafd4f5082]
jump75:    [0xcdb4846a6376aa76, 0x2c6e70f7adbf34d2, 0x7a578e2952b82063, 0x829a0851c8785a61]
```

## See Also

- [Recommended Generators](type-aliases.md)
- [The Jump Technique](jump-technique.md)
- [The Partition Class](partition.md)

<!-- Reference links to items in the standard library -->

[`std::array`]: https://en.cppreference.com/w/cpp/container/array
[`concept`]: https://en.cppreference.com/w/cpp/language/constraints
[`std::format`]: https://en.cppreference.com/w/cpp/utility/format/format
[`std::formatter`]: https://en.cppreference.com/w/cpp/utility/format/formatter
[`std::mersenne_twister_engine`]: https://en.cppreference.com/w/cpp/numeric/random/mersenne_twister_engine
[`std::normal_distribution`]: https://en.cppreference.com/w/cpp/numeric/random/normal_distribution
[`std::random_device`]: https://en.cppreference.com/w/cpp/numeric/random/random_device
[`<random>`]: https://en.cppreference.com/w/cpp/header/random
[`std::sample`]: https://en.cppreference.com/w/cpp/algorithm/sample
[`std::seed_seq`]: https://en.cppreference.com/w/cpp/numeric/random/seed_seq
[`std::shuffle`]: https://en.cppreference.com/w/cpp/algorithm/random_shuffle
[`std::uniform_random_bit_generator`]: https://en.cppreference.com/w/cpp/numeric/random/uniform_random_bit_generator
[`std::unsigned_integral`]: https://en.cppreference.com/w/cpp/concepts/unsigned_integral
[`std::vector`]: https://en.cppreference.com/w/cpp/container/vector

<!-- Other reference links -->

[`SplitMix`]: https://dl.acm.org/doi/10.1145/2660193.2660195
[original paper]: https://vigna.di.unimi.it/ftp/papers/ScrambledLinear.pdf
[Haramoto et al.]: http://www.math.sci.hiroshima-u.ac.jp/m-mat/MT/ARTICLES/jumpf2-printed.pdf
[`gf2`]: https://nessan.github.io/gf2
[`gf2::BitVector`]: https://nessan.github.io/gf2/md_docs_2pages_2BitVector.html
[`gf2::BitPolynomial`]: https://nessan.github.io/gf2/md_docs_2pages_2BitPolynomial.html
[`gf2::BitMatrix`]: https://nessan.github.io/gf2/md_docs_2pages_2BitMatrix.html
[characteristic polynomial]: https://en.wikipedia.org/wiki/Characteristic_polynomial
