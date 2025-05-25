
#pragma once

#include "pch.h"
#include <map>
#include <string>

class D2DRenderer {
public:
    D2DRenderer();
    ~D2DRenderer();

    bool Initialize(
        Microsoft::WRL::ComPtr<IDXGIDevice1> dxgiDevice, // Get from D3D Device
        Microsoft::WRL::ComPtr<ID3D11DeviceContext4> d3dContext // For flushing
    );

    // Call when swap chain is resized or created
    bool CreateDeviceDependentResources(Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain);
    // Call before swap chain is resized or destroyed
    void ReleaseDeviceDependentResources();

    void Shutdown();

    // Drawing operations (must be between BeginDraw/EndDraw)
    void DrawTextLayout(IDWriteTextLayout* layout, float x, float y, ID2D1Brush* brush);
    void DrawText(const std::wstring& text, IDWriteTextFormat* format, const D2D1_RECT_F& layoutRect, ID2D1Brush* brush);
    void DrawImage(const std::string& imageName, const D2D1_RECT_F& destRect, float opacity = 1.0f, D2D1_INTERPOLATION_MODE interpolation = D2D1_INTERPOLATION_MODE_LINEAR);
    void DrawRectangle(const D2D1_RECT_F& rect, ID2D1Brush* brush, float strokeWidth = 1.0f);
    void FillRectangle(const D2D1_RECT_F& rect, ID2D1Brush* brush);

    // Resource Loading/Creation
    bool LoadImageFromFile(const std::wstring& filename, const std::string& imageName);
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> CreateSolidColorBrush(const D2D1_COLOR_F& color);
    Microsoft::WRL::ComPtr<IDWriteTextFormat> CreateTextFormat(const std::wstring& fontFamily = L"Arial", float fontSize = 20.0f, DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL);
    Microsoft::WRL::ComPtr<IDWriteTextLayout> CreateTextLayout(const std::wstring& text, IDWriteTextFormat* format, float maxWidth, float maxHeight);

    // Begin/End Draw sequence for 2D rendering
    void BeginDraw();
    void EndDraw(); // Flushes D2D commands and handles errors

    // Accessors
    ID2D1DeviceContext* GetDeviceContext() { return m_pD2DDeviceContext.Get(); }
    IDWriteFactory* GetDWriteFactory() { return m_pDWriteFactory.Get(); }
    IWICImagingFactory* GetWICFactory() { return m_pWICFactory.Get(); }

private:
    Microsoft::WRL::ComPtr<ID3D11DeviceContext4> m_pD3DContext; // Needed for Flush
    Microsoft::WRL::ComPtr<ID2D1Factory3>        m_pD2DFactory;
    Microsoft::WRL::ComPtr<IDWriteFactory3>      m_pDWriteFactory;
    Microsoft::WRL::ComPtr<IWICImagingFactory2>  m_pWICFactory;
    Microsoft::WRL::ComPtr<ID2D1Device2>         m_pD2DDevice;
    Microsoft::WRL::ComPtr<ID2D1DeviceContext2>  m_pD2DDeviceContext; // Main render target

    // Device-dependent resources (tied to swap chain)
    Microsoft::WRL::ComPtr<ID2D1Bitmap1>         m_pD2DTargetBitmap;

    // Loaded Image Resources
    std::map<std::string, Microsoft::WRL::ComPtr<ID2D1Bitmap>> m_loadedImages;
};
