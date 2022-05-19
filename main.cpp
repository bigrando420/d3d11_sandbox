//#include "minimal_d3d11.cpp"

#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>

#include <DirectXMath.h>

#define TITLE "Rando's Sandbox"

struct Vert
{
    float x, y, z;
    float r, g, b, a;
};

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        } break;
    }
    
    return DefWindowProc (hWnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    //~ WINDOW
    HWND window;
    WNDCLASSEX wc;
    
    ZeroMemory(&wc, sizeof(WNDCLASSEX));
    
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = "WindowClass";
    
    RegisterClassEx(&wc);
    
    RECT wr = {0, 0, 800, 600};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    
    window = CreateWindowEx(NULL,
                            "WindowClass",
                            TITLE,
                            WS_OVERLAPPEDWINDOW,
                            300,
                            300,
                            wr.right - wr.left,
                            wr.bottom - wr.top,
                            NULL,
                            NULL,
                            hInstance,
                            NULL);
    
    ShowWindow(window, nShowCmd);
    
    
    
    //~ D3D11 INIT
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    
    ID3D11Device* baseDevice;
    ID3D11DeviceContext* baseDeviceContext;
    
    D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &baseDevice, nullptr, &baseDeviceContext);
    
    ///////////////////////////////////////////////////////////////////////////////////////////////
    
    ID3D11Device1* device;
    
    baseDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&device));
    
    ID3D11DeviceContext1* deviceContext;
    
    baseDeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&deviceContext));
    
    ///////////////////////////////////////////////////////////////////////////////////////////////
    
    IDXGIDevice1* dxgiDevice;
    
    device->QueryInterface(__uuidof(IDXGIDevice1), reinterpret_cast<void**>(&dxgiDevice));
    
    IDXGIAdapter* dxgiAdapter;
    
    dxgiDevice->GetAdapter(&dxgiAdapter);
    IDXGIFactory2* dxgiFactory;
    
    
    dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory));
    
    ///////////////////////////////////////////////////////////////////////////////////////////////
    
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    swapChainDesc.Width              = 0; // use window width
    swapChainDesc.Height             = 0; // use window height
    swapChainDesc.Format             = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    swapChainDesc.Stereo             = FALSE;
    swapChainDesc.SampleDesc.Count   = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount        = 2;
    swapChainDesc.Scaling            = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect         = DXGI_SWAP_EFFECT_DISCARD;
    swapChainDesc.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags              = 0;
    
    IDXGISwapChain1* swapChain;
    
    dxgiFactory->CreateSwapChainForHwnd(device, window, &swapChainDesc, nullptr, nullptr, &swapChain);
    
    
    
    //~ SETUP FRAME BUFFER AND RENDER VIEW
    ID3D11Texture2D* frameBuffer;
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&frameBuffer));
    
    // TODO(randy): maybe I can fill out this texture myself?
    // that way I can render directly to the back buffer?
    // or maybe just use the pixel shader and sample from my texture?
    // TODO(randy): might have to look into constants
    
    ID3D11RenderTargetView* frameBufferView;
    device->CreateRenderTargetView(frameBuffer, nullptr, &frameBufferView);
    
    deviceContext->OMSetRenderTargets(1, &frameBufferView, nullptr);
    
    // Set the viewport
    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
    
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = 800;
    viewport.Height = 600;
    
    deviceContext->RSSetViewports(1, &viewport);
    
    //~ SHADERS
    ID3D10Blob *VS, *PS;
    D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "vs_main", "vs_5_0", 0, 0, &VS, nullptr);
    D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "ps_main", "ps_5_0", 0, 0, &PS, nullptr);
    
    ID3D11VertexShader *pVS;
    ID3D11PixelShader *pPS;
    device->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &pVS);
    device->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &pPS);
    
    deviceContext->VSSetShader(pVS, 0, 0);
    deviceContext->PSSetShader(pPS, 0, 0);
    
    
    //~ VERTEX BUFFER
    Vert our_verts[] =
    {
        {0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f},
        {0.45f, -0.5, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f},
        {-0.45f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f}
    };
    
    ID3D11Buffer *pVBuffer;
    
    D3D11_BUFFER_DESC bufferDesc;
    ZeroMemory(&bufferDesc, sizeof(bufferDesc));
    
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC; // write access access by CPU and GPU
    bufferDesc.ByteWidth = sizeof(our_verts); // TODO(randy): test
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER; // use as a vertex buffer
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // allow CPU to write in buffer
    
    device->CreateBuffer(&bufferDesc, NULL, &pVBuffer);
    
    // copy vets into buffer
    D3D11_MAPPED_SUBRESOURCE ms;
    deviceContext->Map(pVBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
    memcpy(ms.pData, our_verts, sizeof(our_verts));
    deviceContext->Unmap(pVBuffer, NULL);
    
    //~ VERTEX INPUT LAYOUT
    D3D11_INPUT_ELEMENT_DESC ied[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    
    ID3D11InputLayout *pLayout;
    device->CreateInputLayout(ied,
                              2,
                              VS->GetBufferPointer(), // shader entry point in the pipeline for this data
                              VS->GetBufferSize(),
                              &pLayout);
    deviceContext->IASetInputLayout(pLayout);
    
    
    
    //~ LOOP
    while (true)
    {
        MSG msg;
        
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_KEYDOWN || msg.message == WM_QUIT) return 0;
            DispatchMessageA(&msg);
        }
        
        //~ clear the buffer with blue
        float col[4] = {0.0f, 0.2f, 0.4f, 1.0f};
        deviceContext->ClearRenderTargetView(frameBufferView, col);
        
        
        
        
        //~ RENDER TIMEEE
        UINT stride = sizeof(Vert);
        UINT offset = 0;
        deviceContext->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);
        
        deviceContext->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        // deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
        // NOTE(randy): I could use the line strips for rendering debug lines yay
        
        deviceContext->Draw(3, 0); // draw the vertex buffer to the back buffer
        
        
        
        
        
        //~ Swap back and front buffers
        swapChain->Present(0, 0);
    }
}