#include "pch.h"
#include "Graphic_Device.h"

Graphic_Device::Graphic_Device()
{

}

Graphic_Device::~Graphic_Device()
{

}

HRESULT Graphic_Device::Initialize(HWND hWnd, EWinMode eWinMode, uint32 winSizeX, uint32 winSizeY,
                                    ComPtr<Device>& deviceOut, ComPtr<DeviceContext>& contextOut)
{
    uint32 flag = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    _width  = winSizeX;
    _height = winSizeY;

    _originalWidth = winSizeX;
    _originalHeight = winSizeY;

#ifdef _DEBUG
    //flag = D3D11_CREATE_DEVICE_DEBUG;
    //flag = 0;
#endif

    D3D_FEATURE_LEVEL FeatureLV;

    /* Dx11 : 우선적으로 장치 객체를 생성하고, 장치 객체를 통해 기타 초기화 작업 및 설정을 해나간다. */

    /* 그래픽 장치를 초기화한다. */
    if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, flag, nullptr, 0, D3D11_SDK_VERSION,
        _device.GetAddressOf(), &FeatureLV, _context.GetAddressOf())))
        return E_FAIL;

    /* SawpChain : 더블버퍼링. 전면과 후면 버퍼를 번갈아가면서 화면에 보여준다. (Present) */

    /* 스왑체인 객체를 생성하였고 생성한 스왑체인 객체가 백버퍼를 내장한다. */
    if (FAILED(Ready_SwapChain(hWnd, eWinMode, winSizeX, winSizeY)))
        return E_FAIL;

    /* 스왑체인이 들고 있는 텍스쳐 2D를 가져와서 이를 바탕으로 백버퍼 렌더타겟 뷰를 만든다. */
    if (FAILED(Ready_BackBuffer_RenderTargetView()))
        return E_FAIL;

    if (FAILED(Ready_DepthStencilView(winSizeX, winSizeY)))
        return E_FAIL;

    /* 장치에 바인드해놓을 렌더 타겟들과 뎁스스텐실뷰를 세팅한다. */
    /* 장치는 동시에 최대 4->8개의 렌더타겟을 들고 있을 수 있다. */
    ID3D11RenderTargetView* pRTVs[] = {
       _renderTarget.Get(),
    };

    /* 렌더타겟의 픽셀 수와 깊이스텐실버퍼의 픽셀수가 서로 다르다면 절대 렌더링이 불가능해진다. */
    _context->OMSetRenderTargets(1, pRTVs, _depthStencil.Get());

    D3D11_VIEWPORT ViewPortDesc;
    ZeroMemory(&ViewPortDesc, sizeof(D3D11_VIEWPORT));
    ViewPortDesc.TopLeftX = 0;
    ViewPortDesc.TopLeftY = 0;
    ViewPortDesc.Width = static_cast<float>(winSizeX);
    ViewPortDesc.Height = static_cast<float>(winSizeY);
    ViewPortDesc.MinDepth = 0.f;
    ViewPortDesc.MaxDepth = 1.f;

    _context->RSSetViewports(1, &ViewPortDesc);

    deviceOut = _device;
    contextOut = _context;

    return S_OK;
}

HRESULT Graphic_Device::Clear_BackBufferView(const Color& clearColor)
{
    if (_context == nullptr)
        return E_FAIL;

    ID3D11RenderTargetView* pRTVs[] = { _renderTarget.Get() };
    _context->OMSetRenderTargets(1, pRTVs, _depthStencil.Get());

    D3D11_VIEWPORT vp;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width = _width;
    vp.Height = _height;
    vp.MinDepth = 0.f;
    vp.MaxDepth = 1.f;
    _context->RSSetViewports(1, &vp);

    /* DX9기준 : Clear함수는 백버퍼, 깊이스텐실버퍼를 한꺼번에 지운다. */
    /* 백버퍼를 초기화한다. */
    float colorArray[4] = { clearColor.x, clearColor.y, clearColor.z, clearColor.w };
    _context->ClearRenderTargetView(_renderTarget.Get(), colorArray);

    return S_OK;
}

HRESULT Graphic_Device::Clear_DepthStencil_View()
{
    if (_context == nullptr)
        return E_FAIL;

    _context->ClearDepthStencilView(
        _depthStencil.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

    return S_OK;
}

HRESULT Graphic_Device::Present()
{
    if (_swapChain == nullptr)
        return E_FAIL;

    /* 전면 버퍼와 후면 버퍼를 교체하여 후면 버퍼를 전면으로 보여주는 역할을 한다. */
    /* 후면 버퍼를 직접 화면에 보여줄게. */
    return _swapChain->Present(0, 0);
}

void Graphic_Device::BindBackBuffer()
{
    // RenderTarget을 백버퍼로 다시 설정
    ID3D11RenderTargetView* pRTVs[] = { _renderTarget.Get() };
    _context->OMSetRenderTargets(1, pRTVs, _depthStencil.Get());

    // Viewport 복원
    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = 0.f;
    vp.TopLeftY = 0.f;
    vp.Width = _width;
    vp.Height = _height;
    vp.MinDepth = 0.f;
    vp.MaxDepth = 1.f;

    _context->RSSetViewports(1, &vp);
}

HRESULT Graphic_Device::Resize(uint32 width, uint32 height)
{
    _width = width;
    _height = height;

    if (_swapChain == nullptr) return E_FAIL;
    _context->OMSetRenderTargets(0, 0, 0);

    // 1. 기존 View 해제
    _renderTarget.Reset();
    _depthStencil.Reset();

    // 2. 버퍼 크기 변경
    if (FAILED(_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0)))
        return E_FAIL;

    // 3. View 재생성
    if (FAILED(Ready_BackBuffer_RenderTargetView()))
        return E_FAIL;
    if (FAILED(Ready_DepthStencilView(width, height)))
        return E_FAIL;

    return S_OK;
}

HRESULT Graphic_Device::Ready_SwapChain(HWND hWnd, EWinMode eWinMode, uint32 winSizeX, uint32 winSizeY)
{
    ComPtr<IDXGIDevice> pDXGIDevice;
    _device.As(&pDXGIDevice);

    ComPtr<IDXGIAdapter> pAdapter;
    pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void**)pAdapter.GetAddressOf());

    ComPtr<IDXGIFactory> pFactory;
    pAdapter->GetParent(__uuidof(IDXGIFactory), (void**)pFactory.GetAddressOf());

    /* 스왑체인을 생성한다. = 텍스쳐를 생성하는 행위 + 스왑하는 형태 */
    DXGI_SWAP_CHAIN_DESC SwapChainDesc;
    ZeroMemory(&SwapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));

    /* 백버퍼 == 텍스쳐 */
    /* 텍스처(백버퍼 == ID3D11Texture2D)를 생성하는 행위 */
    SwapChainDesc.BufferDesc.Width = winSizeX;   /* 가로 픽셀 수 */
    SwapChainDesc.BufferDesc.Height = winSizeY;  /* 세로 픽셀 수 */

    SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; /* 32BIT 픽셀생성 */
    SwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    SwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    /* RENDER_TARGET : 그림을 당하는 대상. 스케치북 */
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.BufferCount = 1;

    /* 스왑하는 형태 : 모니터 주사율에 따라 조절해도 됨. */
    SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;

    /* 멀티샘플링 : 안티얼라이징 (계단현상방지) */
    SwapChainDesc.SampleDesc.Quality = 0;
    SwapChainDesc.SampleDesc.Count = 1;

    SwapChainDesc.OutputWindow = hWnd;
    SwapChainDesc.Windowed = (eWinMode == EWinMode::Win);
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    /* 백버퍼라는 텍스처(ID3D11Texture2D)를 생성했다. */
    if (FAILED(pFactory->CreateSwapChain(_device.Get(), &SwapChainDesc, _swapChain.GetAddressOf())))
        return E_FAIL;

    return S_OK;
}

HRESULT Graphic_Device::Ready_BackBuffer_RenderTargetView()
{
    if (_device == nullptr)
        return E_FAIL;

    ComPtr<Texture2D> pBackBufferTexture;

    /* 스왑체인이 들고있던 텍스처를 가져와봐. */
    if (FAILED(_swapChain->GetBuffer(0, __uuidof(Texture2D), (void**)pBackBufferTexture.GetAddressOf())))
        return E_FAIL;

    /* 실제 렌더타겟용도로 사용할 수 있는 텍스쳐 타입(ID3D11RenderTargetView)의 객체를 생성한다. */
    if (FAILED(_device->CreateRenderTargetView(pBackBufferTexture.Get(), nullptr, _renderTarget.GetAddressOf())))
        return E_FAIL;

    return S_OK;
}

HRESULT Graphic_Device::Ready_DepthStencilView(uint32 winSizeX, uint32 winSizeY)
{
    if (_device == nullptr)
        return E_FAIL;

    D3D11_TEXTURE2D_DESC TextureDesc{};

    /* 깊이 버퍼의 픽셀은 백버퍼의 픽셀과 갯수가 동일해야만 깊이 테스트가 가능해진다. */
    TextureDesc.Width = winSizeX;
    TextureDesc.Height = winSizeY;
    TextureDesc.MipLevels = 1;
    TextureDesc.ArraySize = 1;
    TextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    TextureDesc.SampleDesc.Quality = 0;
    TextureDesc.SampleDesc.Count = 1;
    TextureDesc.Usage = D3D11_USAGE_DEFAULT;
    TextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    TextureDesc.CPUAccessFlags = 0;
    TextureDesc.MiscFlags = 0;

    ComPtr<Texture2D> pDepthStencilTexture;
    if (FAILED(_device->CreateTexture2D(&TextureDesc, nullptr, pDepthStencilTexture.GetAddressOf())))
        return E_FAIL;

    if (FAILED(_device->CreateDepthStencilView(pDepthStencilTexture.Get(), nullptr, _depthStencil.GetAddressOf())))
        return E_FAIL;

    return S_OK;
}

unique_ptr<Graphic_Device> Graphic_Device::Create(HWND hWnd, EWinMode eWinMode, uint32 winSizeX, uint32 winSizeY,
    ComPtr<Device>& deviceOut, ComPtr<DeviceContext>& contextOut)
{
    auto instance = make_unique<Graphic_Device>();

    if (FAILED(instance->Initialize(hWnd, eWinMode, winSizeX, winSizeY, deviceOut, contextOut)))
    {
        MSG_BOX("Faield to Created : GraphicDevice");

        return nullptr;
    }

    return instance;
}

void Graphic_Device::Free()
{
    Base::Free();

    /* 디버그 모드에서 메모리 누수 체크 */
#if defined(DEBUG) || defined(_DEBUG)
    ComPtr<ID3D11Debug> d3dDebug;
    if (SUCCEEDED(_device->QueryInterface(__uuidof(ID3D11Debug), (void**)d3dDebug.GetAddressOf())))
    {
        OutputDebugStringW(L"----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- \r ");
        OutputDebugStringW(L"                                                                    D3D11 Live Object ref Count Checker \r ");
        OutputDebugStringW(L"----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- \r ");
        d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
        OutputDebugStringW(L"----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- \r ");
        OutputDebugStringW(L"                                                                    D3D11 Live Object ref Count Checker END \r ");
        OutputDebugStringW(L"----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- \r ");
    }
#endif
}
