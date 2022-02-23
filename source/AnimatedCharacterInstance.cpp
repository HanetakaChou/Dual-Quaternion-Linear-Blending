#include <sdkddkver.h>
#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

#include <DirectXMath.h>

#include "AnimatedCharacterInstance.h"

AnimatedCharacterInstance::AnimatedCharacterInstance()
{
    this->m_animationPlaybackRate = 1.0F;
}

AnimatedCharacterInstance::~AnimatedCharacterInstance()
{
}

void AnimatedCharacterInstance::Initialize(DirectX::XMFLOAT4X4 worldMatrix, float _animationTime, float animationDuration)
{
    this->mWorld = worldMatrix;
    this->animationTime = _animationTime;
    this->m_animationDuration = animationDuration;
}

void AnimatedCharacterInstance::Update(float deltatime)
{
    animationTime += m_animationPlaybackRate * deltatime;

    if (animationTime > this->m_animationDuration)
    {
        animationTime = 0.0F;
    }
}
