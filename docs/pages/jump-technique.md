# The Jump Technique

## Introduction

The heart of all random number generators is the method that produces a stream of _uniformly_ distributed output words---usually 32-bit or 64-bit unsigned integers.
That core stream is used by other functions to simulate any desired distribution over the field of interest, for example, to simulate a uniform distribution of reals in the range 0 to 1.

For some generators, including those in the xoshiro/xoroshiro family, the core step has two stages.
In the first, the generator’s current _state_ $\mathbf{s}$ is iteratively advanced in some fashion, and, in the second, that new state is passed through a function to get an output $\phi(\mathbf{s})$ of the desired size:

$$
\begin{gather}
    \mathbf{s} \gets t(\mathbf{s}),     \\\
 o          \gets \phi(\mathbf{s}).
\end{gather}
$$

Here $t$ is the _transition function_ which advances the state, and $\phi$ is the output function, sometimes referred to as the _scrambler_, that reduces the state to a single output word $o$.

The output function doesn’t change the current state but mangles it in some way to project the higher-dimensional state to the desired number of output bits.
At a minimum, the state will have the same number of bits as an output word, and generally, "good" generators will have more state bits than that.

Sound generators produce streams of output words with desirable statistical properties.
For example, uniformly distributed across the output space, having a high degree of unpredictability, etc.
A complete mathematical analysis of an RNG is typically complicated, but various empirical test suites directly examine the output streams looking for statistical weaknesses.

Of course, a sound generator must also be _efficient_.
In many practical applications, the generator must produce a vast number of outputs, so speed is essential.

For this reason, most RNGs internally treat the state as a finite collection of computer words (in many architectures, 64-bit words or perhaps 32-bit words). Their transition functions mix those words using a small number of basic computer operations, such as `XOR` instructions.

For example, one well-regarded RNG for 64-bit outputs is a particular variant of _xoshiro_.
In this RNG, the state is stored internally as a collection of 4 64-bit words (256 bits in total).

If that array is `{s[0], s[1], s[2], s[3]}` then the state step/transition function $t$ is:

```cpp
void step()
{
    const std::uint64_t tmp = s[1] << 17;
    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];
    s[2] ^= tmp;
    s[3] = std::rotl(s[3], 45);
}
```

So a single bit-shift operation, five `XOR` operations, and a final bitwise left-rotation.
Not a lot of computation and all of it is fundamentally efficient.

The step function can be coupled with various simple scramblers to produce the desired output.
One $\phi(\mathbf{s})$ that is used is:

```cpp
std::uint64_t operator()(const std::uint64_t *s) { return std::rotl(s[1] * 5, 7) * 9; }
```

The total amount of code here is tiny, and this RNG will be efficient.
Of course, verifying its quality is another matter.

## Linear Transition Functions

The state transition function $t$ for many popular RNGs is actually written as a _linear_ transformation when the state is viewed as a vector over [GF(2)], the simplest [Galois Field] with two elements.
GF(2) is just a mathematical way of talking about _bits_---and is also referred to as $\FF$.

> [!NOTE]
> It seems surprising that anything "random" can have a linear representation! However, this is linearity over [GF(2)]---the implicit _folding_ that occurs in bit-space can have the same sort of mixing effects that _non-linearity_ has over the reals.

GF(2) has just two elements, 0 and 1, and all the usual arithmetic operations are carried out mod 2 to keep everything closed in $\{0,1\}$.
You can easily verify that addition in GF(2) becomes `XOR` and multiplication becomes `AND`.
It is also helpful to know that you can replace any minus signs with pluses and divisions by multiplications.

In our specific _xoshiro_ example above, we saw that the generator works internally on an array of four 64-bit unsigned words and the transition function $t$ performs a small number of bit-shift, `XOR`, and bit-rotation operations on and between those four words.

For analysis purposes, the four 64-bit state words can be viewed as a single 256-element state bit-vector:

$$
    \mathbf{s} \in \FF^{256}.
$$

Remember that any `XOR` operation between 64-bit words is an addition in $\FF^{64}$ when those words are viewed as 64-element bit-vectors.

We also note that bit-shifting a 64-bit number one place to the left can be thought of as pre-multiplying the word’s 64 bits with a $64 \times 64$ left _bit shift_ matrix (i.e. the bit-matrix that is all zeros except for ones on the principal sub-diagonal):

$$
\cal{L} =
\begin{bmatrix}
0 & 0 & 0 & \ldots & 0 & 0 & 0 \\\
1 & 0 & 0 & \ldots & 0 & 0 & 0 \\\
0 & 1 & 0 & \ldots & 0 & 0 & 0 \\\
\vdots & \vdots & \vdots & \vdots & \vdots & \vdots & \vdots \\\
0 & 0 & 0 & \ldots & 1 & 0 & 0 \\\
0 & 0 & 0 & \ldots & 0 & 1 & 0
\end{bmatrix}
$$

Similarly, rotating a 64-bit word one place to the left can be thought of as pre-multiplying the word’s 64 bits with a $64 \times 64$ left _bit rotation_ matrix (i.e. the bit-matrix that is all zeros except for ones on the principal sub-diagonal and a one in the upper right corner):

$$
\cal{R} =
\begin{bmatrix}
0 & 0 & 0 & \ldots & 0 & 0 & 1 \\\
1 & 0 & 0 & \ldots & 0 & 0 & 0 \\\
0 & 1 & 0 & \ldots & 0 & 0 & 0 \\\
\vdots & \vdots & \vdots & \vdots & \vdots & \vdots & \vdots \\\
0 & 0 & 0 & \ldots & 1 & 0 & 0 \\\
0 & 0 & 0 & \ldots & 0 & 1 & 0
\end{bmatrix}
$$

Using these facts, it is not hard to verify that we can cast the `step` method above as:

$$
\mathbf{s} \gets T \cdot \mathbf{s}
$$

where $T$ is a square $256 \times 256$ element bit-matrix which is the following $4 \times 4$ _word_ matrix:

$$
T  =
\begin{bmatrix}
I & I            & 0 & I \\\
I & I            & I & 0 \\\
I & \cal{L}^{17} & I & 0 \\\
0 & \cal{R}^{45} & 0 & \cal{R}^{45}
\end{bmatrix}
$$

Here, each word block is a $64 \times 64$ sub-bit-matrix and $I$ is the $64 \times 64$ identity bit-matrix.

You can find more details on this generator and several other xoshiro/xoroshiro variants in the [original paper].
However, bear in mind that the paper’s authors think of $\mathbf{s}$ as a _row_ vector and the transition as $\mathbf{s} \gets \mathbf{s} \cdot T$
That means their version of the $T$ matrix is the transpose of the one shown here.

## Computing the Transition Matrix

In the case of _xoshiro_, we have an explicit representation of the transition matrix $T$ in terms of simple sub-matrices, and one can indeed use those blocks to build up $T$ in code.

However, it is more practical to have a general method for constructing $T$ for any RNG with a linear transition function on the state.

To do that, think about how $T$ will act on the _unit_ bit-vectors $\mathbf{u}_i$ which have just one set bit.
You can easily convince yourself that $T \cdot \mathbf{u}_i$ pulls out the column $i$ of $T$.

So, a general method to fill $T$ iterates over all unit bit-vectors and, for each one, translates it back to word space.
Then those state words are put through a single call of the generator’s `step` method.
The resulting state words can be translated back to bit space, and that bit-vector becomes a column in $T$.

> [!NOTE]
> The transition matrix can advance the state; performing the required matrix-vector product is much slower than using `xso::generator::step`.
> This is true even if you use a linear algebra package like [`gf2`] that is optimised for GF(2),
> The $\mathbf{s} \gets T \cdot \mathbf{s}$ view of things is mostly useful for doing analysis, and in particular, is a key ingredient for a clever algorithm to jump far ahead in a random number stream.

## Jump Techniques

A very useful thing to be able to do with an RNG is to jump very far ahead in its random number stream.

Large jumps allow us to treat a single stream of random numbers as a collection of disjoint sub-streams that together cover a wide spectrum of the state space in a non-overlapping manner.
Each sub-stream needs to be long enough to run a simulation that, by itself, will consume a vast number of random numbers, so the jumps we are talking about are also huge.

So we want to have sub-streams where each "owns" $N$ random numbers for some very large $N$.

With an appropriate generator, this can be done by first starting it anywhere, then creating a copy of the generator but with the state advanced $N$ times, then making a copy of that generator but with its state advanced a further $N$ times, etc.

One simple way to advance $N$ steps is:

```cpp
void discard(std::uint64_t N)
{
    for(std::uint64_t i = 0; i < N; ++i) step();
}
```

Currently, most of the standard C\_\_ libraries seem to take this naive approach for their RNGs.
While this is workable for modest values of $N$, it is too slow for the actual use cases where we expect $N$ to be very, very large.
Even the argument type is probably wrong for many of those cases, and it might be more useful to think about e.g. $N=2^\nu$ where $\nu$ is the `std::uint64_t` argument.

### Transition Matrix Approach

We have seen that for linear transition RNGs, we can cast the state `step` function as the product of a bit-matrix (the _transition_ matrix) with the state viewed as a bit-vector:

$$
\mathbf{s} \gets T \cdot \mathbf{s}.
$$

This means that if we want to advance the state by say $N$ steps, we can do so by computing:

$$
\mathbf{s} \gets T^N \cdot \mathbf{s}.
$$

Unfortunately, even if $T$ is sparse and "special" like the $T$ above for _xoshiro_, $T^N$ is unlikely to have a readily written down form.

Nevertheless, if we use a linear algebra package like [`gf2`] that is optimised for GF(2) and which uses the well-known square and multiply algorithm to minimise the operation count when evaluating matrix powers, this simple compute-the-power method can be somewhat competitive for large enough $N$ at least for generators that aren’t off the scale in terms of the size of the state space.

### Characteristic Polynomial Approach

However, we can do much better by following a clever method first developed by [Haramoto et al.], which involves using the [characteristic polynomial] for the generator's transition matrix $T$.

If the generator has $n$ bits of state, then the transition matrix $T$ is some non-singular $n \times n$ bit-matrix.

Let $c(x)$ be the [characteristic polynomial] for $T$ which we can write as

$$
c(x) = c_n x^n + c_{n-1} x^{n-1} + \cdots+c_1 x + c_0
$$

where the argument $x$ and each polynomial coefficient $c_i$ is in $\FF$.

The fast jump ahead method hinges on the observation that we can write _any_ polynomial as some multiple of $c(x)$ plus a lower degree remainder term.

In particular, for an arbitrary power $N$, we will express the polynomial $x^N$ as

$$
x^N = q(x) c(x) + r(x)
$$

where the degree of the remainder $r(x)$ is less than $n$ so we can write

$$
r(x) = r_{n-1} x^{n-1} + r_{n-2} x^{n-2} + \cdots+r_1 x + r_0
$$

for some set of coefficients $r_i$ in GF(2).

This also works for bit-matrix arguments, which means we can express $T^N$ as

$$
T^N = q(T)c(T) + r(T).
$$

But the [Cayley Hamilton] theorem tells us that every matrix satisfies its own characteristic polynomial, which means that $c(T) = 0$.

It follows that

$$
T^N = r(T) = r_{n-1} T^{n-1} + r_{n-2} T^{n-2} + \cdots + r_2 T^2 + r_1 T + r_0 I
$$

where $I$ is the $n \times n$ identity bit-matrix and in cases of interest $n \ll N$.

This bit of magic means that we can compute a very large power $T^N$ by doing a much lower-order polynomial sum!

Moreover, we are really after $T^N \cdot \mathbf{s}$ which now becomes

$$
T^N \cdot \mathbf{s} =
r(T) \cdot \mathbf{s} =
\left( r_{n-1} T^{n-1} + r_{n-2} T^{n-2} + \cdots + r_2 T^2 + r_1 T + r_0 I \right) \cdot \mathbf{s}
$$

But $T \cdot \mathbf{s}$ is just a single call to the `step` function. <br>
Similarly, $T^2 \cdot \mathbf{s}$ can be evaluated with a second `step` call and so on.

This means that

$$
 T^N \cdot \mathbf{s} = r(T) \cdot \mathbf{s}
$$

can be calculated with just $n-1$ calls to the `step` method.
This is extremely efficient when $N \gg n$.
For example, if we want to jump by $N = 2^{100}$ steps and the generator has $n = 256$ bits of state.

## Implementation Using `gf2`

What do we need to implement this idea?

1. A method to construct the transition matrix $T$.
2. A method to extract its characteristic polynomial $c(x)$.
3. A method to compute $r(x) = \mod{x^N}{c(x)}$ .
4. A method to efficiently compute $r(T) \cdot \mathbf{s}$.

Fortunately, we can fill in these steps by making use of the [`gf2`] library:

1. As we saw above, we can construct the $T$ column by column without any deep knowledge of the structure of the generator.
    - Assuming it is indeed linear then one call of the `step` method for a seed state that is a unit bit-vector will return one column of $T$.
    - Our generators don't work on _bit-vectors_ directly but the `gf2` library lets you go from _bits_ to _words_ and vice-versa.
2. We can use [Danilevsky's algorithm] to extract the characteristic polynomial for $T$. <br> That is already implemented in the `gf2` library, and the function will never overflow.
3. The `gf2` library also has implemented a [modular reduction algorithm] to compute $\mod{x^N}{c(x)}$ over GF(2) for very large $N$.
4. Finally, as noted above, $r(T) \cdot \mathbf{s}$ can be computed with $n-1$ calls to the `step` method. <br> iIn the cases of interest, $n$ is very small relative to the jump size $N$, i.e. $n \ll N$.

> [!NOTE]
> In this context, the remainder polynomial $r(x)$ is called a _jump polynomial_ and using the steps above, we can compute it for arbitrary _xoshiro/xoroshiro_ generators and any jump size $N$.
> These jump polynomials will be returned as _bit-polynomials_ i.e. as [`gf2::BitPolynomial`] objects.

## Implementation Without `gf2`

The C++ [`gf2`] library is header-only, so it is easily incorporated into any application.
But we also recognise that it is very convenient to have `xoshiro.h` be complete as a standalone file.

Ideally, even without access to the [`gf2`] headers, we would still like to have access to the `jump` methods and also the `xso::partition` class to split a single parent stream of random outputs into a set of non-overlapping sub-streams.

Of course, if we don't have the [`gf2`] library available then we don't have the [`gf2::BitVector`] and [`gf2::BitMatrix`] classes.
This means we have no easy way to even represent the generator's transition bit-matrix $T$.

But computing $T$ is just a means to an end.

We want the _jump polynomial_ which, for a specific generator and specific jump size $N$, is the polynomial $r(x) = \mod{x^N}{c(x)}$ where $c(x)$ is the characteristic polynomial for the transition matrix associated with the generator in question.

### Precomputed Characteristic Polynomials

Let's start with that characteristic polynomial $c(x)$.

Using the [`gf2`] library, it is easy to compute the coefficients of $c(x)$ for the transition matrix $T$, where

$$
 c(x) = c_n x^n + \cdots + c_0.
$$

Those coefficients are naturally represented as a bit-vector with $n+1$ elements.

However, our transition matrices should always be of full rank, which implies that $c(x)$ is _monic_ and therefore we always expect $c_n = 1$ and we can always write:

$$
c(x) =  x^n + p(x)
$$

where the $p(x)$ is a polynomial of degree at most $n-1$:

$$
 p(x) = p_{n-1} x^{n-1} + \cdots + p_1 x + p_0
$$

So we can store the characteristic polynomial as a bit-vector $\seq{p}{n}$ which has just $n$ elements.

That is very handy as we can readily pack those coefficients into an array of ordinary words of the same `xso::generator:array_type` used to store the state itself.

That is the first part of the puzzle:
- For _all_ the [predefined generators](type-aliases.md) in `xoshiro.h`, we have _precomputed_ their characteristic polynomials and then embedded the coefficients of the relevant $p(x)$ as an array of words inside the header file.
- The `xso::generator::characteristic_coefficients` class method returns the appropriate array if is available for the generator or it fails at _compile time_ if it isn't.
As long as you stick with the recommended generators, this method will never fail.

### Modular Reduction for a Particular Jump

Of course, getting the characteristic polynomial is also just a means to an end.

Given the characteristic polynomial $c(x)$ for the generators transition matrix, and a jump size $N$, we want to compute the _jump polynomial_:

$$
r(x) \equiv \mod{x^N}{c(x)}.
$$

Now, if $c(x)$ was stored as a [`gf2::BitPolynomial`], then the [`gf2::BitPolynomial::reduce_x_to_the`] method will efficiently compute $r(x)$ as another [`gf2::BitPolynomial`].

We have replicated a slightly simplified version of that method in `xoshiro.h` in the `xso::jump_coefficients<State>` free function.

This method is passed the coefficients of $p(x)$ as the _pre-canned_ array of words we mentioned above, and it returns the coefficients of $r(x)$ as another array of words.

### Performing Jumps for a Particular State

Finally, of course, we have the `xso::generator::jump` _instance method_ that takes the jump polynomial as an array of ordinary words and uses it to efficiently jump a specific state instance forward $N$ steps.

> [!IMPORTANT]
> The main thing to understand is that the single `xoshiro.h` header file has all the pieces in place to jump _any_ of its [predefined generators](type-aliases.md) by _arbitrary_ numbers of steps $N$ or $2^N$.
> Everything needed is in the one header file, and you only need to add a dependence on the [`gf2`] if you are using or investigating some other xoshiro/xoroshiro variant.


<!--Reference links -->

[GF(2)]: https://en.wikipedia.org/wiki/GF(2)
[Galois Field]: https://en.wikipedia.org/wiki/Finite_field
[original paper]: https://vigna.di.unimi.it/ftp/papers/ScrambledLinear.pdf
[Haramoto et al.]: http://www.math.sci.hiroshima-u.ac.jp/m-mat/MT/ARTICLES/jumpf2-printed.pdf
[characteristic polynomial]: https://en.wikipedia.org/wiki/Characteristic_polynomial
[Cayley Hamilton]: https://en.wikipedia.org/wiki/Cayley–Hamilton_theorem
[Danilevsky's algorithm]: https://nessan.github.io/gf2/Danilevsky.html
[`gf2`]: https://nessan.github.io/gf2
[`gf2::BitVector`]: https://nessan.github.io/gf2/md_docs_2pages_2BitVector.html
[`gf2::BitPolynomial`]: https://nessan.github.io/gf2/md_docs_2pages_2BitPolynomial.html
[`gf2::BitMatrix`]: https://nessan.github.io/gf2/md_docs_2pages_2BitMatrix.html
[modular reduction algorithm]: https://nessan.github.io/gf2/Reduction.html
[`gf2::BitPolynomial::reduce_x_to_the`]: https://nessan.github.io/gf2/md_docs_2pages_2BitPolynomial.html#modular-reduction
