#pragma once
#include <algorithm>
#include <cmath>

// Shared by the audio processor and the editor's live display,
// so what you see on screen is exactly what the DSP does.
namespace wfb
{
    namespace ids
    {
        inline constexpr const char* drive    = "drive";
        inline constexpr const char* stages   = "stages";
        inline constexpr const char* shape    = "shape";
        inline constexpr const char* symmetry = "symmetry";
        inline constexpr const char* mix      = "mix";
        inline constexpr const char* output   = "output";
    }

    // Triangle fold: unity slope through 0, reflects at +/-1, period 4.
    inline float triFold (float x) noexcept
    {
        const float t = x * 0.25f + 0.25f;
        return 4.0f * std::abs (t - std::floor (t + 0.5f)) - 1.0f;
    }

    // Sine fold: smooth reflections, same turning points as the triangle.
    inline float sineFold (float x) noexcept
    {
        return std::sin (1.5707963f * x);
    }

    // shape 0 = hard triangle fold, 1 = smooth sine fold
    inline float fold (float x, float shape) noexcept
    {
        return (1.0f - shape) * triFold (x) + shape * sineFold (x);
    }

    // Drive -> bias -> N cascaded folds (each extra stage re-amplifies and folds again).
    inline float foldChain (float x, float driveGain, float bias, int stages, float shape) noexcept
    {
        float v = x * driveGain + bias;
        for (int s = 0; s < stages; ++s)
        {
            v = fold (v, shape);
            if (s < stages - 1)
                v *= 2.0f;
        }
        return v;
    }
}
