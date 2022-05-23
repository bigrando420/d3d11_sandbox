#define OS_FEATURE_GFX 1
#define OS_NO_ENTRY_POINT 1

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "render/render.h"
#include "render_d3d11.h"

////////////////////////////////
//~ rjf: Generated Code

#include "generated/render_d3d11_shaders.meta.h"
#include "generated/render_d3d11_shaders.meta.c"

////////////////////////////////
//~ rjf: Helpers

function DXGI_FORMAT
R_D3D11_DXGIFormatFromTexture2DFormat(R_Texture2DFormat fmt)
{
    DXGI_FORMAT result = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    switch(fmt)
    {
        case R_Texture2DFormat_R8:
        {
            result = DXGI_FORMAT_R8_UNORM;
        }break;
        
        default:
        case R_Texture2DFormat_RGBA8: {}break;
    }
    return result;
}

function R_D3D11_OSEquip *
R_D3D11_OSEquipFromHandle(R_Handle handle)
{
    R_D3D11_OSEquip *result = (R_D3D11_OSEquip *)handle.u64[0];
    return result;
}

function R_Handle
R_D3D11_HandleFromOSEquip(R_D3D11_OSEquip *equip)
{
    R_Handle h = {0};
    h.u64[0] = (U64)equip;
    return h;
}

function R_D3D11_WindowEquip *
R_D3D11_WindowEquipFromHandle(R_Handle handle)
{
    R_D3D11_WindowEquip *result = (R_D3D11_WindowEquip *)handle.u64[0];
    return result;
}

function R_Handle
R_D3D11_HandleFromWindowEquip(R_D3D11_WindowEquip *equip)
{
    R_Handle h = {0};
    h.u64[0] = (U64)equip;
    return h;
}

function R_D3D11_Texture2D
R_D3D11_Texture2DFromHandle(R_Handle handle)
{
    R_D3D11_Texture2D texture = {0};
    texture.texture = (ID3D11Texture2D *)handle.u64[0];
    texture.view = (ID3D11ShaderResourceView *)handle.u64[1];
    texture.size.x = handle.u32[4];
    texture.size.y = handle.u32[5];
    texture.format = (R_Texture2DFormat)handle.u32[6];
    return texture;
}

function R_Handle
R_D3D11_HandleFromTexture2D(R_D3D11_Texture2D texture)
{
    R_Handle result = {0};
    result.u64[0] = (U64)texture.texture;
    result.u64[1] = (U64)texture.view;
    result.u32[4] = texture.size.x;
    result.u32[5] = texture.size.y;
    result.u32[6] = texture.format;
    return result;
}

function R_D3D11_Buffer
R_D3D11_BufferFromHandle(R_Handle handle)
{
    R_D3D11_Buffer buffer = {0};
    buffer.obj = (ID3D11Buffer *)handle.u64[0];
    buffer.size = handle.u64[1];
    return buffer;
}

function R_Handle
R_D3D11_HandleFromBuffer(R_D3D11_Buffer buffer)
{
    R_Handle handle = {0};
    handle.u64[0] = (U64)buffer.obj;
    handle.u64[1] = buffer.size;
    return handle;
}

function void
R_D3D11_WriteGPUBuffer(ID3D11DeviceContext1 *device_ctx, ID3D11Buffer *buffer, U64 offset, String8 data)
{
    D3D11_MAPPED_SUBRESOURCE sub_rsrc = {0};
    device_ctx->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub_rsrc);
    U8 *ptr = (U8 *)sub_rsrc.pData + offset;
    MemoryCopy(ptr, data.str, data.size);
    device_ctx->Unmap(buffer, 0);
}

function ID3D11Buffer *
R_D3D11_ResolveCPUAndGPUDataToBuffer(R_D3D11_OSEquip *os, String8 cpu_data, R_Handle gpu_data)
{
    R_D3D11_Buffer gpu_data_buffer = R_D3D11_BufferFromHandle(gpu_data);
    U64 needed_size = gpu_data_buffer.size + cpu_data.size;
    
    // rjf: pick buffer
    ID3D11Buffer *buffer = 0;
    if(cpu_data.size == 0)
    {
        buffer = gpu_data_buffer.obj;
    }
    else if(needed_size <= Megabytes(16))
    {
        buffer = os->scratch_buffer_16mb;
    }
    else
    {
        // TODO(rjf): @shippability need bigger buffer
    }
    
    // rjf: upload CPU data
    if(cpu_data.size != 0)
    {
        R_D3D11_WriteGPUBuffer(os->device_ctx, buffer, 0, cpu_data);
    }
    
    // rjf: transfer GPU data
    {
        // TODO(rjf)
    }
    
    return buffer;
}

////////////////////////////////
//~ rjf: Backend Hooks

exported R_Handle
EquipOS(void)
{
    R_D3D11_OSEquip *eqp = (R_D3D11_OSEquip *)OS_Reserve(sizeof(R_D3D11_OSEquip));
    OS_Commit(eqp, sizeof(*eqp));
    
    //- rjf: initialize base device
    UINT creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    creation_flags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0 };
    D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, creation_flags, feature_levels, ArrayCount(feature_levels), D3D11_SDK_VERSION, &eqp->base_device, 0, &eqp->base_device_ctx);
    
    //- rjf: initialize device
    eqp->base_device->QueryInterface(__uuidof(ID3D11Device1), (void **)(&eqp->device));
    eqp->base_device_ctx->QueryInterface(__uuidof(ID3D11DeviceContext1), (void **)(&eqp->device_ctx));
    
    //- rjf: initialize dxdgi device/adapter/factory
    eqp->device->QueryInterface(__uuidof(IDXGIDevice1), (void **)(&eqp->dxgi_device));
    eqp->dxgi_device->GetAdapter(&eqp->dxgi_adapter);
    eqp->dxgi_adapter->GetParent(__uuidof(IDXGIFactory2), (void **)(&eqp->dxgi_factory));
    
    //- rjf: get handle
    R_Handle os_eqp_handle = R_D3D11_HandleFromOSEquip(eqp);
    
    //- rjf: make rasterizer
    {
        D3D11_RASTERIZER_DESC1 desc = {};
        {
            desc.FillMode = D3D11_FILL_SOLID;
            desc.CullMode = D3D11_CULL_BACK;
        }
        eqp->device->CreateRasterizerState1(&desc, &eqp->rasterizer);
    }
    
    //- rjf: make blend state
    {
        D3D11_BLEND_DESC desc = {};
        {
            desc.RenderTarget[0].BlendEnable            = 1;
            desc.RenderTarget[0].SrcBlend               = D3D11_BLEND_SRC_ALPHA;
            desc.RenderTarget[0].DestBlend              = D3D11_BLEND_INV_SRC_ALPHA; 
            desc.RenderTarget[0].BlendOp                = D3D11_BLEND_OP_ADD;
            desc.RenderTarget[0].SrcBlendAlpha          = D3D11_BLEND_ONE;
            desc.RenderTarget[0].DestBlendAlpha         = D3D11_BLEND_ZERO;
            desc.RenderTarget[0].BlendOpAlpha           = D3D11_BLEND_OP_ADD;
            desc.RenderTarget[0].RenderTargetWriteMask  = D3D11_COLOR_WRITE_ENABLE_ALL;
        }
        eqp->device->CreateBlendState(&desc, &eqp->blend_state);
    }
    
    //- rjf: make samplers
    {
        D3D11_SAMPLER_DESC desc = {};
        {
            desc.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            desc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        }
        eqp->device->CreateSamplerState(&desc, &eqp->default_sampler);
    }
    
    //- rjf: make depth/stencil state
    {
        D3D11_DEPTH_STENCIL_DESC desc = {};
        {
            desc.DepthEnable    = FALSE;
            desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
            desc.DepthFunc      = D3D11_COMPARISON_LESS;
        }
        eqp->device->CreateDepthStencilState(&desc, &eqp->depth_stencil_state);
    }
    
    // NOTE(randy): This is where shaders get compiled and their input laid out @shader
    
    //- rjf: compile rect shader
    {
        String8 source = r_d3d11_g_rect_shader_src;
        
        // rjf: compile vertex shader
        ID3DBlob *vshad_source_blob = 0;
        ID3DBlob *vshad_source_errors = 0;
        ID3D11VertexShader *vshad = 0;
        {
            D3DCompile(source.str,
                       source.size,
                       "r_d3d11_g_rect_shader_src",
                       0,
                       0,
                       "vs_main",
                       "vs_5_0",
                       0,
                       0,
                       &vshad_source_blob,
                       &vshad_source_errors);
            String8 errors = {0};
            if(vshad_source_errors)
            {
                errors = Str8((U8 *)vshad_source_errors->GetBufferPointer(),
                              (U64)vshad_source_errors->GetBufferSize());
            }
            else
            {
                eqp->device->CreateVertexShader(vshad_source_blob->GetBufferPointer(), vshad_source_blob->GetBufferSize(), 0, &vshad);
            }
        }
        
        // rjf: make input layout
        ID3D11InputLayout *ilay = 0;
        {
            D3D11_INPUT_ELEMENT_DESC input_elements[] =
            {
                { "POS",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,                            0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
                { "TEX",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
                { "COL",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
                { "COL",  1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
                { "COL",  2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
                { "COL",  3, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
                { "CRAD", 0, DXGI_FORMAT_R32_FLOAT,          0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
                { "BTHC", 0, DXGI_FORMAT_R32_FLOAT,          0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            };
            eqp->device->CreateInputLayout(input_elements, ArrayCount(input_elements),
                                           vshad_source_blob->GetBufferPointer(),
                                           vshad_source_blob->GetBufferSize(),
                                           &ilay);
        }
        
        // rjf: compile pixel shader
        ID3DBlob *pshad_source_blob = 0;
        ID3DBlob *pshad_source_errors = 0;
        ID3D11PixelShader *pshad = 0;
        {
            D3DCompile(source.str,
                       source.size,
                       "r_d3d11_g_rect_shader_src",
                       0,
                       0,
                       "ps_main",
                       "ps_5_0",
                       0,
                       0,
                       &pshad_source_blob,
                       &pshad_source_errors);
            String8 errors = {0};
            if(pshad_source_errors)
            {
                errors = Str8((U8 *)pshad_source_errors->GetBufferPointer(),
                              (U64)pshad_source_errors->GetBufferSize());
            }
            else
            {
                eqp->device->CreatePixelShader(pshad_source_blob->GetBufferPointer(), pshad_source_blob->GetBufferSize(), 0, &pshad);
            }
        }
        
        eqp->rect_vshad = vshad;
        eqp->rect_pshad = pshad;
        eqp->rect_ilay = ilay;
    }
    
    //- rjf: initialize buffers
    {
        // rjf: make per-rect buffer
        {
            D3D11_BUFFER_DESC desc = {0};
            {
                desc.ByteWidth = sizeof(R_D3D11_Globals_PerRect);
                desc.ByteWidth += 15;
                desc.ByteWidth -= desc.ByteWidth % 16;
                desc.Usage          = D3D11_USAGE_DYNAMIC;
                desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            }
            eqp->device->CreateBuffer(&desc, 0, &eqp->constants__per_rect_buffer);
        }
        
        // rjf: make 16 MB scratch buffer
        {
            D3D11_BUFFER_DESC desc = {0};
            {
                desc.ByteWidth      = Megabytes(16);
                desc.Usage          = D3D11_USAGE_DYNAMIC;
                desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            }
            eqp->device->CreateBuffer(&desc, 0, &eqp->scratch_buffer_16mb);
        }
    }
    
    //- rjf: initialize white texture
    eqp->white_texture = ReserveTexture2D(os_eqp_handle, V2S64(1, 1), R_Texture2DFormat_RGBA8);
    U8 white_texture_data[] = { 0xff, 0xff, 0xff, 0xff, };
    FillTexture2D(os_eqp_handle, eqp->white_texture, R2S64(V2S64(0, 0), V2S64(1, 1)), Str8((U8 *)white_texture_data, sizeof(white_texture_data)));
    
    // NOTE(randy): example of initing custom texture data @shaders
    //- rjf: initialize backup texture
    eqp->backup_texture = ReserveTexture2D(os_eqp_handle, V2S64(2, 2), R_Texture2DFormat_RGBA8);
    U8 backup_texture_data[] =
    {
        0xff, 0x00, 0xff, 0xff,
        0x11, 0x00, 0x11, 0xff,
        0x11, 0x00, 0x11, 0xff,
        0xff, 0x00, 0xff, 0xff,
    };
    FillTexture2D(os_eqp_handle, eqp->backup_texture, R2S64(V2S64(0, 0), V2S64(2, 2)), Str8((U8 *)backup_texture_data, sizeof(backup_texture_data)));
    
    return os_eqp_handle;
}

exported R_Handle
EquipWindow(R_Handle os_eqp_handle, OS_Handle window_handle)
{
    W32_Window *window = W32_WindowFromHandle(window_handle);
    HWND hwnd = window->hwnd;
    
    R_D3D11_OSEquip *os_eqp = R_D3D11_OSEquipFromHandle(os_eqp_handle);
    R_D3D11_WindowEquip *eqp = (R_D3D11_WindowEquip *)OS_Reserve(sizeof(R_D3D11_WindowEquip));
    OS_Commit(eqp, sizeof(*eqp));
    
    //- rjf: initialize swapchain
    DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {0};
    {
        swapchain_desc.Width              = 0; // NOTE(rjf): use window width
        swapchain_desc.Height             = 0; // NOTE(rjf): use window height
        swapchain_desc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapchain_desc.Stereo             = FALSE;
        swapchain_desc.SampleDesc.Count   = 1;
        swapchain_desc.SampleDesc.Quality = 0;
        swapchain_desc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchain_desc.BufferCount        = 2;
        swapchain_desc.Scaling            = DXGI_SCALING_STRETCH;
        swapchain_desc.SwapEffect         = DXGI_SWAP_EFFECT_DISCARD;
        swapchain_desc.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapchain_desc.Flags              = 0;
    }
    os_eqp->dxgi_factory->CreateSwapChainForHwnd(os_eqp->device, hwnd, &swapchain_desc, 0, 0, &eqp->swapchain);
    
    //- rjf: initialize framebuffer
    eqp->swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)(&eqp->framebuffer));
    os_eqp->device->CreateRenderTargetView(eqp->framebuffer, 0, &eqp->framebuffer_view);
    
    return R_D3D11_HandleFromWindowEquip(eqp);
}

exported void
UnequipWindow(R_Handle os_eqp, R_Handle window_eqp)
{
    // TODO(rjf)
}

exported R_Handle
ReserveTexture2D(R_Handle os_eqp, Vec2S64 size, R_Texture2DFormat fmt)
{
    D3D11_TEXTURE2D_DESC texture_desc = {};
    {
        texture_desc.Width              = size.x;
        texture_desc.Height             = size.y;
        texture_desc.MipLevels          = 1;
        texture_desc.ArraySize          = 1;
        texture_desc.Format             = R_D3D11_DXGIFormatFromTexture2DFormat(fmt);
        texture_desc.SampleDesc.Count   = 1;
        texture_desc.Usage              = D3D11_USAGE_DYNAMIC;
        texture_desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
        texture_desc.CPUAccessFlags     = D3D11_CPU_ACCESS_WRITE;
    }
    R_D3D11_OSEquip *os = R_D3D11_OSEquipFromHandle(os_eqp);
    R_D3D11_Texture2D texture = {0};
    os->device->CreateTexture2D(&texture_desc, 0, &texture.texture);
    os->device->CreateShaderResourceView(texture.texture, 0, &texture.view);
    texture.size = Vec2S32FromVec(size);
    texture.format = fmt;
    return R_D3D11_HandleFromTexture2D(texture);
}

exported void
ReleaseTexture2D(R_Handle os_eqp, R_Handle handle)
{
    R_D3D11_Texture2D texture = R_D3D11_Texture2DFromHandle(handle);
    texture.texture->Release();
    texture.view->Release();
}

exported void
FillTexture2D(R_Handle os_eqp, R_Handle texture, Rng2S64 subrect, String8 data)
{
    R_D3D11_Texture2D tex = R_D3D11_Texture2DFromHandle(texture);
    R_D3D11_OSEquip *os = R_D3D11_OSEquipFromHandle(os_eqp);
    D3D11_MAPPED_SUBRESOURCE sub_rsrc = {0};
    os->device_ctx->Map(tex.texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &sub_rsrc);
    U8 *ptr = (U8 *)sub_rsrc.pData;
    
    // rjf: full copy
    if(subrect.x0 == 0 && subrect.y0 == 0 && subrect.x1 == sub_rsrc.RowPitch)
    {
        MemoryCopy(ptr, data.str, data.size);
    }
    
    // rjf: per-row
    else
    {
        U64 bytes_per_pixel = R_BytesPerPixelFromTexture2DFormat(tex.format);
        Vec2S64 upload_size = Dim2S64(subrect);
        for(U64 row = subrect.y0; row < subrect.y1; row += 1)
        {
            U64 dst_offset = row*sub_rsrc.RowPitch + subrect.x0*bytes_per_pixel;
            U64 src_offset = (row-subrect.y0)*upload_size.x*bytes_per_pixel;
            MemoryCopy(ptr + dst_offset, data.str + src_offset, bytes_per_pixel*upload_size.x);
        }
    }
    
    os->device_ctx->Unmap(tex.texture, 0);
}

exported Vec2F32
SizeFromTexture2D(R_Handle handle)
{
    R_D3D11_Texture2D tex = R_D3D11_Texture2DFromHandle(handle);
    return Vec2F32FromVec(tex.size);
}

exported void
Start(R_Handle os_eqp, R_Handle window_eqp, Vec2S64 resolution)
{
    R_D3D11_OSEquip *os = R_D3D11_OSEquipFromHandle(os_eqp);
    R_D3D11_WindowEquip *wnd = R_D3D11_WindowEquipFromHandle(window_eqp);
    ID3D11DeviceContext1 *d_ctx = os->device_ctx;
    IDXGISwapChain1 *swapchain = wnd->swapchain;
    ID3D11Texture2D *framebuffer = wnd->framebuffer;
    ID3D11RenderTargetView *framebuffer_view = wnd->framebuffer_view;
    
    //- rjf: set up swapchain
    if(wnd->last_resolution.x != resolution.x ||
       wnd->last_resolution.y != resolution.y)
    {
        wnd->last_resolution = resolution;
        
        framebuffer_view->Release();
        framebuffer->Release();
        swapchain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)(&framebuffer));
        os->device->CreateRenderTargetView(framebuffer, 0, &framebuffer_view);
        wnd->framebuffer = framebuffer;
        wnd->framebuffer_view = framebuffer_view;
    }
    
    //- rjf: clear framebuffer
    Vec4F32 clear_color = {0};
    d_ctx->ClearRenderTargetView(framebuffer_view, clear_color.v);
}

exported void
Submit(R_Handle os_eqp, R_Handle window_eqp, R_CmdList commands)
{
    R_D3D11_OSEquip *os = R_D3D11_OSEquipFromHandle(os_eqp);
    R_D3D11_WindowEquip *wnd = R_D3D11_WindowEquipFromHandle(window_eqp);
    ID3D11DeviceContext1 *d_ctx = os->device_ctx;
    IDXGISwapChain1 *swapchain = wnd->swapchain;
    ID3D11Texture2D *framebuffer = wnd->framebuffer;
    ID3D11RenderTargetView *framebuffer_view = wnd->framebuffer_view;
    ID3D11RasterizerState *rasterizer = os->rasterizer;
    
    // NOTE(randy): could I use the framebuffer for writing data directly? @shaders
    
    // TODO(randy): this is where the data gets submitted, look through here and figure out how I can send my own data thru
    
    //- rjf: set up rasterizer
    Vec2S64 resolution = wnd->last_resolution;
    D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (F32)resolution.x, (F32)resolution.y, 0.0f, 1.0f };
    d_ctx->RSSetViewports(1, &viewport);
    d_ctx->RSSetState(rasterizer);
    
    //- rjf: do commands
    for(R_CmdNode *cmd_node = commands.first; cmd_node != 0; cmd_node = cmd_node->next)
    {
        R_Cmd *cmd = &cmd_node->cmd;
        R_D3D11_Buffer gpu_data_buffer = R_D3D11_BufferFromHandle(cmd->gpu_data);
        U64 gpu_buffer_size = gpu_data_buffer.size;
        
        switch(cmd->kind)
        {
            default:break;
            
            //- rjf: rects
            case R_CmdKind_Rects:
            {
                U64 floats_per_rect = R_Rect_FloatsPerInstance;
                U64 bytes_per_rect = floats_per_rect * sizeof(F32);
                
                ID3D11Buffer *constants__per_rect_buffer = os->constants__per_rect_buffer;
                ID3D11SamplerState *default_sampler = os->default_sampler;
                ID3D11VertexShader *vshad = os->rect_vshad;
                ID3D11PixelShader *pshad = os->rect_pshad;
                ID3D11InputLayout *ilay = os->rect_ilay;
                
                // rjf: upload vertex memory
                ID3D11Buffer *buffer = R_D3D11_ResolveCPUAndGPUDataToBuffer(os, cmd->cpu_data, cmd->gpu_data);
                U64 data_size = gpu_buffer_size + cmd->cpu_data.size;
                U64 data_num_rects = data_size / bytes_per_rect;
                
                // rjf: grab albedo texture
                R_Handle albedo_texture_handle = cmd->albedo_texture;
                if(R_HandleIsNull(albedo_texture_handle))
                {
                    albedo_texture_handle = os->backup_texture;
                }
                R_D3D11_Texture2D albedo_texture = R_D3D11_Texture2DFromHandle(albedo_texture_handle);
                
                // rjf: get albedo sample map matrix, based on format
                Matrix4x4F32 albedo_sample_channel_map = MakeMatrix4x4F32(1.f);
                switch(albedo_texture.format)
                {
                    default: break;
                    case R_Texture2DFormat_R8:
                    {
                        Matrix4x4F32 map =
                        {
                            {
                                { 1, 1, 1, 1 },
                                { 0, 0, 0, 0 },
                                { 0, 0, 0, 0 },
                                { 0, 0, 0, 0 },
                            },
                        };
                        albedo_sample_channel_map = map;
                    }break;
                }
                
                // rjf: upload per-rect constants
                R_D3D11_Globals_PerRect globals__per_rect = {0};
                {
                    globals__per_rect.viewport_size     = Vec2F32FromVec(wnd->last_resolution);
                    globals__per_rect.sdf_mask_boldness = 0.5f;
                    globals__per_rect.sdf_mask_softness = 0.2f;
                    globals__per_rect.albedo_sample_channel_map = albedo_sample_channel_map;
                    globals__per_rect.albedo_t2d_size = Vec2F32FromVec(albedo_texture.size);
                }
                R_D3D11_WriteGPUBuffer(os->device_ctx, os->constants__per_rect_buffer, 0, Str8Struct(&globals__per_rect));
                
                // rjf: grab sdf mask texture
                R_Handle sdf_mask_texture_handle = cmd->sdf_mask_texture;
                if(R_HandleIsNull(sdf_mask_texture_handle))
                {
                    sdf_mask_texture_handle = os->white_texture;
                }
                R_D3D11_Texture2D sdf_mask_texture = R_D3D11_Texture2DFromHandle(sdf_mask_texture_handle);
                
                // rjf: setup input assembly
                U32 stride = bytes_per_rect;
                U32 offset = 0;
                d_ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
                d_ctx->IASetInputLayout(ilay);
                d_ctx->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
                
                // rjf: setup shaders
                d_ctx->VSSetShader(vshad, 0, 0);
                d_ctx->VSSetConstantBuffers(0, 1, &constants__per_rect_buffer);
                d_ctx->PSSetShader(pshad, 0, 0);
                d_ctx->PSSetConstantBuffers(0, 1, &constants__per_rect_buffer);
                d_ctx->PSSetShaderResources(0, 1, &albedo_texture.view);
                d_ctx->PSSetShaderResources(1, 1, &sdf_mask_texture.view);
                d_ctx->PSSetSamplers(0, 1, &default_sampler);
                
                // rjf: setup output merger
                d_ctx->OMSetRenderTargets(1, &framebuffer_view, 0);
                d_ctx->OMSetBlendState(os->blend_state, 0, 0xffffffff);
                
                // rjf: draw
                d_ctx->DrawInstanced(4, data_num_rects, 0, 0);
            }break;
            
            //- rjf: clips
            case R_CmdKind_SetClip:
            {
                Rng2F32 clip_rect = *(Rng2F32 *)cmd->cpu_data.str;
                D3D11_RECT rect = {0};
                rect.left = clip_rect.x0;
                rect.right = clip_rect.x1;
                rect.top = clip_rect.y0;
                rect.bottom = clip_rect.y1;
                os->device_ctx->RSSetScissorRects(1, &rect);
            }break;
            
            //- randy: Ray Tracer experiment
            case R_CmdKind_RayTracer:
            {
                
            }break;
        }
    }
    
}

exported void
Finish(R_Handle os_eqp, R_Handle window_eqp)
{
    R_D3D11_OSEquip *os = R_D3D11_OSEquipFromHandle(os_eqp);
    R_D3D11_WindowEquip *wnd = R_D3D11_WindowEquipFromHandle(window_eqp);
    ID3D11DeviceContext1 *d_ctx = os->device_ctx;
    IDXGISwapChain1 *swapchain = wnd->swapchain;
    ID3D11Texture2D *framebuffer = wnd->framebuffer;
    ID3D11RenderTargetView *framebuffer_view = wnd->framebuffer_view;
    swapchain->Present(1, 0);
}
