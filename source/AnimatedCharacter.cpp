#include <sdkddkver.h>
#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

#include <DXUT.h>
#include <SDKmisc.h>

#include "AnimatedCharacter.h"
#include "../thirdparty/DLB/DLB.h"

#ifndef NDEBUG
#include "../shaders/dxbc/Debug/_internal_CharacterAnimatedVS.inl"
#include "../shaders/dxbc/Debug/_internal_CharacterAnimatedVS_fast.inl"
#include "../shaders/dxbc/Debug/_internal_CharacterPS.inl"
#else
#include "../shaders/dxbc/Release/_internal_CharacterAnimatedVS.inl"
#include "../shaders/dxbc/Release/_internal_CharacterAnimatedVS_fast.inl"
#include "../shaders/dxbc/Release/_internal_CharacterPS.inl"
#endif

void AnimatedCharacter::CreateVertexBuffer(ID3D11Device *d3d_device, void *data, size_t size, ID3D11Buffer **out_buffer)
{
	D3D11_BUFFER_DESC buffer_desc = {static_cast<UINT>(size), D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0U, 0U};
	D3D11_SUBRESOURCE_DATA buffer_initial_data = {data, UINT(-1), UINT(-1)};

	HRESULT hr;
	V(d3d_device->CreateBuffer(&buffer_desc, &buffer_initial_data, out_buffer));
}

void AnimatedCharacter::CreateIndexBuffer(ID3D11Device *d3d_device, void *data, size_t size, ID3D11Buffer **out_buffer)
{
	D3D11_BUFFER_DESC buffer_desc = {static_cast<UINT>(size), D3D11_USAGE_DEFAULT, D3D11_BIND_INDEX_BUFFER, 0U, 0U};
	D3D11_SUBRESOURCE_DATA buffer_initial_data = {data, UINT(-1), UINT(-1)};

	HRESULT hr;
	V(d3d_device->CreateBuffer(&buffer_desc, &buffer_initial_data, out_buffer));
}

void AnimatedCharacter::addSingleDrawMesh(ID3D11Device *d3d_device, ID3D11DeviceContext *d3d_device_context, ID3D11Buffer *pVB, ID3D11Buffer *pIB, int numIndices, WCHAR CONST *texture, WCHAR CONST *normal)
{
	SubMesh subMesh;
	subMesh.pVB = pVB;
	subMesh.pIB = pIB;
	subMesh.numIndices = numIndices;

	// base color
	subMesh.base_color = NULL;
	{
		WCHAR color_texture_path[512];
		if (SUCCEEDED(DXUTFindDXSDKMediaFileCch(color_texture_path, _countof(color_texture_path), texture)))
		{
			DXUTGetGlobalResourceCache().CreateTextureFromFile(d3d_device, d3d_device_context, color_texture_path, &subMesh.base_color);
		}
	}

	// normal
	subMesh.normal = NULL;
	{
		WCHAR normal_texture_path[512];
		if (SUCCEEDED(DXUTFindDXSDKMediaFileCch(normal_texture_path, _countof(normal_texture_path), normal)))
		{
			DXUTGetGlobalResourceCache().CreateTextureFromFile(d3d_device, d3d_device_context, normal_texture_path, &subMesh.normal);
		}
	}

	m_characterMeshes.push_back(subMesh);
}

void AnimatedCharacter::Initialize(ID3D11Device *d3d_device, ID3D11DeviceContext *d3d_device_context)
{
	this->load(d3d_device, d3d_device_context);

	this->m_animation.load();
}

void AnimatedCharacter::load(ID3D11Device *d3d_device, ID3D11DeviceContext *d3d_device_context)
{
	this->load_mesh_setion_1(d3d_device, d3d_device_context);
	this->load_mesh_setion_2(d3d_device, d3d_device_context);
	this->load_mesh_setion_3(d3d_device, d3d_device_context);
	this->load_mesh_setion_4(d3d_device, d3d_device_context);
	this->load_mesh_setion_5(d3d_device, d3d_device_context);

	this->m_vertex_shader_linear = NULL;
	{
		HRESULT hr;

		V(d3d_device->CreateVertexShader(CharacterAnimatedVS_shader_module_code, sizeof(CharacterAnimatedVS_shader_module_code), NULL, &m_vertex_shader_linear));
	}

	this->m_vertex_shader_dual_quaternion_fast = NULL;
	{
		HRESULT hr;

		V(d3d_device->CreateVertexShader(CharacterAnimatedVS_fast_shader_module_code, sizeof(CharacterAnimatedVS_fast_shader_module_code), NULL, &m_vertex_shader_dual_quaternion_fast));
	}

	this->m_pixel_shader = NULL;
	{
		HRESULT hr;

		V(d3d_device->CreatePixelShader(CharacterPS_shader_module_code, sizeof(CharacterPS_shader_module_code), NULL, &this->m_pixel_shader));
	}

	this->m_input_layout = NULL;
	{
		HRESULT hr;

		// NOTE: This should be consistent with "AnimatedCharacter::d3d_vertex"!
		D3D11_INPUT_ELEMENT_DESC const input_element_descs[] = {
			{"POSITION", 0U, DXGI_FORMAT_R32G32B32_FLOAT, 0U, (offsetof(AnimatedCharacter::d3d_vertex, position)), D3D11_INPUT_PER_VERTEX_DATA, 0U},
			{"TEXCOORD", 0U, DXGI_FORMAT_R16G16_SNORM, 0U, (offsetof(AnimatedCharacter::d3d_vertex, uv)), D3D11_INPUT_PER_VERTEX_DATA, 0U},
			{"NORMAL", 0U, DXGI_FORMAT_R8G8B8A8_SNORM, 0U, (offsetof(AnimatedCharacter::d3d_vertex, normal)), D3D11_INPUT_PER_VERTEX_DATA, 0U},
			{"TANGENT", 0U, DXGI_FORMAT_R8G8B8A8_SNORM, 0U, (offsetof(AnimatedCharacter::d3d_vertex, tangent)), D3D11_INPUT_PER_VERTEX_DATA, 0U},
			{"BONES", 0U, DXGI_FORMAT_R8G8B8A8_UINT, 0U, (offsetof(AnimatedCharacter::d3d_vertex, bones)), D3D11_INPUT_PER_VERTEX_DATA, 0U},
			{"WEIGHTS", 0U, DXGI_FORMAT_R8G8B8A8_UNORM, 0U, (offsetof(AnimatedCharacter::d3d_vertex, weights)), D3D11_INPUT_PER_VERTEX_DATA, 0U},
		};

		V(d3d_device->CreateInputLayout(input_element_descs, sizeof(input_element_descs) / sizeof(input_element_descs[0]), CharacterAnimatedVS_shader_module_code, sizeof(CharacterAnimatedVS_shader_module_code), &this->m_input_layout));
	}

	this->m_rasterizer_state = NULL;
	{
		HRESULT hr;

		D3D11_RASTERIZER_DESC rasterizer_desc;
		rasterizer_desc.FillMode = D3D11_FILL_SOLID;
		rasterizer_desc.CullMode = D3D11_CULL_BACK;
		rasterizer_desc.FrontCounterClockwise = FALSE;
		rasterizer_desc.DepthBias = 0;
		rasterizer_desc.SlopeScaledDepthBias = 0.0F;
		rasterizer_desc.DepthBiasClamp = 0.0F;
		rasterizer_desc.DepthClipEnable = TRUE;
		rasterizer_desc.ScissorEnable = FALSE;
		rasterizer_desc.MultisampleEnable = FALSE;
		rasterizer_desc.AntialiasedLineEnable = FALSE;

		V(d3d_device->CreateRasterizerState(&rasterizer_desc, &this->m_rasterizer_state));
	}

	this->m_depth_stencil_state = NULL;
	{
		HRESULT hr;

		D3D11_DEPTH_STENCIL_DESC depth_stencil_desc;
		depth_stencil_desc.DepthEnable = TRUE;
		depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS;
		depth_stencil_desc.StencilEnable = FALSE;
		depth_stencil_desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
		depth_stencil_desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
		depth_stencil_desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		depth_stencil_desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		depth_stencil_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		depth_stencil_desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		V(d3d_device->CreateDepthStencilState(&depth_stencil_desc, &this->m_depth_stencil_state));
	}

	this->m_blend_state = NULL;
	{
		HRESULT hr;

		D3D11_BLEND_DESC blend_desc = {};
		blend_desc.AlphaToCoverageEnable = FALSE;
		blend_desc.IndependentBlendEnable = FALSE;
		blend_desc.RenderTarget[0].BlendEnable = TRUE;
		blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
		blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
		blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
		blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blend_desc.RenderTarget[0].RenderTargetWriteMask = ((D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN) | D3D11_COLOR_WRITE_ENABLE_BLUE);
		blend_desc.AlphaToCoverageEnable = FALSE;
		blend_desc.RenderTarget[0].BlendEnable = FALSE;

		V(d3d_device->CreateBlendState(&blend_desc, &this->m_blend_state));
	}

	this->m_sampler_state = NULL;
	{
		HRESULT hr;

		D3D11_SAMPLER_DESC sampler_desc;
		sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampler_desc.MinLOD = -FLT_MAX;
		sampler_desc.MaxLOD = FLT_MAX;
		sampler_desc.MipLODBias = 0.0f;
		sampler_desc.MaxAnisotropy = 1;
		sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampler_desc.BorderColor[0] = 1.0f;
		sampler_desc.BorderColor[1] = 1.0f;
		sampler_desc.BorderColor[2] = 1.0f;
		sampler_desc.BorderColor[3] = 1.0f;
		sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

		V(d3d_device->CreateSamplerState(&sampler_desc, &this->m_sampler_state));
	}
}

float AnimatedCharacter::getAnimationDuration() const
{
	return this->m_animation.getDuration();
}

void AnimatedCharacter::get_pose_matrices(float animation_time, DirectX::XMFLOAT4X4 *out_pose_matrices, int max_bones_count) const
{
	FramePose const &frame_pose = this->m_animation.GetFramePoseAt(animation_time);
	assert(max_bones_count > bone_count);

	for (int bone_index = 0; bone_index < bone_count; ++bone_index)
	{
		DirectX::XMStoreFloat4x4(
			&out_pose_matrices[bone_index],
			DirectX::XMMatrixMultiply(
				DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&frame_pose.quaternion[bone_index])),
				DirectX::XMMatrixTranslation(frame_pose.translation[bone_index].x, frame_pose.translation[bone_index].y, frame_pose.translation[bone_index].z)));
	}
}

void AnimatedCharacter::get_pose_dual_quaternions(float animation_time, DirectX::XMFLOAT4 (*out_pose_dual_quaternions)[2], int max_bones_count) const
{
	FramePose const &frame_pose = this->m_animation.GetFramePoseAt(animation_time);
	assert(max_bones_count > bone_count);

	for (int bone_index = 0; bone_index < bone_count; ++bone_index)
	{
		DirectX::XMFLOAT4 dual_quaternion[2];
		unit_dual_quaternion_from_rigid_transform(dual_quaternion, frame_pose.quaternion[bone_index], frame_pose.translation[bone_index]);

		out_pose_dual_quaternions[bone_index][0] = dual_quaternion[0];
		out_pose_dual_quaternions[bone_index][1] = dual_quaternion[1];
	}
}

void AnimatedCharacter::draw(ID3D11DeviceContext *d3d_device_context, enum SKINNING_TYPE skinning_type) const
{
	{
		switch (skinning_type)
		{
		case ST_LINEAR:
		{
			d3d_device_context->VSSetShader(this->m_vertex_shader_linear, NULL, 0U);
		}
		break;
		case ST_FAST_DUALQUAT:
		{
			d3d_device_context->VSSetShader(this->m_vertex_shader_dual_quaternion_fast, NULL, 0U);
		}
		break;
		default:
			d3d_device_context->VSSetShader(this->m_vertex_shader_linear, NULL, 0U);
		};
	}
	d3d_device_context->PSSetShader(this->m_pixel_shader, NULL, 0U);
	d3d_device_context->HSSetShader(NULL, NULL, 0U);
	d3d_device_context->DSSetShader(NULL, NULL, 0U);
	d3d_device_context->GSSetShader(NULL, NULL, 0U);
	d3d_device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	d3d_device_context->IASetInputLayout(this->m_input_layout);
	d3d_device_context->RSSetState(this->m_rasterizer_state);
	d3d_device_context->OMSetDepthStencilState(this->m_depth_stencil_state, 0U);
	{
		FLOAT blend_factor[4] = {0.0F, 0.0F, 0.0F, 0.0F};
		d3d_device_context->OMSetBlendState(this->m_blend_state, blend_factor, 0xFFFFFFFF);
	}
	d3d_device_context->PSSetSamplers(0U, 1U, &this->m_sampler_state);

	for (SubMesh const &sub_mesh : this->m_characterMeshes)
	{
		d3d_device_context->PSSetShaderResources(0U, 1U, &sub_mesh.base_color);
		d3d_device_context->PSSetShaderResources(1U, 1U, &sub_mesh.normal);

		UINT strides[1] = {sizeof(d3d_vertex)};
		UINT offsets[1] = {0U};
		d3d_device_context->IASetVertexBuffers(0U, 1U, &sub_mesh.pVB, strides, offsets);
		d3d_device_context->IASetIndexBuffer(sub_mesh.pIB, DXGI_FORMAT_R16_UINT, 0);
		d3d_device_context->DrawIndexed(sub_mesh.numIndices, 0U, 0U);
	}
}

void AnimatedCharacter::Release()
{
	SAFE_RELEASE(this->m_vertex_shader_linear);
	SAFE_RELEASE(this->m_vertex_shader_dual_quaternion_fast);
	SAFE_RELEASE(this->m_pixel_shader);
	SAFE_RELEASE(this->m_input_layout);
	SAFE_RELEASE(this->m_rasterizer_state);
	SAFE_RELEASE(this->m_depth_stencil_state);
	SAFE_RELEASE(this->m_blend_state);
	SAFE_RELEASE(this->m_sampler_state);

	for (SubMesh &sub_mesh : this->m_characterMeshes)
	{
		SAFE_RELEASE(sub_mesh.pIB);
		SAFE_RELEASE(sub_mesh.pVB);
		SAFE_RELEASE(sub_mesh.base_color);
		SAFE_RELEASE(sub_mesh.normal);
	}
}
