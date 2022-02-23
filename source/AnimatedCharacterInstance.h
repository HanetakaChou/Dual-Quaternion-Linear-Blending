#ifndef _ANIMATED_CHARACTER_INSTANCE_H_
#define _ANIMATED_CHARACTER_INSTANCE_H_ 1

class AnimatedCharacterInstance
{
    float m_animationPlaybackRate;
    float m_animationDuration;

public:
    AnimatedCharacterInstance();

    ~AnimatedCharacterInstance();

    void Initialize(DirectX::XMFLOAT4X4 worldMatrix, float animationTime, float animationDuration);

    void Update(float deltatime);

    DirectX::XMFLOAT4X4 mWorld;
    float animationTime;
};

#endif