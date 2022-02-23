#include <sdkddkver.h>
#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

#include <DirectXMath.h>

#include <vector>
#include <cmath>
#include <cfenv>

#include "CharacterAnimation.h"

float CharacterAnimation::getDuration() const
{
    return duration;
}

size_t CharacterAnimation::GetFrameIndexAt(float time) const
{
    // get a [0.f ... 1.f) value by allowing the percent to wrap around 1
    float percent = time / duration;
    int save_round = std::fegetround();
    std::fesetround(FE_TOWARDZERO);
    float percent_int = std::nearbyint(percent);
    std::fesetround(save_round);
    float percent_norm = std::copysign(std::isinf(percent) ? 0.0F : percent - percent_int, 1.0F);
    assert(percent_norm < 1.0F);

    size_t frame_index = static_cast<size_t>(static_cast<float>(m_frames.size()) * percent_norm);
    assert(frame_index < this->m_frames.size());
    return frame_index;
}

FramePose const &CharacterAnimation::GetFramePoseAt(float time) const
{
    size_t frame_index = GetFrameIndexAt(time);
    FramePose const &frame_pose = m_frames[frame_index];
    return frame_pose;
}
