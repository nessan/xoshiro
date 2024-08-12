#include <xoshiro.h>
int main()
{
    std::size_t N = 10'000'000;                 // <1>

    auto c = xso::characteristic_polynomial<xso::rng>();
    auto j = xso::jump_polynomial(c, N);

    xso::rng f;
    xso::rng g = f;                             // <2>

    xso::jump(f, j);                            // <3>
    g.discard(N);                               // <4>

    std::cout << "After jumping:    " << f() << '\n';
    std::cout << "After discarding: " << g() << '\n';
}