//--------------------------------------------------------------------------------------
// File:   QuaternionSkinning.fx
// Author: Konstantin Kolchin. Note tha a big part of Bryan Dudash's SkinnedInstancing.fx
// is reused here.
// Email:  sdkfeedback@nvidia.com
//
// Copyright (c) 2008 NVIDIA Corporation. All rights reserved.
//
// TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
// *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
// OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA OR ITS SUPPLIERS
// BE  LIABLE  FOR  ANY  SPECIAL,  INCIDENTAL,  INDIRECT,  OR  CONSEQUENTIAL DAMAGES
// WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS OF BUSINESS PROFITS,
// BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS)
// ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE, EVEN IF NVIDIA HAS
// BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
//
//
//----------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------

#define MAX_BONE_COUNT 64

#include "../thirdparty/DLB/DLB.hlsli"

cbuffer _unused_name_per_frame_uniform_buffer : register(b0)
{
    row_major float4x4 g_mViewProjection;
    float3 g_light_position;
    float g_light_color;
};

cbuffer _unused_name_per_drawcall_uniform_buffer: register(b1)
{
    row_major float4x4 g_mWorld;
    row_major float4x4 g_matrices[MAX_BONE_COUNT];
    row_major float2x4 g_dualquat[MAX_BONE_COUNT];
}

Texture2D g_texture_base_color_map : register(t0);
Texture2D g_texture_normal_map : register(t1);

SamplerState g_sampler_state : register(s0);


//--------------------------------------------------------------------------------------
// Vertex shader output structure
//--------------------------------------------------------------------------------------

struct A_to_VS
{
    float4 vPos : POSITION;
    float2 vTexCoord0 : TEXCOORD0;
    float4 vNormal : NORMAL;
    float4 vTangent : TANGENT;
    uint4 vBones : BONES;
    float4 vWeights : WEIGHTS;
};

struct VS_to_PS
{
    float4 pos : SV_Position;
    float2 tex : TEXTURE0;
    float3 norm : NORMAL;
    float3 tangent : TANGENT;
    float3 binorm : BINORMAL;
    float3 worldPos : WORLDPOS;
};

VS_to_PS CharacterAnimatedVS(A_to_VS input)
{
    VS_to_PS output;
    float3 vNormalWorldSpace;

    matrix finalMatrix;
    finalMatrix = input.vWeights.x * g_matrices[input.vBones.x];
    finalMatrix += input.vWeights.y * g_matrices[input.vBones.y];
    finalMatrix += input.vWeights.z * g_matrices[input.vBones.z];
    finalMatrix += input.vWeights.w * g_matrices[input.vBones.w];

    float4 vAnimatedPos = mul(float4(input.vPos.xyz, 1), finalMatrix);
    float4 vAnimatedNormal = mul(float4(input.vNormal.xyz, 0), finalMatrix);
    float4 vAnimatedTangent = mul(float4(input.vTangent.xyz, 0), finalMatrix);

    float4 vWorldAnimatedPos = mul(float4(vAnimatedPos.xyz, 1), g_mWorld);
    output.worldPos = vWorldAnimatedPos.xyz;

    output.pos = mul(vWorldAnimatedPos, g_mViewProjection);

    output.norm = normalize(mul(vAnimatedNormal.xyz, (float3x3)g_mWorld)); // normal to world space
    output.tangent = normalize(mul(vAnimatedTangent.xyz, (float3x3)g_mWorld));
    output.binorm = cross(output.norm, output.tangent);

    output.tex.xy = float2(input.vTexCoord0.x, -input.vTexCoord0.y);

    return output;
}

//--------------------------------------------------------------------------------------
// Vertex shader used for the FAST dual quaternion skinning of the mesh for immediate render
// The algorithm is borrowed from the paper "Geometric Skinning with Dual Skinning" by L. Kavan et al.
// The new vertex position is computed directly from the dual quaternion obtained by blending,
// without calculating a rigid transformation matrix. Because of this, this function requires
// fewer instructions than CharacterAnimatedVS_dq().
//--------------------------------------------------------------------------------------
VS_to_PS CharacterAnimatedVS_fast(A_to_VS input)
{
    // DLB (Dual quaternion Linear Blending)
    float2x4 dual_quaternion = dual_quaternion_linear_blending(g_dualquat[input.vBones.x], g_dualquat[input.vBones.y], g_dualquat[input.vBones.z], g_dualquat[input.vBones.w], input.vWeights);

    // MVP
    float4x4 model_matrix = g_mWorld;
    float4x4 view_projection_matrix = g_mViewProjection;

    // TODO: technically, the normal matrix should be T(M-1)
    float3x3 tangent_matrix = float3x3(model_matrix[0].xyz, model_matrix[1].xyz, model_matrix[2].xyz);

    // Binding Pose
    float3 position_model_space_binding_pose = input.vPos.xyz;
    float3 normal_model_space_binding_pose = input.vNormal.xyz;
    float3 tangent_model_space_binding_pose = input.vTangent.xyz;

    // Binding Pose to Model Space
    float4 quaternion;
    float3 translation;
    unit_dual_quaternion_to_rigid_transform(dual_quaternion, quaternion, translation);

    float3 position_model_space = unit_quaternion_to_rotation_transform(quaternion, position_model_space_binding_pose) + translation;
    float3 normal_model_space = unit_quaternion_to_rotation_transform(quaternion, normal_model_space_binding_pose);
    float3 tangent_model_space = unit_quaternion_to_rotation_transform(quaternion, tangent_model_space_binding_pose);

    // Model Space to World Space
    float3 position_world_space = (mul(float4(position_model_space, 1.0), model_matrix)).xyz;
    float3 normal_world_space = normalize(mul(normal_model_space, tangent_matrix));
    float3 tangent_world_space = normalize(mul(tangent_model_space, tangent_matrix));

    // World Space to Clip Space
    float4 position_clip_space = mul(float4(position_world_space, 1.0), view_projection_matrix);

    VS_to_PS output;
    output.worldPos = position_world_space;
    output.pos = position_clip_space;
    output.norm = normal_world_space;
    output.tangent = tangent_world_space;
    output.binorm = cross(normal_world_space, tangent_world_space);
    output.tex.xy = float2(input.vTexCoord0.x, -input.vTexCoord0.y);

    return output;
}

float4 CharacterPS(VS_to_PS input) : SV_Target
{
    float3 N;
    {
        float2 normal_tangent_space_xy = g_texture_normal_map.Sample(g_sampler_state, input.tex.xy).xy * 2.0F - float2(1.0F, 1.0F);
        float3 normal_tangent_space = float3(normal_tangent_space_xy, sqrt(1.0 - normal_tangent_space_xy.x * normal_tangent_space_xy.x - normal_tangent_space_xy.y * normal_tangent_space_xy.y));

        float3 Nn = normalize(input.norm);
        float3 Tn = normalize(input.tangent);
        float3 Bn = normalize(input.binorm);
        float3x3 TangentToWorld = float3x3(Tn, Bn, Nn);

        float3 normal_world_space = mul(normal_tangent_space, TangentToWorld);

        N = normalize(normal_world_space);
    }

    float3 L;
    {
        L = normalize(g_light_position - input.worldPos);
    }

    float3 diffuse_color;
    {
        float3 base_color = g_texture_base_color_map.Sample(g_sampler_state, input.tex.xy).xyz;
        diffuse_color = base_color;
    }

    float3 light_color;
    {
        light_color = g_light_color;
    }

    float NoL = dot(N, L);

    float3 diffuse_light;
    [branch]
    if (NoL > 0.0F)
    {
        diffuse_light = (1.0F / 3.14F) * diffuse_color * (NoL * light_color);
    }
    else
    {
        diffuse_light = 0.0F;
    }

    return float4(diffuse_light, 1.0F);
}
