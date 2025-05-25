
#pragma once

#include "pch.h"
#include "resource.h" // Ensure this exists for icons IDI_ICON1, IDI_ICON2
#include "InputManager.h"
#include "AudioManager.h"
#include "D2DRenderer.h"
#include "Camera.h"
#include "PhysicsManager.h"
#include "GameTimer.h"
#include "AssetTypes.h" // Include asset types

// Forward Declarations
class InputManager;
class AudioManager;
class D2DRenderer;
class Camera;
class PhysicsManager;
class GameTimer;
struct PlayerState;


// --- Global Application State (Minimize usage, encapsulate where possible) ---
HINSTANCE   g_hInstance;
HWND        g_hWnd;
bool        g_exitGame = false;
bool        g_isResizing = false; // Flag to indicate resize is in progress
bool        g_isMinimized = false; // Flag for minimized state

// --- Managers (Owned by the application or a Game class) ---
std::unique_ptr<InputManager>    g_inputManager;
std::unique_ptr<AudioManager>    g_audioManager;
std::unique_ptr<D2DRenderer>     g_d2dRenderer;
std::unique_ptr<PhysicsManager>  g_physicsManager;
std::unique_ptr<GameTimer>       g_gameTimer;
// std::unique_ptr<AssetManager> g_assetManager; // Add later
// std::unique_ptr<ColladaParser> g_colladaParser; // Add later if needed


// --- DirectX Core Objects ---
Microsoft::WRL::ComPtr<ID3D11Device5>           g_d3dDevice; // Renamed for clarity
Microsoft::WRL::ComPtr<ID3D11DeviceContext4>    g_d3dContext;
Microsoft::WRL::ComPtr<IDXGISwapChain4>         g_dxgiSwapChain;
Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  g_d3dRenderTargetView;
Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  g_d3dDepthStencilView;
Microsoft::WRL::ComPtr<ID3D11Texture2D>         g_d3dDepthStencilBuffer;
Microsoft::WRL::ComPtr<ID3D11Debug>             g_d3dDebug;
Microsoft::WRL::ComPtr<IDXGIFactory6>           g_dxgiFactory; // Renamed for clarity
Microsoft::WRL::ComPtr<IDXGIDevice1>            g_dxgiDevice; // For D2D interop

// --- D3D States (Could be managed by a Renderer class) ---
Microsoft::WRL::ComPtr<ID3D11DepthStencilState> g_d3dDepthStencilState; // Default state
Microsoft::WRL::ComPtr<ID3D11RasterizerState>   g_d3dRasterizerState;   // Default state (Cull Back)
Microsoft::WRL::ComPtr<ID3D11RasterizerState>   g_d3dRasterizerStateNoCull; // Example: No culling
Microsoft::WRL::ComPtr<ID3D11BlendState>        g_d3dBlendStateOpaque;    // Default opaque
Microsoft::WRL::ComPtr<ID3D11BlendState>        g_d3dBlendStateAlpha;     // Alpha blending

D3D_FEATURE_LEVEL                               g_featureLevel;
DXGI_FORMAT                                     g_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
DXGI_FORMAT                                     g_depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

// --- Window State ---
RECT        g_windowRect;
RECT        g_clientRect;
UINT        g_clientWidth = 1516;  // Initial desired size
UINT        g_clientHeight = 839;
int         g_windowPosX = 10;
int         g_windowPosY = 10;
bool        g_isFullScreen = false; // Current fullscreen state
const wchar_t g_windowTitle[] = L"Agrona";
const wchar_t g_windowClassName[] = L"AgronaMainWindowClass";

// --- Game Specific State ---
struct PlayerState {
    int playerId = 0;
    Camera camera;
    bool isActive = false;
    // Add position, health, score, current weapon etc.
    DirectX::XMFLOAT3 position = {0,0,0};
    int physicsObjectId = -1; // Link to physics object if controlled by physics
};

std::vector<PlayerState> g_players; // Support up to MAX_PLAYERS
int g_activePlayers = 1; // Start with one player

// Viewports (Recalculated on resize based on g_activePlayers)
std::vector<D3D11_VIEWPORT> g_viewports;

// --- Rendering Resources (Examples - Move to specific classes later) ---
Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> g_textBrush;    // Example D2D brush
Microsoft::WRL::ComPtr<IDWriteTextFormat>    g_textFont;     // Example DWrite format
Microsoft::WRL::ComPtr<IDWriteTextLayout>    g_debugTextLayout; // For dynamic text

float g_clearColor[4] = { 0.1f, 0.1f, 0.2f, 1.0f }; // Darker clear color

// --- Function Prototypes ---
// WinMain.cpp
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
bool InitializeMainWindow(HINSTANCE hInstance, int nCmdShow);
bool InitializeDirectX();
bool InitializeManagers();
void InitializeGame(); // Setup players, camera etc.
void ShutdownDirectX();
void ShutdownManagers();
void RunGameLoop();
void Update(float deltaTime);
void Render();
void Cleanup();
void HandleResize(UINT width, UINT height);
void CreateD3DResources();      // Create device-independent resources
void CreateWindowSizeDependentResources(); // Create RTV, DSV, Viewports, D2D Target
void ReleaseWindowSizeDependentResources();
void UpdateViewports(); // Calculate viewports based on active players

