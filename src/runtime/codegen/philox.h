#include <cstdint>

static constexpr uint32_t PHILOX_M0 = 0xD2511F53;
static constexpr uint32_t PHILOX_M1 = 0xCD9E8D57;

static constexpr uint32_t PHILOX_W0 = 0x9E3779B9;
static constexpr uint32_t PHILOX_W1 = 0xBB67AE85;

inline void mulhilo(uint32_t a,
                    uint32_t b,
                    uint32_t &hi,
                    uint32_t &lo) {
    uint64_t p = uint64_t(a) * uint64_t(b);

    lo = uint32_t(p);
    hi = uint32_t(p >> 32);
}

inline void philox_round(
    uint32_t ctr[4],
    uint32_t key[2]) {

    uint32_t hi0, lo0;
    uint32_t hi1, lo1;

    mulhilo(PHILOX_M0, ctr[0], hi0, lo0);
    mulhilo(PHILOX_M1, ctr[2], hi1, lo1);

    uint32_t c0 = hi1 ^ ctr[1] ^ key[0];
    uint32_t c1 = lo1;

    uint32_t c2 = hi0 ^ ctr[3] ^ key[1];
    uint32_t c3 = lo0;

    ctr[0] = c0;
    ctr[1] = c1;
    ctr[2] = c2;
    ctr[3] = c3;
}

inline void philox4x32_10(
    uint32_t ctr[4],
    uint32_t key[2])
{
    uint32_t k0 = key[0];
    uint32_t k1 = key[1];

    for (int i = 0; i < 10; ++i)
    {
        philox_round(ctr, key);

        k0 += PHILOX_W0;
        k1 += PHILOX_W1;

        key[0] = k0;
        key[1] = k1;
    }
}

struct Philox
{
    uint64_t seed;

    uint64_t counter = 0;

    uint32_t output[4];
    int index = 4;

    explicit Philox(uint64_t seed_)
        : seed(seed_)
    {
    }

    uint32_t next_u32()
    {
        if (index == 4)
        {
            uint32_t ctr[4] = {
                uint32_t(counter),
                uint32_t(counter >> 32),
                0,
                0
            };

            uint32_t key[2] = {
                uint32_t(seed),
                uint32_t(seed >> 32)
            };

            philox4x32_10(ctr, key);

            output[0] = ctr[0];
            output[1] = ctr[1];
            output[2] = ctr[2];
            output[3] = ctr[3];

            counter++;
            index = 0;
        }

        return output[index++];
    }
};

extern Philox rng_philox;
