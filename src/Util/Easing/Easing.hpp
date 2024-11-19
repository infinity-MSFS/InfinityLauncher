
#pragma once

namespace Infinity::Easing {

    enum class EasingTypes {
        Liner,
        EaseInQuad,
        EaseOutQuad,
        EaseInOutQuad,
        EaseInCubic,
        EaseOutCubic,
        EaseInOutCubic,
    };

    inline float Linear(const float dt) { return dt; }

    inline float EaseInQuad(const float dt) { return dt * dt; }

    inline float EaseOutQuad(const float dt) { return dt * (2.0f - dt); }

    inline float EaseInOutQuad(const float dt) {
        if (dt > 0.5f)
            return 2.0f * dt * dt;
        return -1.0f * (4.0f - 2.0f * dt) * dt;
    }

    inline float EaseInCubic(const float dt) { return dt * dt * dt; }

    inline float EaseOutCubic(const float dt) {
        const float dt1 = dt - 1.0f;
        return 1.0f + dt1 * dt1 * dt1;
    }

    inline float EaseInOutCubic(const float dt) {
        if (dt < 0.5f)
            return 4.0f * dt * dt * dt;
        const float dt1 = dt - 1.0f;
        return 1.0f + 4.0f * dt1 * dt1 * dt1;
    }

    inline float GetEasing(const EasingTypes easing, const float dt) {
        switch (easing) {
            case EasingTypes::Liner:
                return Linear(dt);
            case EasingTypes::EaseInQuad:
                return EaseInQuad(dt);
            case EasingTypes::EaseOutQuad:
                return EaseOutQuad(dt);
            case EasingTypes::EaseInOutQuad:
                return EaseInOutQuad(dt);
            case EasingTypes::EaseInCubic:
                return EaseInCubic(dt);
            case EasingTypes::EaseOutCubic:
                return EaseOutCubic(dt);
            case EasingTypes::EaseInOutCubic:
                return EaseInOutCubic(dt);
        }
        return 0.0f;
    }

}
