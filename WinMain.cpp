// Agrona
// Copyright (c) 2025 CGLJ08. All rights reserved.
// This project includes code derived from Microsoft's MSDN samples. See the LICENSE file for details.

#include "pch.h"
#include "WinMain.h"
#include "AssetTypes.h" // Make sure asset types are included if used directly here

// For ComPtr<> and other WRL utilities
using namespace Microsoft::WRL;
using namespace DirectX; // For math types if needed directly

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        MessageBox(nullptr, L"COM Initialization Failed!", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    g_hInstance = hInstance;

    // 1. Initialize Window
    if (!InitializeMainWindow(hInstance, nCmdShow)) {
        MessageBox(nullptr, L"Window Initialization Failed!", L"Error", MB_OK | MB_ICONERROR);
        CoUninitialize();
        return 2;
    }

    // 2. Initialize DirectX Core
    if (!InitializeDirectX()) {
        MessageBox(nullptr, L"DirectX Initialization Failed!", L"Error", MB_OK | MB_ICONERROR);
        Cleanup(); // Cleanup window and COM
        return 3;
    }

     // 3. Initialize Managers (Input, Audio, D2D, Physics, Timer)
    if (!InitializeManagers()) {
        MessageBox(nullptr, L"Manager Initialization Failed!", L"Error", MB_OK | MB_ICONERROR);
        Cleanup(); // Cleanup window, DX, COM
        return 4;
    }

     // 4. Initialize Game Specifics (Players, Camera, Load initial assets)
     InitializeGame();


    // 5. Run the main game loop
    RunGameLoop();

    // 6. Cleanup
    Cleanup();

    return 0;
}

// --- Initialization Functions ---

bool InitializeMainWindow(HINSTANCE hInstance, int nCmdShow) {
    // Register class
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS; // Removed CS_OWNDC if not strictly needed
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // Or NULL for no background drawing by OS
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = g_windowClassName;
    wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2));

    if (!RegisterClassExW(&wcex)) {
        return false;
    }

    // Create window
    // Calculate window size needed for desired client size
    RECT desiredClientRect = { 0, 0, static_cast<LONG>(g_clientWidth), static_cast<LONG>(g_clientHeight) };
    AdjustWindowRect(&desiredClientRect, WS_OVERLAPPEDWINDOW, FALSE);

    int windowWidth = desiredClientRect.right - desiredClientRect.left;
    int windowHeight = desiredClientRect.bottom - desiredClientRect.top;

    g_hWnd = CreateWindowW(g_windowClassName, g_windowTitle, WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, // Use default position initially
                           windowWidth, windowHeight,
                           nullptr, nullptr, hInstance, nullptr);

    if (!g_hWnd) {
        return false;
    }

    // Get initial actual client rect size
    GetClientRect(g_hWnd, &g_clientRect);
    g_clientWidth = g_clientRect.right - g_clientRect.left;
    g_clientHeight = g_clientRect.bottom - g_clientRect.top;
    GetWindowRect(g_hWnd, &g_windowRect);
    g_windowPosX = g_windowRect.left;
    g_windowPosY = g_windowRect.top;


    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    return true;
}

bool InitializeDirectX() {
    HRESULT hr = S_OK;
    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // Create DXGI Factory
    hr = CreateDXGIFactory2(createDeviceFlags, IID_PPV_ARGS(&g_dxgiFactory));
     if (FAILED(hr)) {
         MessageBox(g_hWnd, L"Failed to create DXGI Factory.", L"DirectX Error", MB_OK);
         return false;
     }

    // Select Adapter (High Performance Preference)
    ComPtr<IDXGIAdapter1> dxgiAdapter;
    hr = g_dxgiFactory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&dxgiAdapter));
    if (FAILED(hr)) {
        // Fallback to default adapter
        hr = g_dxgiFactory->EnumAdapters1(0, &dxgiAdapter);
         if (FAILED(hr)) {
            MessageBox(g_hWnd, L"No suitable DXGI Adapter found.", L"DirectX Error", MB_OK);
            return false;
         }
    }

     // Define Feature Levels
     D3D_FEATURE_LEVEL featureLevels[] = {
         D3D_FEATURE_LEVEL_11_1,
         D3D_FEATURE_LEVEL_11_0,
     };
     UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    // Create D3D Device and Context
    ComPtr<ID3D11Device> tempDevice;
    ComPtr<ID3D11DeviceContext> tempContext;
    hr = D3D11CreateDevice(
        dxgiAdapter.Get(),          // Specify adapter
        D3D_DRIVER_TYPE_UNKNOWN,    // Must be UNKNOWN when specifying adapter
        nullptr,                    // No software device
        createDeviceFlags,          // Debug flags
        featureLevels,              // Feature levels array
        numFeatureLevels,           // Number of feature levels
        D3D11_SDK_VERSION,          // SDK version
        &tempDevice,                // [out] D3D Device
        &g_featureLevel,            // [out] Actual feature level
        &tempContext                // [out] D3D Context
    );
    if (FAILED(hr)) {
        MessageBox(g_hWnd, L"Failed to create D3D11 Device.", L"DirectX Error", MB_OK);
        return false;
    }

    // Query for newer interfaces (ID3D11Device5, ID3D11DeviceContext4)
    hr = tempDevice.As(&g_d3dDevice);
    if (FAILED(hr)) { MessageBox(g_hWnd, L"Failed to query ID3D11Device5.", L"DirectX Error", MB_OK); return false;}
    hr = tempContext.As(&g_d3dContext);
     if (FAILED(hr)) { MessageBox(g_hWnd, L"Failed to query ID3D11DeviceContext4.", L"DirectX Error", MB_OK); return false;}

    // Query for DXGI Device (for D2D interop)
    hr = g_d3dDevice.As(&g_dxgiDevice);
    if (FAILED(hr)) { MessageBox(g_hWnd, L"Failed to query IDXGIDevice1.", L"DirectX Error", MB_OK); return false;}


#ifdef _DEBUG
    // Setup Debug Layer
    hr = g_d3dDevice.As(&g_d3dDebug);
    if (SUCCEEDED(hr)) {
        ComPtr<ID3D11InfoQueue> d3dInfoQueue;
        hr = g_d3dDebug.As(&d3dInfoQueue);
        if (SUCCEEDED(hr)) {
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
            // Optional: Break on warnings too
            // d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, true);

             // Suppress irrelevant messages (optional)
            D3D11_MESSAGE_ID hide[] = {
                D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
                // Add more IDs here as needed
            };
            D3D11_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = ARRAYSIZE(hide);
            filter.DenyList.pIDList = hide;
            d3dInfoQueue->AddStorageFilterEntries(&filter);
        }
    }
#endif

    // Create Swap Chain
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = g_clientWidth;   // Use client width
    swapChainDesc.Height = g_clientHeight; // Use client height
    swapChainDesc.Format = g_backBufferFormat;
    swapChainDesc.Stereo = false;
    swapChainDesc.SampleDesc.Count = 1; // No multisampling
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2; // Double buffered
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH; // Or NONE/ASPECT_RATIO_STRETCH
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // Recommended
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE; // Or PREMULTIPLIED if needed
    swapChainDesc.Flags = 0; // Add DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH for fullscreen transitions

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsDesc = {};
    fsDesc.Windowed = TRUE; // Start windowed

    ComPtr<IDXGISwapChain1> tempSwapChain;
    hr = g_dxgiFactory->CreateSwapChainForHwnd(
        g_d3dDevice.Get(),        // D3D Device
        g_hWnd,                   // Target window
        &swapChainDesc,           // Swap chain description
        &fsDesc,                  // Fullscreen description (can be nullptr)
        nullptr,                  // Restrict to output (can be nullptr)
        &tempSwapChain
    );
    if (FAILED(hr)) {
        MessageBox(g_hWnd, L"Failed to create Swap Chain.", L"DirectX Error", MB_OK);
        return false;
    }

    // Query for IDXGISwapChain4
    hr = tempSwapChain.As(&g_dxgiSwapChain);
     if (FAILED(hr)) { MessageBox(g_hWnd, L"Failed to query IDXGISwapChain4.", L"DirectX Error", MB_OK); return false;}

    // Prevent DXGI from monitoring Alt+Enter (we handle it manually in WndProc)
    g_dxgiFactory->MakeWindowAssociation(g_hWnd, DXGI_MWA_NO_ALT_ENTER);

    // Create device-independent D3D resources (states, etc.)
    CreateD3DResources();

    // Create initial window size dependent resources (RTV, DSV, Viewports)
    CreateWindowSizeDependentResources();

    return true;
}


bool InitializeManagers() {
    // Input Manager
    g_inputManager = std::make_unique<InputManager>();
    if (!g_inputManager || !g_inputManager->Initialize(g_hWnd)) {
        return false;
    }

    // Audio Manager
    g_audioManager = std::make_unique<AudioManager>();
    if (!g_audioManager || !g_audioManager->Initialize()) {
        return false;
    }
    // Example: Load sounds
    // g_audioManager->LoadWaveFile(L"Assets/Sounds/shoot.wav", "shoot");
    // g_audioManager->LoadWaveFile(L"Assets/Sounds/music.wav", "bg_music");


    // D2D Renderer
    g_d2dRenderer = std::make_unique<D2DRenderer>();
    if (!g_d2dRenderer || !g_d2dRenderer->Initialize(g_dxgiDevice, g_d3dContext)) {
        return false;
    }
    // Create D2D resources dependent on swap chain (must happen AFTER swap chain creation)
    if (!g_d2dRenderer->CreateDeviceDependentResources(g_dxgiSwapChain)) {
         return false;
    }
     // Example: Load images / create brushes/formats
     // g_d2dRenderer->LoadImageFromFile(L"Assets/Images/crosshair.png", "crosshair");
     g_textBrush = g_d2dRenderer->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White));
     g_textFont = g_d2dRenderer->CreateTextFormat(L"Consolas", 14.0f);
     if (!g_textBrush || !g_textFont) return false; // Check creation


    // Physics Manager
    g_physicsManager = std::make_unique<PhysicsManager>();
    if (!g_physicsManager) return false;
    g_physicsManager->Initialize(); // Use default gravity


     // Game Timer
    g_gameTimer = std::make_unique<GameTimer>();
    if (!g_gameTimer) return false;
    g_gameTimer->Reset();


    // Asset Manager / Collada Parser (Initialize later when needed)
    // g_colladaParser = std::make_unique<ColladaParser>();
    // g_assetManager = std::make_unique<AssetManager>();
    // Model testModel;
    // g_colladaParser->ParseFile(L"Assets/Models/character.dae", testModel); // Placeholder!

    return true;
}

void InitializeGame() {
     g_activePlayers = 1; // Start with 1 player
     g_players.resize(g_activePlayers);

     for (int i = 0; i < g_activePlayers; ++i) {
         g_players[i].playerId = i;
         g_players[i].isActive = true;
         g_players[i].camera.SetPosition(0.0f, 1.0f, -5.0f); // Initial position
         g_players[i].camera.UpdateProjectionMatrix(XM_PIDIV4, // 45 degrees FOV
             static_cast<float>(g_clientWidth) / static_cast<float>(g_clientHeight), // Aspect Ratio
             0.1f, 1000.0f); // Near/Far Z
        g_players[i].camera.SetMode(CameraMode::FPS);

        // Optional: Create a physics object for the player
        PhysicsObject playerPhys;
        playerPhys.Position = g_players[i].camera.GetPosition();
        playerPhys.BoundingBox = { {-0.5f, -1.0f, -0.5f}, {0.5f, 1.0f, 0.5f} }; // Example size
        playerPhys.Mass = 80.0f;
        playerPhys.HasGravity = true;
        playerPhys.IsStatic = false;
        g_players[i].physicsObjectId = g_physicsManager->AddObject(playerPhys);

     }

     UpdateViewports(); // Setup initial viewports

     // Start background music?
     // g_audioManager->PlayMusic("bg_music");
}


// --- Game Loop ---

void RunGameLoop() {
    MSG msg = {};
    g_gameTimer->Reset();

    while (!g_exitGame) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) {
                g_exitGame = true;
                break;
            }
        }

        if (g_exitGame) break;

        // If window is minimized, don't update/render (or throttle significantly)
        if (g_isMinimized) {
            // Optional: Sleep briefly to avoid spinning CPU
            Sleep(100); // Sleep for 100ms
            // Ensure timer is stopped if we pause on minimize
             if (g_gameTimer->DeltaTime() >= 0) { // Check if timer was running
                 g_gameTimer->Stop();
             }
            continue; // Skip update/render loop
        }

         // Resume timer if we were paused
         if (g_gameTimer->DeltaTime() < 0) { // Check if timer was stopped
             g_gameTimer->Start();
         }

        g_gameTimer->Tick(); // Calculate delta time for this frame
        float deltaTime = g_gameTimer->DeltaTime();

        // Update game logic, input, physics, audio, etc.
        Update(deltaTime);

        // Render the scene
        Render();
    }
}


// --- Update Function ---
void Update(float deltaTime) {
     // 1. Update Input Manager (reads current state)
     g_inputManager->Update();

     // 2. Update Audio System (e.g., check for finished sounds, stream data)
     // g_audioManager->Update(); // Add if needed

     // 3. Update Physics
     g_physicsManager->Update(deltaTime);

     // 4. Update Game Logic (Players, AI, Objects)
     for (int i = 0; i < g_activePlayers; ++i) {
         if (!g_players[i].isActive) continue;

         // Update player camera based on input
         g_players[i].camera.Update(deltaTime, *g_inputManager);

         // Update player physics based on input (if applicable)
         PhysicsObject* pObj = g_physicsManager->GetObject(g_players[i].physicsObjectId);
         if (pObj) {
             XMFLOAT3 inputForce = {0,0,0};
             float moveSpeed = 500.0f; // Force units
             if (g_inputManager->IsKeyDown('W')) inputForce.z += moveSpeed;
             if (g_inputManager->IsKeyDown('S')) inputForce.z -= moveSpeed;
             if (g_inputManager->IsKeyDown('A')) inputForce.x -= moveSpeed;
             if (g_inputManager->IsKeyDown('D')) inputForce.x += moveSpeed;
             // TODO: Apply force relative to camera direction if using physics for movement
             g_physicsManager->ApplyForce(pObj->ObjectId, inputForce);

             // Sync camera position to physics object position (after physics update)
             // Be careful about feedback loops if camera directly controls physics force
             // It's often better to have input apply forces, then camera follows physics object
              g_players[i].camera.SetPosition(pObj->Position.x, pObj->Position.y + 0.8f, pObj->Position.z); // Camera slightly above physics center
         }


         // Handle player actions (shooting, interaction, etc.)
         if (g_inputManager->IsMouseButtonJustPressed(0)) { // Left Mouse Button
              // Play shoot sound
              g_audioManager->PlaySoundEffect("shoot", 1.0f); // Assuming "shoot" is loaded

              // Create a projectile
               Ray projectileRay;
               projectileRay.Origin = g_players[i].camera.GetPosition();
               projectileRay.Direction = g_players[i].camera.GetLookDirection();

               Projectile proj;
               proj.Position = projectileRay.Origin;
               XMVECTOR vel = XMLoadFloat3(&projectileRay.Direction);
               vel = XMVectorScale(vel, 50.0f); // Projectile speed
               XMStoreFloat3(&proj.Velocity, vel);
               proj.BoundingBox = {{-0.1f, -0.1f, -0.1f}, {0.1f, 0.1f, 0.1f}};
               proj.Mass = 0.1f;
               proj.HasGravity = true; // Or false for lasers
               proj.Lifetime = 3.0f;
               g_physicsManager->AddProjectile(proj);
         }
     }

     // 5. Update UI Text (example)
     static float fps = 0.0f;
     static int frameCount = 0;
     static float timeElapsed = 0.0f;
     frameCount++;
     timeElapsed += deltaTime;
     if (timeElapsed >= 1.0f) {
         fps = static_cast<float>(frameCount) / timeElapsed;
         frameCount = 0;
         timeElapsed = 0.0f;
     }

     std::wstringstream debugTextStream;
     debugTextStream.precision(2);
     debugTextStream << std::fixed;
     debugTextStream << L"FPS: " << fps << std::endl;
     debugTextStream << L"Players: " << g_activePlayers << std::endl;
     debugTextStream << L"P0 Pos: (" << g_players[0].camera.GetPosition().x << L", "
                     << g_players[0].camera.GetPosition().y << L", "
                     << g_players[0].camera.GetPosition().z << L")" << std::endl;
      PhysicsObject* pObj = g_physicsManager->GetObject(g_players[0].physicsObjectId);
      if(pObj) {
           debugTextStream << L"P0 Phys: (" << pObj->Position.x << L", " << pObj->Position.y << L", " << pObj->Position.z << L")" << std::endl;
      }

     // Recreate text layout for dynamic text
      g_debugTextLayout = g_d2dRenderer->CreateTextLayout(
         debugTextStream.str(),
         g_textFont.Get(),
         static_cast<float>(g_clientWidth), // Max width
         static_cast<float>(g_clientHeight) // Max height
     );

     // --- Handle Global Input ---
     if (g_inputManager->IsKeyJustPressed(VK_ESCAPE)) {
          // Ask to quit or open menu
          if (MessageBox(g_hWnd, L"Exit Agrona?", L"Confirm Exit", MB_YESNO | MB_ICONQUESTION) == IDYES) {
              g_exitGame = true;
          }
     }

     // Toggle Fullscreen (Manual Alt+Enter) handled in WndProc WM_SYSKEYDOWN

      // Toggle Mouse Capture (Example: F1 key)
      if (g_inputManager->IsKeyJustPressed(VK_F1)) {
          bool currentCapture = g_inputManager->IsMouseButtonDown(-1); // Use dummy button to check capture state if no direct getter
          // Need a getter in InputManager: g_inputManager->IsMouseCaptured()
          // For now, let's assume we toggle:
          static bool captureToggle = false;
          captureToggle = !captureToggle;
          g_inputManager->SetCaptureMouse(captureToggle);
      }

      // Example: Add/Remove player for split screen testing (F2/F3)
      if (g_inputManager->IsKeyJustPressed(VK_F2)) {
          if (g_activePlayers < MAX_PLAYERS) {
              g_activePlayers++;
              g_players.resize(g_activePlayers);
              // Initialize new player state (similar to InitializeGame)
              int i = g_activePlayers - 1;
               g_players[i].playerId = i;
               g_players[i].isActive = true;
               g_players[i].camera.SetPosition( (i%2==0? -2.0f : 2.0f) , 1.0f, -5.0f); // Offset new players
               g_players[i].camera.UpdateProjectionMatrix(XM_PIDIV4, 1.0f, 0.1f, 1000.0f); // Placeholder aspect
               g_players[i].camera.SetMode(CameraMode::FPS);
                PhysicsObject playerPhys; // Create physics obj
                playerPhys.Position = g_players[i].camera.GetPosition();
                g_players[i].physicsObjectId = g_physicsManager->AddObject(playerPhys);

              UpdateViewports(); // Recalculate viewports
              HandleResize(g_clientWidth, g_clientHeight); // Update projection matrices
          }
      }
       if (g_inputManager->IsKeyJustPressed(VK_F3)) {
           if (g_activePlayers > 1) {
                // Remove physics object associated with last player
                g_physicsManager->RemoveObject(g_players.back().physicsObjectId);
                g_activePlayers--;
                g_players.resize(g_activePlayers);
                UpdateViewports();
                HandleResize(g_clientWidth, g_clientHeight);
           }
       }

}


// --- Rendering ---

void Render() {
    if (g_isMinimized || !g_d3dContext || !g_dxgiSwapChain || !g_d3dRenderTargetView || !g_d3dDepthStencilView) {
         // Don't render if minimized or resources are invalid
         return;
    }

     // Clear the main Render Target View and Depth Stencil View
     g_d3dContext->ClearRenderTargetView(g_d3dRenderTargetView.Get(), g_clearColor);
     g_d3dContext->ClearDepthStencilView(g_d3dDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

     // Set common render states
     g_d3dContext->OMSetDepthStencilState(g_d3dDepthStencilState.Get(), 1);
     g_d3dContext->RSSetState(g_d3dRasterizerState.Get());
     float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
     g_d3dContext->OMSetBlendState(g_d3dBlendStateOpaque.Get(), blendFactor, 0xffffffff); // Opaque initially

     // Set the Render Target
     g_d3dContext->OMSetRenderTargets(1, g_d3dRenderTargetView.GetAddressOf(), g_d3dDepthStencilView.Get());


     // --- 3D Rendering Pass (Iterate through players/viewports) ---
     for (int i = 0; i < g_activePlayers; ++i) {
         if (!g_players[i].isActive) continue;

         // Set the viewport for the current player
         if (i < g_viewports.size()) {
              g_d3dContext->RSSetViewports(1, &g_viewports[i]);
         } else {
              // Fallback or error if viewport doesn't exist
              g_d3dContext->RSSetViewports(1, &g_viewports[0]); // Use first viewport
         }


         // Get Camera Matrices for this player
         XMMATRIX view = g_players[i].camera.GetViewMatrix();
         XMMATRIX proj = g_players[i].camera.GetProjectionMatrix();
         // XMMATRIX world = XMMatrixIdentity(); // For static world geometry

         // TODO: Set up vertex/index buffers for models
         // UINT stride = sizeof(Vertex);
         // UINT offset = 0;
         // g_d3dContext->IASetVertexBuffers(0, 1, modelMesh.pVertexBuffer.GetAddressOf(), &stride, &offset);
         // g_d3dContext->IASetIndexBuffer(modelMesh.pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
         // g_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

         // TODO: Set up shaders (Vertex, Pixel)
         // g_d3dContext->VSSetShader(g_vertexShader.Get(), nullptr, 0);
         // g_d3dContext->PSSetShader(g_pixelShader.Get(), nullptr, 0);

         // TODO: Update Constant Buffers with World*View*Projection matrix, lighting info, etc.
         // ConstantBuffer cbData;
         // cbData.WorldViewProj = XMMatrixTranspose(world * view * proj);
         // g_d3dContext->UpdateSubresource(g_constantBuffer.Get(), 0, nullptr, &cbData, 0, 0);
         // g_d3dContext->VSSetConstantBuffers(0, 1, g_constantBuffer.GetAddressOf());

         // TODO: Set Textures
         // g_d3dContext->PSSetShaderResources(0, 1, material.pDiffuseTextureView.GetAddressOf());
         // g_d3dContext->PSSetSamplers(0, 1, g_samplerState.GetAddressOf());

          // TODO: Draw the models/scene geometry
          // g_d3dContext->DrawIndexed(modelMesh.IndexCount, 0, 0);

          // --- Example: Draw Physics Object Bounding Boxes (Debug) ---
          // Need a simple cube mesh and appropriate shaders/state
          // For each physics object:
          //   Get AABB, calculate World matrix for the cube based on AABB center/size
          //   Update constant buffer
          //   Draw cube (wireframe is useful: use a wireframe rasterizer state)
     }


     // --- 2D Rendering Pass (UI / HUD) ---
     g_d2dRenderer->BeginDraw();

     // Draw common UI elements (e.g., central messages) - Rendered across whole screen
     // g_d2dRenderer->DrawText(...)

     // Draw player-specific HUD elements (health, ammo, crosshair)
     for (int i = 0; i < g_activePlayers; ++i) {
         if (!g_players[i].isActive || i >= g_viewports.size()) continue;

         // Calculate HUD position relative to the player's viewport
         const D3D11_VIEWPORT& vp = g_viewports[i];
         float hudX = vp.TopLeftX + 10.0f; // 10 pixels from left of viewport
         float hudY = vp.TopLeftY + 10.0f; // 10 pixels from top of viewport
         D2D1_RECT_F textRect = D2D1::RectF(hudX, hudY, vp.TopLeftX + vp.Width - 10.0f, vp.TopLeftY + vp.Height - 10.0f);

         // Draw debug text layout within the viewport bounds
         if (i == 0 && g_debugTextLayout) { // Only draw full debug for player 0 for now
              g_d2dRenderer->DrawTextLayout(g_debugTextLayout.Get(), hudX, hudY, g_textBrush.Get());
         } else {
             // Draw minimal HUD for other players (e.g., just player number)
             std::wstring simpleHud = L"Player " + std::to_wstring(i);
             g_d2dRenderer->DrawText(simpleHud, g_textFont.Get(), textRect, g_textBrush.Get());
         }


         // Example: Draw Crosshair in center of viewport
         // float centerX = vp.TopLeftX + vp.Width / 2.0f;
         // float centerY = vp.TopLeftY + vp.Height / 2.0f;
         // float crosshairSize = 32.0f;
         // D2D1_RECT_F crosshairRect = D2D1::RectF(centerX - crosshairSize / 2.0f, centerY - crosshairSize / 2.0f,
         //                                      centerX + crosshairSize / 2.0f, centerY + crosshairSize / 2.0f);
         // g_d2dRenderer->DrawImage("crosshair", crosshairRect); // Assuming "crosshair" image loaded

          // Can use SetTransform and PushAxisAlignedClip to restrict drawing strictly to viewport if needed
     }


     g_d2dRenderer->EndDraw(); // Submits D2D commands


    // --- Present the frame ---
    HRESULT hr = g_dxgiSwapChain->Present(1, 0); // Present with vsync (1)

    // Handle device lost/removed scenarios
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
        OutputDebugString(L"Graphics device removed or reset. Attempting recovery...\n");
        // Need to release all resources and re-initialize DirectX fully
         ShutdownDirectX(); // Release existing stuff
         if (!InitializeDirectX()) { // Attempt reinitialization
             MessageBox(g_hWnd, L"Failed to recover graphics device!", L"Fatal Error", MB_OK | MB_ICONERROR);
             g_exitGame = true; // Exit if recovery fails
         } else {
              // Recreate D2D resources as well
              g_d2dRenderer->Shutdown(); // Full D2D shutdown
               if (!g_d2dRenderer->Initialize(g_dxgiDevice, g_d3dContext) ||
                   !g_d2dRenderer->CreateDeviceDependentResources(g_dxgiSwapChain))
               {
                    MessageBox(g_hWnd, L"Failed to recover D2D resources!", L"Fatal Error", MB_OK | MB_ICONERROR);
                    g_exitGame = true;
               } else {
                    // Recreate application specific D2D brushes/formats if needed
                     g_textBrush = g_d2dRenderer->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White));
                     g_textFont = g_d2dRenderer->CreateTextFormat(L"Consolas", 14.0f);
               }
         }
    } else if (FAILED(hr)) {
        // Handle other present errors
        OutputDebugString(L"Present failed with other error.\n");
    }

    // Ensure render target is not bound after Present when using flip models
    g_d3dContext->OMSetRenderTargets(0, nullptr, nullptr);
}

// --- Resource Creation / Destruction ---

void CreateD3DResources() {
    HRESULT hr;

    // --- Depth Stencil State ---
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = true;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    dsDesc.StencilEnable = false; // Stencil disabled by default
    // Setup stencil operations if needed...
    hr = g_d3dDevice->CreateDepthStencilState(&dsDesc, &g_d3dDepthStencilState);
    if (FAILED(hr)) { MessageBox(g_hWnd, L"Failed to create Depth Stencil State.", L"DirectX Error", MB_OK); /* Handle Error */ }

    // --- Rasterizer States ---
    // Default (Solid, Cull Back)
    D3D11_RASTERIZER_DESC rastDesc = {};
    rastDesc.FillMode = D3D11_FILL_SOLID;
    rastDesc.CullMode = D3D11_CULL_BACK;
    rastDesc.FrontCounterClockwise = false; // Usually false for LH coordinates or exported models
    rastDesc.DepthClipEnable = true;
    rastDesc.MultisampleEnable = false; // No MSAA for swap chain buffer
    hr = g_d3dDevice->CreateRasterizerState(&rastDesc, &g_d3dRasterizerState);
    if (FAILED(hr)) { MessageBox(g_hWnd, L"Failed to create Rasterizer State.", L"DirectX Error", MB_OK); /* Handle Error */ }

    // No Culling Example
    rastDesc.CullMode = D3D11_CULL_NONE;
    hr = g_d3dDevice->CreateRasterizerState(&rastDesc, &g_d3dRasterizerStateNoCull);
    if (FAILED(hr)) { MessageBox(g_hWnd, L"Failed to create No Cull Rasterizer State.", L"DirectX Error", MB_OK); /* Handle Error */ }

    // Wireframe Example (Add later if needed)
    // rastDesc.FillMode = D3D11_FILL_WIREFRAME;
    // rastDesc.CullMode = D3D11_CULL_NONE; // Often disable culling for wireframe
    // hr = g_d3dDevice->CreateRasterizerState(&rastDesc, &g_d3dRasterizerStateWireframe);

    // --- Blend States ---
    // Opaque (Default)
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = FALSE; // Blending disabled
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    // Set IndependentBlendEnable = FALSE if all RTVs use same blend state
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.AlphaToCoverageEnable = FALSE; // For MSAA alpha effects
    hr = g_d3dDevice->CreateBlendState(&blendDesc, &g_d3dBlendStateOpaque);
    if (FAILED(hr)) { MessageBox(g_hWnd, L"Failed to create Opaque Blend State.", L"DirectX Error", MB_OK); /* Handle Error */ }

    // Alpha Blending (Typical Transparency)
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE; // Often use One/Zero for alpha channel blend
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    hr = g_d3dDevice->CreateBlendState(&blendDesc, &g_d3dBlendStateAlpha);
     if (FAILED(hr)) { MessageBox(g_hWnd, L"Failed to create Alpha Blend State.", L"DirectX Error", MB_OK); /* Handle Error */ }


    // TODO: Create Sampler States, Constant Buffers, Shaders, Input Layouts
}

void ReleaseWindowSizeDependentResources() {
     // Important: Unbind render targets before releasing them!
     g_d3dContext->OMSetRenderTargets(0, nullptr, nullptr);

     g_d3dRenderTargetView.Reset();
     g_d3dDepthStencilView.Reset();
     g_d3dDepthStencilBuffer.Reset();
     // Reset back buffer reference obtained via GetBuffer (if holding one)

     // Release D2D target bitmap
     if (g_d2dRenderer) {
          g_d2dRenderer->ReleaseDeviceDependentResources();
     }

     // Flush context to ensure resources are truly released (optional but sometimes helps)
     if (g_d3dContext) {
          g_d3dContext->Flush();
     }
}

void CreateWindowSizeDependentResources() {
     if (!g_dxgiSwapChain || !g_d3dDevice || !g_d3dContext) return;

     HRESULT hr;

     // --- Get Back Buffer and Create Render Target View (RTV) ---
     ComPtr<ID3D11Texture2D> backBuffer;
     hr = g_dxgiSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
     if (FAILED(hr)) { MessageBox(g_hWnd, L"Failed to get Swap Chain Buffer.", L"DirectX Error", MB_OK); /* Handle Error */ return; }

     D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
     rtvDesc.Format = g_backBufferFormat;
     rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
     rtvDesc.Texture2D.MipSlice = 0;

     hr = g_d3dDevice->CreateRenderTargetView(backBuffer.Get(), &rtvDesc, &g_d3dRenderTargetView);
      if (FAILED(hr)) { MessageBox(g_hWnd, L"Failed to create Render Target View.", L"DirectX Error", MB_OK); /* Handle Error */ return; }

     // --- Create Depth Stencil Buffer and View (DSV) ---
     D3D11_TEXTURE2D_DESC depthStencilDesc = {};
     depthStencilDesc.Width = g_clientWidth;
     depthStencilDesc.Height = g_clientHeight;
     depthStencilDesc.MipLevels = 1;
     depthStencilDesc.ArraySize = 1;
     depthStencilDesc.Format = g_depthBufferFormat;
     depthStencilDesc.SampleDesc.Count = 1; // Match swap chain MSAA
     depthStencilDesc.SampleDesc.Quality = 0;
     depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
     depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
     depthStencilDesc.CPUAccessFlags = 0;
     depthStencilDesc.MiscFlags = 0;

     hr = g_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &g_d3dDepthStencilBuffer);
      if (FAILED(hr)) { MessageBox(g_hWnd, L"Failed to create Depth Stencil Buffer.", L"DirectX Error", MB_OK); /* Handle Error */ return; }

     D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
     dsvDesc.Format = depthStencilDesc.Format;
     dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
     dsvDesc.Texture2D.MipSlice = 0;
     dsvDesc.Flags = 0;

     hr = g_d3dDevice->CreateDepthStencilView(g_d3dDepthStencilBuffer.Get(), &dsvDesc, &g_d3dDepthStencilView);
      if (FAILED(hr)) { MessageBox(g_hWnd, L"Failed to create Depth Stencil View.", L"DirectX Error", MB_OK); /* Handle Error */ return; }


     // --- Update Viewports ---
     UpdateViewports();


      // --- Update Projection Matrices for all active cameras ---
      float aspectRatio = static_cast<float>(g_clientWidth) / static_cast<float>(g_clientHeight);
      for(int i=0; i < g_activePlayers; ++i) {
          // Need to adjust aspect ratio based on individual viewport dimensions for split screen
           float vpAspect = 1.0f; // Default
           if (i < g_viewports.size() && g_viewports[i].Height > 0) {
                vpAspect = g_viewports[i].Width / g_viewports[i].Height;
           } else if (g_clientHeight > 0) {
               vpAspect = aspectRatio; // Use full aspect if viewport invalid
           }

           g_players[i].camera.UpdateProjectionMatrix(XM_PIDIV4, vpAspect, 0.1f, 1000.0f);
      }


     // --- Recreate D2D Resources ---
     if (g_d2dRenderer) {
         if (!g_d2dRenderer->CreateDeviceDependentResources(g_dxgiSwapChain)) {
              MessageBox(g_hWnd, L"Failed to recreate D2D Resources.", L"D2D Error", MB_OK);
              // Handle error - potentially try again or exit
         }
     }
}


// --- Utility and Cleanup ---

void UpdateViewports() {
    g_viewports.resize(g_activePlayers);
    float totalWidth = static_cast<float>(g_clientWidth);
    float totalHeight = static_cast<float>(g_clientHeight);

    switch (g_activePlayers) {
        case 1:
            g_viewports[0] = { 0.0f, 0.0f, totalWidth, totalHeight, 0.0f, 1.0f };
            break;
        case 2: // Side-by-side split
            g_viewports[0] = { 0.0f, 0.0f, totalWidth / 2.0f, totalHeight, 0.0f, 1.0f };
            g_viewports[1] = { totalWidth / 2.0f, 0.0f, totalWidth / 2.0f, totalHeight, 0.0f, 1.0f };
            break;
        case 3: // Player 1 top half, P2/P3 bottom corners
            g_viewports[0] = { 0.0f, 0.0f, totalWidth, totalHeight / 2.0f, 0.0f, 1.0f };
            g_viewports[1] = { 0.0f, totalHeight / 2.0f, totalWidth / 2.0f, totalHeight / 2.0f, 0.0f, 1.0f };
            g_viewports[2] = { totalWidth / 2.0f, totalHeight / 2.0f, totalWidth / 2.0f, totalHeight / 2.0f, 0.0f, 1.0f };
            break;
        case 4: // Four corners
            g_viewports[0] = { 0.0f, 0.0f, totalWidth / 2.0f, totalHeight / 2.0f, 0.0f, 1.0f };
            g_viewports[1] = { totalWidth / 2.0f, 0.0f, totalWidth / 2.0f, totalHeight / 2.0f, 0.0f, 1.0f };
            g_viewports[2] = { 0.0f, totalHeight / 2.0f, totalWidth / 2.0f, totalHeight / 2.0f, 0.0f, 1.0f };
            g_viewports[3] = { totalWidth / 2.0f, totalHeight / 2.0f, totalWidth / 2.0f, totalHeight / 2.0f, 0.0f, 1.0f };
            break;
        default: // Fallback to single player
             g_viewports.resize(1);
             g_viewports[0] = { 0.0f, 0.0f, totalWidth, totalHeight, 0.0f, 1.0f };
             break;
    }
}

void ShutdownManagers() {
     if(g_inputManager) g_inputManager->Shutdown();
     if(g_audioManager) g_audioManager->Shutdown();
     if(g_d2dRenderer) g_d2dRenderer->Shutdown();
     if(g_physicsManager) g_physicsManager->Shutdown();

     g_inputManager.reset();
     g_audioManager.reset();
     g_d2dRenderer.reset();
     g_physicsManager.reset();
     g_gameTimer.reset();
     // Reset other managers
}

void ShutdownDirectX() {
    // Ensure leaving fullscreen before releasing swap chain
     if (g_dxgiSwapChain) {
         g_dxgiSwapChain->SetFullscreenState(FALSE, nullptr);
     }

    ReleaseWindowSizeDependentResources(); // Release RTV, DSV etc.

    // Release device-independent states etc.
    g_d3dDepthStencilState.Reset();
    g_d3dRasterizerState.Reset();
    g_d3dRasterizerStateNoCull.Reset();
    g_d3dBlendStateOpaque.Reset();
    g_d3dBlendStateAlpha.Reset();
    // Reset other states, shaders, buffers...

    g_dxgiSwapChain.Reset();
    g_dxgiFactory.Reset(); // Release factory after swap chain
    g_d3dContext.Reset();
    g_dxgiDevice.Reset(); // Release DXGI device

    // Report Live Objects before releasing the device (requires debug layer)
    if (g_d3dDebug) {
         OutputDebugString(L"Reporting live Direct3D objects...\n");
         g_d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
         g_d3dDebug.Reset();
    }

    g_d3dDevice.Reset();
}

void Cleanup() {
    OutputDebugString(L"Starting Cleanup...\n");
    ShutdownManagers();
    ShutdownDirectX();

    if (g_hWnd) {
        DestroyWindow(g_hWnd);
        g_hWnd = nullptr;
    }
    UnregisterClassW(g_windowClassName, g_hInstance);

    CoUninitialize();
    OutputDebugString(L"Cleanup Finished.\n");
}

void HandleResize(UINT width, UINT height) {
    if (g_clientWidth == width && g_clientHeight == height && !g_isMinimized) {
        // No actual size change, possibly just moved (or first WM_SIZE on creation)
        return;
    }

     if (!g_dxgiSwapChain || !g_d3dContext || !g_d3dDevice) {
         // Cannot resize if essential objects don't exist
         return;
     }

     g_clientWidth = width;
     g_clientHeight = height;

     // If minimized, width/height might be 0. Don't resize buffers.
     if (g_clientWidth == 0 || g_clientHeight == 0) {
         g_isMinimized = true;
         if (g_gameTimer && g_gameTimer->DeltaTime() >= 0) g_gameTimer->Stop(); // Pause timer
          if(g_audioManager) g_audioManager->Suspend(); // Suspend audio
         OutputDebugString(L"Window minimized.\n");
         // Optionally release window-size resources here to save memory
         // ReleaseWindowSizeDependentResources();
         return;
     } else if (g_isMinimized) {
         // Resuming from minimized state
         g_isMinimized = false;
         if (g_gameTimer && g_gameTimer->DeltaTime() < 0) g_gameTimer->Start(); // Resume timer
         if(g_audioManager) g_audioManager->Resume(); // Resume audio
         OutputDebugString(L"Window restored.\n");
         // If resources were released on minimize, need to recreate them *before* resizing swap chain
         // CreateWindowSizeDependentResources(); // This might fail if swap chain buffers are wrong size
     }


     OutputDebugString((L"HandleResize called: " + std::to_wstring(width) + L"x" + std::to_wstring(height) + L"\n").c_str());

     // Step 1: Release size-dependent resources
     ReleaseWindowSizeDependentResources();

     // Step 2: Resize the swap chain buffers
     HRESULT hr = g_dxgiSwapChain->ResizeBuffers(
         2, // Number of buffers (match creation)
         g_clientWidth,
         g_clientHeight,
         g_backBufferFormat, // Back buffer format
         0 // Flags (same as creation)
     );

     if (FAILED(hr)) {
          // Handle resize failure (e.g., out of memory)
          MessageBox(g_hWnd, L"Failed to resize swap chain buffers!", L"Error", MB_OK | MB_ICONERROR);
          // This might be a fatal error, consider exiting
          g_exitGame = true;
          return;
     }

     // Step 3: Recreate size-dependent resources (RTV, DSV, Viewports, D2D Target)
     CreateWindowSizeDependentResources();

     OutputDebugString(L"Resize complete.\n");
}

// --- Window Procedure ---

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    // Give Input Manager first chance at mouse input if capturing
    if (g_inputManager && g_inputManager->IsMouseButtonDown(-1) /* IsCapturing() */ ) {
         switch (message) {
         case WM_INPUT:
             g_inputManager->ProcessRawMouseInput(lParam);
             return 0; // Indicate message handled
          case WM_MOUSEMOVE:
                // Can ignore WM_MOUSEMOVE when using raw input for delta
                return 0;
         }
    }


    switch (message) {
    case WM_ACTIVATEAPP:
        if (wParam == TRUE) { // Activated
             if (g_gameTimer && g_gameTimer->DeltaTime() < 0) g_gameTimer->Start();
             if (g_audioManager) g_audioManager->Resume();
             if (g_inputManager) {
                 // Optional: Recapture mouse if it was captured before deactivation
                 // g_inputManager->SetCaptureMouse(wasCapturingBefore);
             }
        } else { // Deactivated
             if (g_gameTimer && g_gameTimer->DeltaTime() >= 0) g_gameTimer->Stop();
             if (g_audioManager) g_audioManager->Suspend();
             if (g_inputManager) {
                 // Optional: Release mouse capture on deactivation
                 // wasCapturingBefore = g_inputManager->IsCapturing();
                  g_inputManager->SetCaptureMouse(false);
             }
        }
        break;

    case WM_SIZE:
        {
            UINT width = LOWORD(lParam);
            UINT height = HIWORD(lParam);

            if (wParam == SIZE_MINIMIZED) {
                 g_isMinimized = true;
                 g_isResizing = false;
                 if (g_gameTimer && g_gameTimer->DeltaTime() >= 0) g_gameTimer->Stop();
                 if (g_audioManager) g_audioManager->Suspend();
                 OutputDebugString(L"WM_SIZE: Minimized\n");
            } else {
                 // Check if restored from minimized or maximized
                 if (g_isMinimized) {
                      g_isMinimized = false;
                      if (g_gameTimer && g_gameTimer->DeltaTime() < 0) g_gameTimer->Start();
                      if (g_audioManager) g_audioManager->Resume();
                 }

                 if (wParam == SIZE_MAXIMIZED) {
                      OutputDebugString(L"WM_SIZE: Maximized\n");
                      g_isResizing = false; // Finished maximizing
                      HandleResize(width, height);
                 } else if (wParam == SIZE_RESTORED) {
                     OutputDebugString(L"WM_SIZE: Restored\n");
                      // Check if we are currently resizing (from dragging borders)
                      if (!g_isResizing) {
                           HandleResize(width, height);
                      }
                      // Else: HandleResize will be called on WM_EXITSIZEMOVE
                 }
            }
        }
        return 0; // Indicate message handled

    case WM_ENTERSIZEMOVE:
        g_isResizing = true;
        if (g_gameTimer && g_gameTimer->DeltaTime() >= 0) g_gameTimer->Stop(); // Pause during resize/move
        OutputDebugString(L"WM_ENTERSIZEMOVE\n");
        return 0;

    case WM_EXITSIZEMOVE:
        g_isResizing = false;
        if (g_gameTimer && g_gameTimer->DeltaTime() < 0) g_gameTimer->Start(); // Resume after resize/move
        OutputDebugString(L"WM_EXITSIZEMOVE\n");
        // Get final size and handle resize
        RECT rc;
        GetClientRect(hWnd, &rc);
        HandleResize(rc.right - rc.left, rc.bottom - rc.top);
        return 0;

    // --- Keyboard Input ---
     case WM_KEYDOWN:
     case WM_SYSKEYDOWN: // Handles Alt key combos
         // Forward to Input Manager? Or handle specific keys here.
         if (wParam == VK_RETURN && (lParam & (1 << 29))) // Alt+Enter
         {
             // Toggle Fullscreen Manually
             BOOL currentFullscreenState;
             ComPtr<IDXGIOutput> targetOutput; // Optional: specify output monitor
             g_dxgiSwapChain->GetFullscreenState(&currentFullscreenState, &targetOutput);

             OutputDebugString(L"Alt+Enter Detected. Toggling Fullscreen.\n");

             // Release size-dependent resources BEFORE changing state
             ReleaseWindowSizeDependentResources();

             HRESULT hr = g_dxgiSwapChain->SetFullscreenState(!currentFullscreenState, targetOutput.Get());
             if (FAILED(hr)) {
                  MessageBox(hWnd, L"Failed to toggle fullscreen state!", L"Error", MB_OK);
                   // Attempt to restore resources anyway
             }

             // Important: Resize buffers AFTER SetFullscreenState call
             // Get the new size (might have changed due to fullscreen mode switch)
             DXGI_SWAP_CHAIN_DESC1 desc;
             g_dxgiSwapChain->GetDesc1(&desc);
             g_clientWidth = desc.Width;
             g_clientHeight = desc.Height;
             GetClientRect(hWnd, &g_clientRect); // Update client rect just in case

             hr = g_dxgiSwapChain->ResizeBuffers(2, g_clientWidth, g_clientHeight, g_backBufferFormat, 0);
             if (FAILED(hr)) {
                  MessageBox(hWnd, L"Failed to resize buffers after fullscreen toggle!", L"Error", MB_OK);
                  g_exitGame = true; // Likely fatal
                  return 0;
             }

             // Recreate size-dependent resources with the new size
             CreateWindowSizeDependentResources();

             g_isFullScreen = !currentFullscreenState; // Update our flag
             OutputDebugString(g_isFullScreen ? L"Entered Fullscreen.\n" : L"Exited Fullscreen.\n");
         }
         // Allow default processing for other keys (e.g., Alt+F4) if not handled by InputManager Update()
         // if(g_inputManager && !g_inputManager->HandleKeyDown(wParam)) {
         //     return DefWindowProc(hWnd, message, wParam, lParam);
         // }
         break; // Let DefWindowProc handle if not Alt+Enter

    case WM_KEYUP:
    case WM_SYSKEYUP:
        // Forward to Input Manager?
        // if(g_inputManager) g_inputManager->HandleKeyUp(wParam);
        break;


     // --- Mouse Input (Non-Raw) ---
     // These are less critical if using Raw Input, but needed for UI interaction maybe
     case WM_MOUSEMOVE:
         // POINT pos = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
         // if(g_inputManager) g_inputManager->UpdateMouseMove(pos); // Update manager if needed
         break;
     case WM_LBUTTONDOWN: /*...*/ break;
     case WM_LBUTTONUP: /*...*/ break;
     case WM_RBUTTONDOWN: /*...*/ break;
     case WM_RBUTTONUP: /*...*/ break;
     case WM_MBUTTONDOWN: /*...*/ break;
     case WM_MBUTTONUP: /*...*/ break;
     case WM_MOUSEWHEEL:
         // int delta = GET_WHEEL_DELTA_WPARAM(wParam);
         // if(g_inputManager) g_inputManager->UpdateMouseWheel(delta);
         break;

     // --- Raw Mouse Input ---
     case WM_INPUT:
          // This is handled at the top if capturing, otherwise could process here
          // Needs RegisterRawInputDevices called first (in InitializeManagers/InputManager)
          // UINT dwSize = 0;
          // GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
          // ... allocate buffer ...
          // GetRawInputData(...)
          // RAWINPUT* raw = ...
          // if (raw->header.dwType == RIM_TYPEMOUSE) {
          //      int dx = raw->data.mouse.lLastX;
          //      int dy = raw->data.mouse.lLastY;
          //      if (g_inputManager) g_inputManager->AccumulateRawMouseDelta(dx, dy);
          // }
          return DefWindowProc(hWnd, message, wParam, lParam); // Let default proc handle cleanup

    case WM_CLOSE:
        if (MessageBox(hWnd, L"Would you like to exit Agrona?", L"Agrona", MB_OKCANCEL | MB_ICONQUESTION) == IDOK) {
             g_exitGame = true; // Signal exit
             DestroyWindow(hWnd); // Initiate destroy sequence
        }
        return 0; // Indicate message handled (we initiated destroy or cancelled)

    case WM_DESTROY:
        g_exitGame = true; // Ensure flag is set
        PostQuitMessage(0); // Posts WM_QUIT to the message queue
        return 0; // Indicate message handled

    default:
        return DefWindowProc(hWnd, message, wParam, lParam); // Default handling
    }

    return DefWindowProc(hWnd, message, wParam, lParam); // Ensure default handling if not returned earlier
}
