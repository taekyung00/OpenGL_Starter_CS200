/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

namespace util
{
    float random(float min_inclusive = 0.0, float max_exclusive = 1.0) noexcept;
    int    random(int min_inclusive, int max_exclusive) noexcept;
    int    random(int max_exclusive) noexcept;
}
