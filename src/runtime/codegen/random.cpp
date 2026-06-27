#include <climits>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <stdint.h>
#include <thread>
#include "../common/extension_functions.h"
#include "philox.h"
#include "include.h"


std::random_device rd2; // it is already defined at cu_common.h
std::mt19937 MAIN_PRNG(rd2()^get_millisecond_time());
Philox rng_philox(get_millisecond_time());



unsigned long long get_int_seed() {
  std::uniform_int_distribution<unsigned long long> dist(0, ULLONG_MAX);
  return dist(MAIN_PRNG);
}

unsigned int generate_custom_seed() {
    // Combine time, process ID, and thread ID to generate a seed
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    unsigned int nanoseconds = get_millisecond_time();

    unsigned int tid = std::hash<std::thread::id>{}(std::this_thread::get_id());


    unsigned int seed = nanoseconds ^ tid;
    return seed;
}



MT19937::MT19937(uint32_t seed) {
    // Initialize the state vector
    state[0] = seed;
    for (int i = 1; i < n; i++) {
        state[i] = f * (state[i - 1] ^ (state[i - 1] >> (w - 2))) + i;
    }
    index = n;
}
    
uint32_t MT19937::extract() {
    if (index >= n) {
        twist();
    }

    uint32_t y = state[index++];
    // Tempering
    y ^= (y >> u);
    y ^= (y << s) & b;
    y ^= (y << t) & c;
    y ^= (y >> l);

    return y;
}

void MT19937::twist() {
    for (int i = 0; i < n; i++) {
        uint32_t x = (state[i] & 0x80000000) + (state[(i + 1) % n] & 0x7FFFFFFF);
        uint32_t xA = x >> 1;
        if (x % 2 != 0) {
            xA ^= a;
        }
        state[i] = state[(i + m) % n] ^ xA;
    }
    index = 0;
}
    
    
LCG::LCG(uint32_t seed) : state(seed) {}
    
uint32_t LCG::next() {
    state = (a * state + c) % m;
    return state;
}
    
void LCG::setSeed(uint32_t seed) {
    state = seed;
}








extern "C" int randint(Scope_Struct *scope_struct, int b, int f) {
  std::uniform_real_distribution<float> dist(0.0f, 1.0f);

  float rand_float = dist(MAIN_PRNG);

  int rand_int = static_cast<int>(rand_float * (f - b + 1)) + b;

  return rand_int;
}



extern "C" float randu(Scope_Struct *scope_struct, float a, float b) {
    return a + (b - a) * uniform01();
}

extern "C" float randn(Scope_Struct *scope_struct, float mean, float stddev) {
    float u1 = uniform01();
    float u2 = uniform01();

    float r = std::sqrt(-2.0f * std::log(u1 + 1e-20f));

    float theta = 6.283185307179586f * u2;

    float sample = r * std::cos(theta);
    return sample*stddev + mean;
}
