
#include "pch.h"
#include "D2DRenderer.h"

D2DRenderer::D2DRenderer() {}

D2DRenderer::~D2DRenderer() {
    Shutdown();
}

bool D2DRenderer::Initialize(
    Microsoft::WRL::ComPtr<IDXGIDevice1> dxgiDevice,
    Microsoft::WRL::ComPtr<ID3D11DeviceContext4> d3dContext)
{
    if (!dxgiDevice || !d3dContext) return false;
    m_pD3DContext = d3dContext;

    HRESULT hr = S_OK;

    // Create D2D Factory
    D2D1_FACTORY_OPTIONS d2dFactoryOptions = {};
#ifdef _DEBUG
    d2dFactoryOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory3), &d2dFactoryOptions, &m_pD2DFactory);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Failed to create D2D Factory.", L"D2D Error", MB_OK);
        return false;
    }

    // Create DWrite Factory
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory3), &m_pDWriteFactory);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Failed to create DWrite Factory.", L"DWrite Error", MB_OK);
        return false;
    }

    // Create WIC Factory
    hr = CoCreateInstance(CLSID_WICImagingFactory2, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pWICFactory));
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Failed to create WIC Factory.", L"WIC Error", MB_OK);
        return false;
    }

    // Create D2D Device linked to the DXGI Device
    hr = m_pD2DFactory->CreateDevice(dxgiDevice.Get(), &m_pD2DDevice);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Failed to create D2D Device.", L"D2D Error", MB_OK);
        return false;
    }

    // Create D2D Device Context
    hr = m_pD2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_pD2DDeviceContext);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Failed to create D2D Device Context.", L"D2D Error", MB_OK);
        return false;
    }

    return true;
}

void D2DRenderer::Shutdown() {
    ReleaseDeviceDependentResources(); // Ensure target bitmap is released

    m_loadedImages.clear(); // ComPtrs will auto-release

    m_pD2DDeviceContext.Reset();
    m_pD2DDevice.Reset();
    m_pWICFactory.Reset();
    m_pDWriteFactory.Reset();
    m_pD2DFactory.Reset();
    m_pD3DContext.Reset(); // Release reference
}

bool D2DRenderer::CreateDeviceDependentResources(Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain) {
    if (!m_pD2DDeviceContext || !swapChain) return false;

    // Ensure previous resources are released
    ReleaseDeviceDependentResources();

    HRESULT hr = S_OK;

    // Get the DXGI Surface from the swap chain's back buffer
    Microsoft::WRL::ComPtr<IDXGISurface> dxgiBackBuffer;
    hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer));
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Failed to get DXGI back buffer for D2D.", L"D2D Error", MB_OK);
        return false;
    }

    // Get DPI (important for correct text/UI scaling)
    float dpiX, dpiY;
    m_pD2DFactory->GetDesktopDpi(&dpiX, &dpiY); // Or get from HWND if needed

    // Create a D2D bitmap wrapping the DXGI surface
    D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW, // Target for context, cannot be drawn directly
        D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), // Match swap chain format
        dpiX, dpiY
    );

    hr = m_pD2DDeviceContext->CreateBitmapFromDxgiSurface(
        dxgiBackBuffer.Get(),
        &bitmapProperties,
        &m_pD2DTargetBitmap
    );
    if (FAILED(hr)) {
        MessageBox(nullptr, L"Failed to create D2D bitmap from DXGI surface.", L"D2D Error", MB_OK);
        return false;
    }

    // Set the D2D context to render to this bitmap
    m_pD2DDeviceContext->SetTarget(m_pD2DTargetBitmap.Get());

    // Optional: Set DPI for context if different from factory default
    // m_pD2DDeviceContext->SetDpi(dpiX, dpiY);

    return true;
}

void D2DRenderer::ReleaseDeviceDependentResources() {
    m_pD2DDeviceContext->SetTarget(nullptr); // IMPORTANT: Release target before bitmap
    m_pD2DTargetBitmap.Reset();
    // Optional: Flush context if needed, though usually done after EndDraw
    // if (m_pD3DContext) m_pD3DContext->Flush();
}


void D2DRenderer::BeginDraw() {
    if (!m_pD2DDeviceContext) return;
    m_pD2DDeviceContext->BeginDraw();
    // Optional: Clear background or set transforms here if needed globally
     m_pD2DDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity()); // Reset transform
    // m_pD2DDeviceContext->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f)); // Example clear
}

void D2DRenderer::EndDraw() {
    if (!m_pD2DDeviceContext) return;
    HRESULT hr = m_pD2DDeviceContext->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        // This indicates the D3D device was lost or requires resource recreation.
        // Signal the main application loop to release and recreate D3D & D2D resources.
        OutputDebugString(L"D2DERR_RECREATE_TARGET occurred. Need to reset device.\n");
        // A robust application would trigger the full device reset sequence here.
        ReleaseDeviceDependentResources(); // Release D2D target at minimum
    } else if (FAILED(hr)) {
        OutputDebugString(L"D2D EndDraw failed.\n");
        // Handle other potential errors
    }

    // Optional: Flush the D3D context to ensure D2D commands are submitted
    // This might be needed depending on synchronization, especially if D3D rendering follows immediately
    // if (m_pD3DContext) m_pD3DContext->Flush();
}

// --- Drawing Methods ---

void D2DRenderer::DrawTextLayout(IDWriteTextLayout* layout, float x, float y, ID2D1Brush* brush) {
     if (!m_pD2DDeviceContext || !layout || !brush) return;
     m_pD2DDeviceContext->DrawTextLayout(D2D1::Point2F(x, y), layout, brush);
}

void D2DRenderer::DrawText(const std::wstring& text, IDWriteTextFormat* format, const D2D1_RECT_F& layoutRect, ID2D1Brush* brush) {
    if (!m_pD2DDeviceContext || !format || !brush) return;
    m_pD2DDeviceContext->DrawTextW(
        text.c_str(),
        static_cast<UINT32>(text.length()),
        format,
        layoutRect,
        brush
    );
}

void D2DRenderer::DrawImage(const std::string& imageName, const D2D1_RECT_F& destRect, float opacity, D2D1_INTERPOLATION_MODE interpolation) {
    if (!m_pD2DDeviceContext) return;
    auto it = m_loadedImages.find(imageName);
    if (it == m_loadedImages.end()) {
        OutputDebugStringA(("Image not found for drawing: " + imageName + "\n").c_str());
        return;
    }

    m_pD2DDeviceContext->DrawBitmap(
        it->second.Get(),
        destRect,
        opacity,
        interpolation
        // Optional source rectangle: D2D1::RectF(0.0f, 0.0f, width, height)
    );
}

void D2DRenderer::DrawRectangle(const D2D1_RECT_F& rect, ID2D1Brush* brush, float strokeWidth) {
     if (!m_pD2DDeviceContext || !brush) return;
     m_pD2DDeviceContext->DrawRectangle(rect, brush, strokeWidth);
}

void D2DRenderer::FillRectangle(const D2D1_RECT_F& rect, ID2D1Brush* brush) {
     if (!m_pD2DDeviceContext || !brush) return;
     m_pD2DDeviceContext->FillRectangle(rect, brush);
}


// --- Resource Loading/Creation ---

bool D2DRenderer::LoadImageFromFile(const std::wstring& filename, const std::string& imageName) {
    if (!m_pWICFactory || !m_pD2DDeviceContext) return false;
    if (m_loadedImages.count(imageName)) return true; // Already loaded

    HRESULT hr;
    Microsoft::WRL::ComPtr<IWICBitmapDecoder> pDecoder;
    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> pSourceFrame;
    Microsoft::WRL::ComPtr<IWICFormatConverter> pConverter;
    Microsoft::WRL::ComPtr<ID2D1Bitmap> pD2DBitmap;

    // Create decoder for the file
    hr = m_pWICFactory->CreateDecoderFromFilename(
        filename.c_str(),
        NULL, // No preferred vendor
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad, // Cache metadata
        &pDecoder
    );
    if (FAILED(hr)) {
        OutputDebugString((L"Failed to create WIC decoder for: " + filename + L"\n").c_str());
        return false;
    }

    // Get the first frame (WIC supports multi-frame images like GIF/TIFF)
    hr = pDecoder->GetFrame(0, &pSourceFrame);
    if (FAILED(hr)) {
        OutputDebugString((L"Failed to get WIC frame from: " + filename + L"\n").c_str());
        return false;
    }

    // Create a format converter to ensure the image is in a D2D-compatible format
    hr = m_pWICFactory->CreateFormatConverter(&pConverter);
    if (FAILED(hr)) {
        OutputDebugString(L"Failed to create WIC format converter.\n");
        return false;
    }

    // Initialize the converter
    hr = pConverter->Initialize(
        pSourceFrame.Get(),                 // Input bitmap to convert
        GUID_WICPixelFormat32bppPBGRA,      // Destination pixel format (premultiplied BGRA)
        WICBitmapDitherTypeNone,            // No dithering
        NULL,                               // No custom palette
        0.f,                                // Alpha threshold
        WICBitmapPaletteTypeMedianCut       // Palette translation type
    );
    if (FAILED(hr)) {
        OutputDebugString(L"Failed to initialize WIC format converter.\n");
        return false;
    }

    // Create a D2D bitmap from the WIC bitmap source
    hr = m_pD2DDeviceContext->CreateBitmapFromWicBitmap(
        pConverter.Get(),
        NULL, // Default bitmap properties
        &pD2DBitmap
    );
    if (FAILED(hr)) {
        OutputDebugString(L"Failed to create D2D bitmap from WIC bitmap.\n");
        return false;
    }

    // Store the loaded bitmap
    m_loadedImages[imageName] = pD2DBitmap;
    return true;
}

Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> D2DRenderer::CreateSolidColorBrush(const D2D1_COLOR_F& color) {
    if (!m_pD2DDeviceContext) return nullptr;

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    HRESULT hr = m_pD2DDeviceContext->CreateSolidColorBrush(color, &brush);
    if (FAILED(hr)) {
        OutputDebugString(L"Failed to create solid color brush.\n");
        return nullptr;
    }
    return brush;
}

Microsoft::WRL::ComPtr<IDWriteTextFormat> D2DRenderer::CreateTextFormat(
    const std::wstring& fontFamily, float fontSize,
    DWRITE_FONT_WEIGHT weight, DWRITE_FONT_STYLE style, DWRITE_FONT_STRETCH stretch)
{
    if (!m_pDWriteFactory) return nullptr;

    Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
    HRESULT hr = m_pDWriteFactory->CreateTextFormat(
        fontFamily.c_str(),
        NULL, // Font collection (nullptr for system default)
        weight,
        style,
        stretch,
        fontSize,
        L"en-us", // Locale
        &format
    );
    if (FAILED(hr)) {
        OutputDebugString(L"Failed to create text format.\n");
        return nullptr;
    }
    return format;
}

Microsoft::WRL::ComPtr<IDWriteTextLayout> D2DRenderer::CreateTextLayout(
    const std::wstring& text, IDWriteTextFormat* format, float maxWidth, float maxHeight)
{
     if (!m_pDWriteFactory || !format) return nullptr;

     Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
     HRESULT hr = m_pDWriteFactory->CreateTextLayout(
         text.c_str(),
         static_cast<UINT32>(text.length()),
         format,
         maxWidth,
         maxHeight,
         &layout
     );
     if (FAILED(hr)) {
         OutputDebugString(L"Failed to create text layout.\n");
         return nullptr;
     }
     return layout;
}
