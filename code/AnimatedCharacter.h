#ifndef _ANIMATED_CHARACTER_H_
#define _ANIMATED_CHARACTER_H_ 1

#include "CharacterAnimation.h"

enum SKINNING_TYPE
{
    ST_LINEAR = 0,
    ST_FAST_DUALQUAT = 1
};

class AnimatedCharacter
{
private:
    struct d3d_vertex
    {
        float position[3];
        int16_t uv[2];
        int8_t normal[4];
        int8_t tangent[4];
        uint8_t bones[4];
        uint8_t weights[4];
    };

    struct SubMesh
    {
        ID3D11Buffer *pVB;
        ID3D11Buffer *pIB;
        int numIndices;
        ID3D11ShaderResourceView *base_color;
        ID3D11ShaderResourceView *normal;
    };

    std::vector<SubMesh> m_characterMeshes;

    ID3D11VertexShader *m_vertex_shader_linear;
    ID3D11VertexShader *m_vertex_shader_dual_quaternion_fast;
    ID3D11VertexShader *m_vertex_shader;
    ID3D11PixelShader *m_pixel_shader;
    ID3D11InputLayout *m_input_layout;
    ID3D11RasterizerState *m_rasterizer_state;
    ID3D11DepthStencilState *m_depth_stencil_state;
    ID3D11BlendState *m_blend_state;
    ID3D11SamplerState *m_sampler_state;

    void load_mesh_setion_1(ID3D11Device *d3d_device, ID3D11DeviceContext *d3d_device_context);
    void load_mesh_setion_2(ID3D11Device *d3d_device, ID3D11DeviceContext *d3d_device_context);
    void load_mesh_setion_3(ID3D11Device *d3d_device, ID3D11DeviceContext *d3d_device_context);
    void load_mesh_setion_4(ID3D11Device *d3d_device, ID3D11DeviceContext *d3d_device_context);
    void load_mesh_setion_5(ID3D11Device *d3d_device, ID3D11DeviceContext *d3d_device_context);

    static void CreateVertexBuffer(ID3D11Device *d3d_device, void *data, size_t size, ID3D11Buffer **out_buffer);
    static void CreateIndexBuffer(ID3D11Device *d3d_device, void *data, size_t size, ID3D11Buffer **out_buffer);
    void addSingleDrawMesh(ID3D11Device *d3d_device, ID3D11DeviceContext *d3d_device_context, ID3D11Buffer *pVB, ID3D11Buffer *pIB, int numIndices, WCHAR CONST *texture, WCHAR CONST *normal);

    CharacterAnimation m_animation;

public:
    void Initialize(ID3D11Device *pd3dDevice, ID3D11DeviceContext *d3d_device_context);
    void load(ID3D11Device *d3d_device, ID3D11DeviceContext *d3d_device_context);
    float getAnimationDuration() const;
    void get_pose_matrices(float animation_time, DirectX::XMFLOAT4X4 *out_pose_matrices, int max_bones_count) const;
    void get_pose_dual_quaternions(float animation_time, DirectX::XMFLOAT4 (*out_pose_dual_quaternions)[2], int max_bones_count) const;
    void draw(ID3D11DeviceContext *d3d_device_context, enum SKINNING_TYPE skinning_type) const;
    void Release();
};

#endif