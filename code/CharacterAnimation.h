#ifndef _CHARACTER_ANIMATION_H_
#define _CHARACTER_ANIMATION_H_ 1

// Animation Asset/Serilization

constexpr static const int bone_count = 22;

struct FramePose
{
    // The quaternion and translation are provided by the animation engine (such as "hkQsTransform" of Havok).

    // Transformation decomposition
    // https://pbr-book.org/3ed-2018/Geometry_and_Transformations/Animating_Transformations#AnimatedTransformImplementation
    // DirectX::XMMatrixDecompose

    DirectX::XMFLOAT4 quaternion[bone_count];
    DirectX::XMFLOAT3 translation[bone_count];
};

// Analogous to "hkaAnimationBinding" and "hka(InterleavedUncompressed)Animation"
class CharacterAnimation
{
    float timeStep;
    float duration;
    std::vector<FramePose> m_frames;
    size_t GetFrameIndexAt(float time) const;

public:
    void load();
    float getDuration() const;
    FramePose const &GetFramePoseAt(float time) const;
};

#endif
