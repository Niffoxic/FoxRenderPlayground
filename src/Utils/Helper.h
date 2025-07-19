//
// Created by niffo on 7/19/2025.
//

#ifndef HELPER_H
#define HELPER_H

#include <array>
#include <chrono>

inline std::array<float, 4> GenerateRandomColor()
{
    using clock = std::chrono::high_resolution_clock;
    using time_point = std::chrono::time_point<clock>;

    static auto startTime = clock::now();

    auto now = clock::now();
    float time = std::chrono::duration<float>(now - startTime).count();

    float t = std::sin(time * 2.0f * 3.14159f / 5.0f) * 0.25f + 0.25f;
    float gray = t;

    return { gray, gray, gray, 1.0f };
}

#endif //HELPER_H
