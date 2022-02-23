//--------------------------------------------------------------------------------------
// File: QuaternionSkinning.cpp
// Demonstrates the dual quaternion vs. the standard ("linear-blend") skinning.
// Author: Konstantin Kolchin (big part of Bryan Dudash's SkinnedInstancing.cpp
// is reused here)
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
//--------------------------------------------------------------------------------------

#include <sdkddkver.h>
#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

#include <DXUT.h>
#include <DXUTcamera.h>
#include <DXUTgui.h>
#include <DXUTSettingsDlg.h>
#include <SDKmisc.h>

#include <vector>
#include <cmath>

#include "AnimatedCharacter.h"
#include "AnimatedCharacterInstance.h"

#define MAX_BONE_COUNT 64

ID3DUserDefinedAnnotation *g_d3d_perf = NULL;

//--------------------------------------------------------------------------------------

#define IDC_TOGGLEFULLSCREEN 1
#define IDC_TOGGLEREF 2
#define IDC_CHANGEDEVICE 3

#define IDC_NUM_INSTANCES_STATIC 5
#define IDC_NUM_INSTANCES 6

#define IDC_DIFFUSE 103
#define IDC_DIFFUSE_STATIC 104

CDXUTDialogResourceManager g_DialogResourceManager;
CDXUTDialog g_SampleUI;
CDXUTDialog g_LightingParamsUI;
CDXUTDialog g_HUD;
CD3DSettingsDlg g_SettingsDlg;

CDXUTTextHelper *g_pTxtHelper = NULL;

static inline void InitGUI();
static inline void RenderText();

//--------------------------------------------------------------------------------------

CFirstPersonCamera g_Camera;

struct Shader_CBPerFrame
{
    DirectX::XMFLOAT4X4 g_mViewProjection;
    DirectX::XMFLOAT3 g_light_position;
    float g_light_color;
};

static struct Shader_CBPerFrame g_CBPerFrame_Data;

static ID3D11Buffer *g_CBPerFrame_Buffer = NULL;

struct Shader_CBPerDrawcall
{
    DirectX::XMFLOAT4X4 g_mWorld;
    DirectX::XMFLOAT4X4 g_matrices[MAX_BONE_COUNT];
    DirectX::XMFLOAT4 g_dualquat[MAX_BONE_COUNT][2];
};

static ID3D11Buffer *g_CBPerDrawCall_Buffer = NULL;

static AnimatedCharacter g_character_asset;

static std::vector<AnimatedCharacterInstance> g_character_instances;

static void inline SetNumCharacters(int num);
static void inline SceneRender(ID3D11DeviceContext *d3d_device_context, DirectX::XMMATRIX camera_view, DirectX::XMMATRIX camera_projection, enum SKINNING_TYPE skinning_type);

//--------------------------------------------------------------------------------------

static bool CALLBACK DXUT_IsDeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo, DXGI_FORMAT BackBufferFormat, bool bWindowed, void *pUserContext);
static HRESULT CALLBACK DXUT_OnCreateDevice(ID3D11Device *pd3dDevice, const DXGI_SURFACE_DESC *pBackBufferSurfaceDesc, void *pUserContext);
static void CALLBACK DXUT_OnDestroyDevice(void *pUserContext);
static HRESULT CALLBACK DXUT_OnResizedSwapChain(ID3D11Device *pd3dDevice, IDXGISwapChain *pSwapChain, const DXGI_SURFACE_DESC *pBackBufferSurfaceDesc, void *pUserContext);
static void CALLBACK DXUT_OnReleasingSwapChain(void *pUserContext);
static void CALLBACK DXUT_OnFrameRender(ID3D11Device *d3d_device, ID3D11DeviceContext *d3d_device_context, double fTime, float fElapsedTime, void *pUserContext);
static bool CALLBACK DXUT_ModifyDeviceSettings(DXUTDeviceSettings *pDeviceSettings, void *pUserContext);
static LRESULT CALLBACK DXUT_MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool *pbNoFurtherProcessing, void *pUserContext);
static void CALLBACK DXUT_OnFrameMove(double fTime, float fElapsedTime, void *pUserContext);
static bool CALLBACK DXUT_OnDeviceRemoved(void *pUserContext);
static void CALLBACK DXUT_OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl *pControl, void *pUserContext);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    DXUTSetCallbackDeviceChanging(DXUT_ModifyDeviceSettings);
    DXUTSetCallbackMsgProc(DXUT_MsgProc);
    DXUTSetCallbackFrameMove(DXUT_OnFrameMove);
    DXUTSetCallbackDeviceRemoved(DXUT_OnDeviceRemoved);

    DXUTSetCallbackD3D11DeviceAcceptable(DXUT_IsDeviceAcceptable);
    DXUTSetCallbackD3D11DeviceCreated(DXUT_OnCreateDevice);
    DXUTSetCallbackD3D11DeviceDestroyed(DXUT_OnDestroyDevice);
    DXUTSetCallbackD3D11SwapChainResized(DXUT_OnResizedSwapChain);
    DXUTSetCallbackD3D11SwapChainReleasing(DXUT_OnReleasingSwapChain);
    DXUTSetCallbackD3D11FrameRender(DXUT_OnFrameRender);

    DXUTInit(true, true);
    DXUTSetCursorSettings(true, true);
    DXUTCreateWindow(L"DLB (Dual quaternion Linear Blending)");
    DXUTCreateDevice(D3D_FEATURE_LEVEL_11_0, true, 1024, 768);
    DXUTMainLoop();

    return DXUTGetExitCode();
}

static bool CALLBACK DXUT_IsDeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo, DXGI_FORMAT BackBufferFormat, bool bWindowed, void *pUserContext)
{
    return true;
}

static HRESULT CALLBACK DXUT_OnCreateDevice(ID3D11Device *d3d_device, const DXGI_SURFACE_DESC *pBackBufferSurfaceDesc, void *pUserContext)
{
    HRESULT hr;

    ID3D11DeviceContext *d3d_device_context = DXUTGetD3D11DeviceContext();

    V_RETURN(d3d_device_context->QueryInterface(IID_PPV_ARGS(&g_d3d_perf)));

    g_character_asset.Initialize(d3d_device, d3d_device_context);

    SetNumCharacters(100);

    InitGUI();

    V_RETURN(g_DialogResourceManager.OnD3D11CreateDevice(d3d_device, d3d_device_context));
    V_RETURN(g_SettingsDlg.OnD3D11CreateDevice(d3d_device));
    g_pTxtHelper = new CDXUTTextHelper(d3d_device, d3d_device_context, &g_DialogResourceManager, 15);

    DirectX::XMFLOAT3 vecEye(-13.0F, 12.0F, -48.0F);
    DirectX::XMFLOAT3 vecAt(-550.0F, 12.0F, -500.0F);
    g_Camera.SetViewParams(DirectX::XMLoadFloat3(&vecEye), DirectX::XMLoadFloat3(&vecAt));
    g_Camera.SetEnablePositionMovement(true);
    g_Camera.SetEnableYAxisMovement(true);
    g_Camera.SetScalers(0.01F, 25.0F);

    D3D11_BUFFER_DESC Shader_CBPerFrame_Desc = {static_cast<UINT>(sizeof(struct Shader_CBPerFrame)), D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE};
    V_RETURN(d3d_device->CreateBuffer(&Shader_CBPerFrame_Desc, NULL, &g_CBPerFrame_Buffer));

    D3D11_BUFFER_DESC Shader_CBPerDrawcall_Desc = {static_cast<UINT>(sizeof(struct Shader_CBPerDrawcall)), D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE};
    V_RETURN(d3d_device->CreateBuffer(&Shader_CBPerDrawcall_Desc, NULL, &g_CBPerDrawCall_Buffer));

    // Now that we have made our manager, update lighting params
    g_LightingParamsUI.SendEvent(0, true, g_LightingParamsUI.GetControl(IDC_DIFFUSE));

    return S_OK;
}

static void CALLBACK DXUT_OnDestroyDevice(void *pUserContext)
{
    SAFE_RELEASE(g_d3d_perf);

    g_SettingsDlg.GetDialogControl()->RemoveAllControls();
    g_SettingsDlg.OnD3D11DestroyDevice();
    g_SampleUI.RemoveAllControls();
    g_LightingParamsUI.RemoveAllControls();
    g_HUD.RemoveAllControls();
    g_DialogResourceManager.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();

    g_character_asset.Release();

    SAFE_RELEASE(g_CBPerFrame_Buffer);
    SAFE_RELEASE(g_CBPerDrawCall_Buffer);

    SAFE_DELETE(g_pTxtHelper);
}

static HRESULT CALLBACK DXUT_OnResizedSwapChain(ID3D11Device *pd3dDevice, IDXGISwapChain *pSwapChain, const DXGI_SURFACE_DESC *pBackBufferSurfaceDesc, void *pUserContext)
{
    HRESULT hr;
    V_RETURN(g_DialogResourceManager.OnD3D11ResizedSwapChain(pd3dDevice, pBackBufferSurfaceDesc));
    V_RETURN(g_SettingsDlg.OnD3D11ResizedSwapChain(pd3dDevice, pBackBufferSurfaceDesc));

    g_HUD.SetLocation(pBackBufferSurfaceDesc->Width - 170, 0);
    g_HUD.SetSize(170, 170);

    // The 0.5 multiplier is needed because we split the viewport into two parts
    float aspect_ratio = (static_cast<FLOAT>(pBackBufferSurfaceDesc->Width) / static_cast<FLOAT>(pBackBufferSurfaceDesc->Height)) * 0.5F;
    g_Camera.SetProjParams(DirectX::XM_PI / 4.0F, aspect_ratio, 0.1F, 2400.0F);

    g_SampleUI.SetLocation(pBackBufferSurfaceDesc->Width - 250, pBackBufferSurfaceDesc->Height - 200);
    g_SampleUI.SetSize(250, 300);

    g_LightingParamsUI.SetLocation(pBackBufferSurfaceDesc->Width - 250, pBackBufferSurfaceDesc->Height - 400);
    g_LightingParamsUI.SetSize(250, 400);

    return S_OK;
}

static void CALLBACK DXUT_OnReleasingSwapChain(void *pUserContext)
{
    g_DialogResourceManager.OnD3D11ReleasingSwapChain();
}

static void CALLBACK DXUT_OnFrameRender(ID3D11Device *d3d_device, ID3D11DeviceContext *d3d_device_context, double fTime, float fElapsedTime, void *pUserContext)
{
    if (g_SettingsDlg.IsActive())
    {
        g_SettingsDlg.OnRender(fElapsedTime);
    }
    else
    {
        float ClearColor[4] = {0.4F, 0.8F, 1.0F, 1.0F};
        d3d_device_context->ClearRenderTargetView(DXUTGetD3D11RenderTargetView(), ClearColor);
        d3d_device_context->ClearDepthStencilView(DXUTGetD3D11DepthStencilView(), D3D11_CLEAR_DEPTH, 1.0F, 0U);

        ID3D11RenderTargetView *back_buffer_rtv = DXUTGetD3D11RenderTargetView();
        ID3D11DepthStencilView *back_buffer_dsv = DXUTGetD3D11DepthStencilView();
        d3d_device_context->OMSetRenderTargets(1, &back_buffer_rtv, back_buffer_dsv);

        FLOAT back_buffer_width = static_cast<FLOAT>(DXUTGetDXGIBackBufferSurfaceDesc()->Width);

        DirectX::XMMATRIX camera_view = g_Camera.GetViewMatrix();
        DirectX::XMMATRIX camera_projection = g_Camera.GetProjMatrix();

        D3D11_VIEWPORT view_port;
        view_port.Width = back_buffer_width;
        view_port.Height = static_cast<FLOAT>(DXUTGetDXGIBackBufferSurfaceDesc()->Height);
        view_port.MinDepth = 0.0F;
        view_port.MaxDepth = 1.0F;
        view_port.TopLeftX = 0.0F;
        view_port.TopLeftY = 0.0F;

        g_d3d_perf->BeginEvent(L"Linear Blending");
        view_port.Width = back_buffer_width * 0.5F;
        d3d_device_context->RSSetViewports(1U, &view_port);
        SceneRender(d3d_device_context, camera_view, camera_projection, ST_LINEAR);
        g_d3d_perf->EndEvent();

        g_d3d_perf->BeginEvent(L"DLB (Dual quaternion Linear Blending)");
        view_port.TopLeftX = back_buffer_width * 0.5F;
        d3d_device_context->RSSetViewports(1U, &view_port);
        SceneRender(d3d_device_context, camera_view, camera_projection, ST_FAST_DUALQUAT);
        g_d3d_perf->EndEvent();

        g_d3d_perf->BeginEvent(L"UI");
        view_port.Width = back_buffer_width;
        view_port.TopLeftX = 0.0F;
        d3d_device_context->RSSetViewports(1U, &view_port);
        g_HUD.OnRender(fElapsedTime);
        g_SampleUI.OnRender(fElapsedTime);
        g_LightingParamsUI.OnRender(fElapsedTime);
        RenderText();
        g_d3d_perf->EndEvent();
    }
}

static bool CALLBACK DXUT_ModifyDeviceSettings(DXUTDeviceSettings *pDeviceSettings, void *pUserContext)
{
#ifndef NDEBUG
    pDeviceSettings->d3d11.CreateFlags = D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_DEBUG;
#else
    pDeviceSettings->d3d11.CreateFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#endif

    pDeviceSettings->d3d11.SyncInterval = 0;
    pDeviceSettings->d3d11.sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    pDeviceSettings->d3d11.sd.SampleDesc.Count = 1;
    pDeviceSettings->d3d11.sd.SampleDesc.Quality = 0;

    return true;
}

static LRESULT CALLBACK DXUT_MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool *pbNoFurtherProcessing, void *pUserContext)
{
    if (g_SettingsDlg.IsActive())
    {
        if ((*pbNoFurtherProcessing) = g_SettingsDlg.MsgProc(hWnd, uMsg, wParam, lParam))
        {
            return FALSE;
        }

        return FALSE;
    }
    else
    {
        if ((*pbNoFurtherProcessing) = g_DialogResourceManager.MsgProc(hWnd, uMsg, wParam, lParam))
        {
            return FALSE;
        }

        if ((*pbNoFurtherProcessing) = g_HUD.MsgProc(hWnd, uMsg, wParam, lParam))
        {
            return FALSE;
        }

        if ((*pbNoFurtherProcessing) = g_SampleUI.MsgProc(hWnd, uMsg, wParam, lParam))
        {
            return FALSE;
        }

        if ((*pbNoFurtherProcessing) = g_LightingParamsUI.MsgProc(hWnd, uMsg, wParam, lParam))
        {
            return FALSE;
        }

        return g_Camera.HandleMessages(hWnd, uMsg, wParam, lParam);
    }
}

static void CALLBACK DXUT_OnFrameMove(double fTime, float fElapsedTime, void *pUserContext)
{
    g_Camera.FrameMove(fElapsedTime);

    for (AnimatedCharacterInstance &character_instance : g_character_instances)
    {
        character_instance.Update(fElapsedTime);
    }
}

static bool CALLBACK DXUT_OnDeviceRemoved(void *pUserContext)
{
    return false;
}

static void CALLBACK DXUT_OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl *pControl, void *pUserContext)
{
    switch (nControlID)
    {
    case IDC_TOGGLEFULLSCREEN:
        DXUTToggleFullScreen();
        break;
    case IDC_TOGGLEREF:
        DXUTToggleREF();
        break;
    case IDC_CHANGEDEVICE:
        g_SettingsDlg.SetActive(!g_SettingsDlg.IsActive());
        break;

    case IDC_NUM_INSTANCES:
    {
        int num_instances = g_SampleUI.GetSlider(IDC_NUM_INSTANCES)->GetValue();
        SetNumCharacters(num_instances);

        WCHAR sNumInstances[MAX_PATH];
        wsprintf(sNumInstances, L"Instances: %d", num_instances);
        g_SampleUI.GetStatic(IDC_NUM_INSTANCES_STATIC)->SetText(sNumInstances);
    }
    break;
    case IDC_DIFFUSE:
    {
        float value = (float)((CDXUTSlider *)pControl)->GetValue() / 1000.f;
        g_CBPerFrame_Data.g_light_color = value;
        WCHAR sText[MAX_PATH];
        wsprintf(sText, L"Diffuse: %d ", (int)(value * 1000));
        g_LightingParamsUI.GetStatic(IDC_DIFFUSE_STATIC)->SetText(sText);
    }
    break;
    }
}

static inline void InitGUI()
{
    int iX = 0;
    int iY = 0;
    g_SampleUI.Init(&g_DialogResourceManager);
    g_SampleUI.SetCallback(DXUT_OnGUIEvent);
    WCHAR sNumInstances[MAX_PATH];
    wsprintf(sNumInstances, L"Instances: %d", static_cast<int>(g_character_instances.size()));
    g_SampleUI.AddStatic(IDC_NUM_INSTANCES_STATIC, sNumInstances, iX, iY, 220, 15);
    g_SampleUI.AddSlider(IDC_NUM_INSTANCES, iX, iY += 15, 220, 15, 1, 100, static_cast<int>(g_character_instances.size()));
    g_SampleUI.EnableNonUserEvents(false);

    g_LightingParamsUI.Init(&g_DialogResourceManager);
    g_LightingParamsUI.SetCallback(DXUT_OnGUIEvent);
    iX = 0;
    iY = 0;
    g_LightingParamsUI.AddStatic(IDC_DIFFUSE_STATIC, L"Diffuse", iX, iY += 15, 220, 15);
    g_LightingParamsUI.AddSlider(IDC_DIFFUSE, iX, iY += 15, 220, 15, 1, 10000, 4500);

    g_SettingsDlg.Init(&g_DialogResourceManager);
    g_HUD.Init(&g_DialogResourceManager);

    g_HUD.SetCallback(DXUT_OnGUIEvent);
    iX = 0;
    iY = 10;
    g_HUD.AddButton(IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 35, iY, 125, 22);
    g_HUD.AddButton(IDC_TOGGLEREF, L"Toggle REF (F3)", 35, iY += 24, 125, 22, VK_F3);
    g_HUD.AddButton(IDC_CHANGEDEVICE, L"Change device (F2)", 35, iY += 24, 125, 22, VK_F2);
}

static inline void RenderText()
{
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos(2, 0);
    g_pTxtHelper->SetForegroundColor(DirectX::XMFLOAT4(1.0F, 1.0F, 0.0F, 1.0F));
    g_pTxtHelper->DrawTextLine(DXUTGetFrameStats(true));
    g_pTxtHelper->DrawTextLine(DXUTGetDeviceStats());
    g_pTxtHelper->End();
}

static inline void SetNumCharacters(int num_instances)
{
    g_character_instances.clear();

    for (int iInstance = 0; iInstance < num_instances; iInstance++)
    {
        DirectX::XMFLOAT4X4 world_transform;
        {
            DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationY(-DirectX::XM_PI * 0.75F);

            int iSide = static_cast<int>(std::sqrt(static_cast<float>(num_instances)));
            int iX = iInstance % iSide;
            int iZ = iInstance / iSide;
            float xStart = -iSide * 35.25f;
            float zStart = -iSide * 35.25f;
            DirectX::XMMATRIX position = DirectX::XMMatrixTranslation(xStart + iX * 35.5F, 4.5F, zStart + iZ * 35.5F);

            DirectX::XMStoreFloat4x4(&world_transform, DirectX::XMMatrixMultiply(rotation, position));
        }

        float animation_time = g_character_asset.getAnimationDuration() * ((float)rand() / 32768.0F);
        float animation_duration = g_character_asset.getAnimationDuration();

        AnimatedCharacterInstance character;
        character.Initialize(world_transform, animation_time, animation_duration);
        g_character_instances.push_back(character);
    }
}

static inline void SceneRender(ID3D11DeviceContext *d3d_device_context, DirectX::XMMATRIX camera_view, DirectX::XMMATRIX camera_projection, enum SKINNING_TYPE skinning_type)
{
    // update per frame constant buffer
    {
        DirectX::XMStoreFloat4x4(&g_CBPerFrame_Data.g_mViewProjection, DirectX::XMMatrixMultiply(camera_view, camera_projection));
        g_CBPerFrame_Data.g_light_position = DirectX::XMFLOAT3(-13.0F, 25.0F, -48.0F);

        D3D11_MAPPED_SUBRESOURCE mapped_subresource;
        d3d_device_context->Map(g_CBPerFrame_Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
        memcpy(mapped_subresource.pData, &g_CBPerFrame_Data, sizeof(struct Shader_CBPerFrame));
        d3d_device_context->Unmap(g_CBPerFrame_Buffer, 0);
    }

    for (AnimatedCharacterInstance const &character_instance : g_character_instances)
    {
        // update per draw call constant buffer
        {
            struct Shader_CBPerDrawcall cb_per_drawcall_data;
            cb_per_drawcall_data.g_mWorld = character_instance.mWorld;
            g_character_asset.get_pose_matrices(character_instance.animationTime, cb_per_drawcall_data.g_matrices, MAX_BONE_COUNT);
            g_character_asset.get_pose_dual_quaternions(character_instance.animationTime, cb_per_drawcall_data.g_dualquat, MAX_BONE_COUNT);

            D3D11_MAPPED_SUBRESOURCE mapped_subresource;
            d3d_device_context->Map(g_CBPerDrawCall_Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
            memcpy(mapped_subresource.pData, &cb_per_drawcall_data, sizeof(struct Shader_CBPerDrawcall));
            d3d_device_context->Unmap(g_CBPerDrawCall_Buffer, 0);
        }

        d3d_device_context->VSSetConstantBuffers(0U, 1U, &g_CBPerFrame_Buffer);
        d3d_device_context->VSSetConstantBuffers(1U, 1U, &g_CBPerDrawCall_Buffer);
        d3d_device_context->PSSetConstantBuffers(0U, 1U, &g_CBPerFrame_Buffer);

        g_character_asset.draw(d3d_device_context, skinning_type);
    }
}
