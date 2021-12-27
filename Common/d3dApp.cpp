#include "d3dApp.h"
#include <WindowsX.h>

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Forward hwnd on because we can get messages (e.g., WM_CREATE)
	// before CreateWindow returns, and thus before mhMainWnd is valid.    
	return D3DApp::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

D3DApp* D3DApp::mApp = nullptr;
D3DApp* D3DApp::GetApp()
{
    return mApp;
}

D3DApp::D3DApp(HINSTANCE hInstance)
:	mhAppInst(hInstance)
{
    // ֻ�ܹ���һ�� D3DApp��
    assert(mApp == nullptr);
    mApp = this;
}

D3DApp::~D3DApp()
{
	if(md3dDevice != nullptr)
		FlushCommandQueue();
}

HINSTANCE D3DApp::AppInst()const
{
	return mhAppInst;
}

HWND D3DApp::MainWnd()const
{
	return mhMainWnd;
}

float D3DApp::AspectRatio()const
{
	return static_cast<float>(mClientWidth) / mClientHeight;
}

bool D3DApp::Get4xMsaaState()const
{
    return m4xMsaaState;
}

void D3DApp::Set4xMsaaState(bool value)
{
    if(m4xMsaaState != value)
    {
        m4xMsaaState = value;

        // ʹ���µĶ��ز����������´����������ͻ�������
        CreateSwapChain();
        OnResize();
    }
}

int D3DApp::Run()
{
	MSG msg = {0};
 
	mTimer.Reset();

	while(msg.message != WM_QUIT)
	{
		// �����Ϣ�������д�����Ϣ���������ǡ�
		if(PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
		{
            TranslateMessage( &msg );
            DispatchMessage( &msg );
		}
		// ����ִ�ж�������Ϸ�߼����ֵĴ��� 
		else
        {	
			mTimer.Tick();

			if( !mAppPaused )
			{
				CalculateFrameStats();
				Update(mTimer);	
                Draw(mTimer);
			}
			else
			{
				Sleep(100);
			}
        }
    }

	return (int)msg.wParam;
}

bool D3DApp::Initialize()
{
	if(!InitMainWindow())
		return false;

	if(!InitDirect3D())
		return false;

    // ִ�г�ʼ������С����
    OnResize();

	return true;
}
 
void D3DApp::CreateRtvAndDsvDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
        &rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));


    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
        &dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

//MsgProc�������յ� WM_SIZE��Ϣʱ�����ô˷����������ڵĴ�С�����˸ı䣬һЩ�빤������С�йص�Direct3D����Ҳ��Ҫ��֮�������ر��Ǻ�̨�������Լ����/ģ�建������Ϊ��ƥ�䴰�ڹ�����������Ĵ�С��Ҫ�������´�����ͨ������IDXGISwapChain: :ResizeBuffers������������̨�������ĳߴ硣�������/ģ�建�������ԣ�����Ҫ�����ٺ�����µĹ�������С�����ؽ������⣬��ȾĿ������/ģ�����ͼҲӦ���´�����������Щ���������⣬�����ڹ�������С���������ԣ���ͶӰ����projection matrix )ҲҪ�ڴ�����Ӧ���޸ġ�

void D3DApp::OnResize()
{
	assert(md3dDevice);
	assert(mSwapChain);
    assert(mDirectCmdListAlloc);

	// Flush before changing any resources.
	FlushCommandQueue();

    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Release the previous resources we will be recreating.
	for (int i = 0; i < SwapChainBufferCount; ++i)
		mSwapChainBuffer[i].Reset();
    mDepthStencilBuffer.Reset();
	
	// Resize the swap chain.
    ThrowIfFailed(mSwapChain->ResizeBuffers(
		SwapChainBufferCount, 
		mClientWidth, mClientHeight, 
		mBackBufferFormat, 
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	mCurrBackBuffer = 0;
 
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SwapChainBufferCount; i++)
	{
		//��ý������ڵĵ�i��������
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
		//Ϊ�˻���������һ��RTV
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		//ƫ�Ƶ����������е���һ��������
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

    // �������/ģ�建����������ͼ
    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = mClientWidth;
    depthStencilDesc.Height = mClientHeight;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = mDepthStencilFormat;
    depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	//������һ�����������Դ���Ż�ֵ��ѡ���ʵ����Ż����ֵ����������������ִ���ٶȡ�
    D3D12_CLEAR_VALUE optClear;
    optClear.Format = mDepthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;
	auto c = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &c,
		D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

    //���ô���Դ�ĸ�ʽ��Ϊ������Դ�ĵ�0 mip�㴴��������
    md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, DepthStencilView());

    // ����Դ�ӳ�ʼ״̬ת��Ϊ��Ȼ�����
	auto f = CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	mCommandList->ResourceBarrier(1, &f);
	
    // Execute the resize commands.
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until resize is complete.
	FlushCommandQueue();

	// Update the viewport transform to cover the client area.
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width    = static_cast<float>(mClientWidth);
	mScreenViewport.Height   = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

    mScissorRect = { 0, 0, mClientWidth, mClientHeight };
}
 
LRESULT D3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch( msg )
	{
	//WM_ACTIVATE �ڴ��ڼ����ͣ��ʱ���͡�
	//�����ڴ���ͣ��ʱ��ͣ��Ϸ������������ʱȡ����ͣ��
	case WM_ACTIVATE:
		if( LOWORD(wParam) == WA_INACTIVE )
		{
			mAppPaused = true;
			mTimer.Stop();
		}
		else
		{
			mAppPaused = false;
			mTimer.Start();
		}
		return 0;

	// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
		// Save the new client area dimensions.
		mClientWidth  = LOWORD(lParam);
		mClientHeight = HIWORD(lParam);
		if( md3dDevice )
		{
			if( wParam == SIZE_MINIMIZED )
			{
				mAppPaused = true;
				mMinimized = true;
				mMaximized = false;
			}
			else if( wParam == SIZE_MAXIMIZED )
			{
				mAppPaused = false;
				mMinimized = false;
				mMaximized = true;
				OnResize();
			}
			else if( wParam == SIZE_RESTORED )
			{
				
				// Restoring from minimized state?
				if( mMinimized )
				{
					mAppPaused = false;
					mMinimized = false;
					OnResize();
				}

				// Restoring from maximized state?
				else if( mMaximized )
				{
					mAppPaused = false;
					mMaximized = false;
					OnResize();
				}
				else if( mResizing )
				{
					// If user is dragging the resize bars, we do not resize 
					// the buffers here because as the user continuously 
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is 
					// done resizing the window and releases the resize bars, which 
					// sends a WM_EXITSIZEMOVE message.
				}
				else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}
		return 0;

	// ���û�ץȡ������ʱ����WM_ENTERSIZEMOVE��Ϣ
	case WM_ENTERSIZEMOVE:
		mAppPaused = true;
		mResizing  = true;
		mTimer.Stop();
		return 0;

	// ���û��ͷŵ�����ʱ����WM_EXITSIZEMOVE��Ϣ
    //�˴��������µĴ��ڴ�С������ض����绺��������ͼ�ȣ�

	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mResizing  = false;
		mTimer.Start();
		OnResize();
		return 0;
 
	// �����ڱ�����ʱ����WM_DESTROY��Ϣ
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	//��ĳһ�˵����ڼ���״̬�������û����µļȲ������Ǽ�(mnemonic key)Ҳ���Ǽ��ټ�(acceleratorkey)ʱ���ͷ���WM_MENUCHAR��Ϣ
	case WM_MENUCHAR:
        // ��������ϼ�alt-enterʱ������beep������
        return MAKELRESULT(0, MNC_CLOSE);

	// �������Ϣ�Է����ڱ�ù�С
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200; 
		return 0;

    //Ϊ���ڴ����е��������Լ���д����������麯����Ҫ�����·�ʽ������������йص���Ϣ
	//Ϊ��ʹ��GET_X_LPARAM��GET_Y_LPARAM�����꣬���Ǳ�������#include <Windowsx.h>
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;


    case WM_KEYUP:
        if(wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
        }
        else if((int)wParam == VK_F2)
            Set4xMsaaState(!m4xMsaaState);

        return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);}

bool D3DApp::InitMainWindow()
{
	WNDCLASS wc;
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = MainWndProc; 
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = mhAppInst;
	wc.hIcon         = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor       = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = L"MainWnd";

	if( !RegisterClass(&wc) )
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	// ��������Ŀͻ����ߴ���㴰�ھ��γߴ�

	RECT R = { 0, 0, mClientWidth, mClientHeight };//RECT �ṹͨ�������ϽǺ����½ǵ����궨��һ�����Ρ�
	//��������Ŀͻ��˾��δ�С��������Ĵ��ھ��δ�С��
    //Ȼ����Խ����ھ��δ��ݸ�CreateWindow�����Դ���һ���ͻ���Ϊ�����С�Ĵ��ڡ�
	//����������ʱ���ýṹ�����������ϽǺ����½ǵ���������������Ŀͻ�����
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width  = R.right - R.left;
	int height = R.bottom - R.top;

	mhMainWnd = CreateWindow(L"MainWnd", mMainWndCaption.c_str(),	
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhAppInst, 0); 
	if( !mhMainWnd )
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);

	return true;
}

bool D3DApp::InitDirect3D()
{
#if defined(DEBUG) || defined(_DEBUG) 
	// ����D3D12�ĵ��Բ�
{
	ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();
}
#endif

	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));

	// ���Դ���Ӳ���豸
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             // Ĭ��������
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&md3dDevice));

	// ����D3D12CreateDevice ʧ�ܺ�,������ WARP �豸.
	if(FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> pWarpAdapter;
		//ͨ��IDXGIFactory4������ö��WARP������
		ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&md3dDevice)));
	}
	//����Χ������ȡ�������Ĵ�С���������������й�������Ҫ�˽����ǵĴ�С��
    //���������ڲ�ͬ��GPUƽ̨�ϴ�С���죬��Ҫ��ѯ��ص���Ϣ��
	//�����������Ĵ�С������������Ҫʱ����ֱ������.

	ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mFence)));

	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);



    // ������ǵĺ�̨��������ʽ�� 4X MSAA ��������
	//����֧�� Direct3D 11 ���豸��֧��������Ⱦ�� 4X MSAA
	//Ŀ���ʽ����������ֻ��Ҫ�����������
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;

	//���ݸ����������ʽ�Ͳ������������Ǿ�����ID3D12Device::
	//CheckFeaturesupport������ѯ����Ӧ����������:
	//ע�⣬�˷����ĵڶ�����������������������ԡ�
	//������Ϊ�������ʱ�����Ǳ���ָ�������ʽ����������
	//�Լ�ϣ����ѯ�Ķ��ز�����֧�ֵı�־������flag���������)��
	//���ţ�������ִ�к�����дͼ������������Ϊ�����
	ThrowIfFailed(md3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));

    m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");
	


#ifdef _DEBUG
    LogAdapters();
#endif

	//����������к������б�
	CreateCommandObjects();
	//����������������
    CreateSwapChain();
	//������������
    CreateRtvAndDsvDescriptorHeaps();

	return true;
}

void D3DApp::CreateCommandObjects()
{
	//ͨ����дD3D12_COMMAND_QUEUE_DESC�ṹ�����������У�
    //�ٵ���ID3D12Device::CreateCommandQueue�����������С�
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	ThrowIfFailed(md3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));

	ThrowIfFailed(md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mDirectCmdListAlloc.Get(), // ����������䱸��
		nullptr,                   // ��ʼ����ˮ��״̬����
		//����ˮ��״̬����( pipeline state object)��һ����ָ��Ϊ�˿�ָ�롣
		//������ʱ���ᷢ���κλ����������Ҳ�Ͳ����õ���ˮ��״̬����

		IID_PPV_ARGS(mCommandList.GetAddressOf())));

	// ����Ҫ�������б����ڹر�״̬��������Ϊ�ڵ�һ�����������б�ʱ��
	//����Ҫ�����������ã����ڵ������÷���֮ǰ�����Ƚ���ر�
	mCommandList->Close();
}

void D3DApp::CreateSwapChain()
{
    // �ͷ�֮ǰ�����Ľ�����,����ٽ����ؽ�
    mSwapChain.Reset();

    DXGI_SWAP_CHAIN_DESC sd;
    sd.BufferDesc.Width = mClientWidth;
    sd.BufferDesc.Height = mClientHeight;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = mBackBufferFormat;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = SwapChainBufferCount;
    sd.OutputWindow = mhMainWnd;
    sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// ע��,��������Ҫͨ��������ж������ˢ��
    ThrowIfFailed(mdxgiFactory->CreateSwapChain(
		mCommandQueue.Get(),
		&sd, 
		mSwapChain.GetAddressOf()));
}

void D3DApp::FlushCommandQueue()
{
	// ����Χ��ֵ,�������������ǵ���Χ����
    mCurrentFence++;

    // ��������������һ������������Χ���������
	// ������������Ҫ����GPU����(����GPU�����޸�Χ��ֵ)��������GPU������
	// ��������д�Signal()����������֮ǰ, �������������µ�Χ����

    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

	//��CPU�˵ȴ�GPU��ֱ������ִ�������Χ����֮ǰ����������
    if(mFence->GetCompletedValue() < mCurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, LPCWSTR(false), false, EVENT_ALL_ACCESS);

        // ��GPU���е�ǰ��Χ������ִ�е�signal()ָ��޸���Χ��ֵ)���򼤷�Ԥ���¼�  
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

        // �ȴ�GPU����Χ���������¼�
		WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
	}
}

ID3D12Resource* D3DApp::CurrentBackBuffer()const
{
	return mSwapChainBuffer[mCurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::CurrentBackBufferView()const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		mCurrBackBuffer,
		mRtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::DepthStencilView()const
{
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void D3DApp::CalculateFrameStats()
{
	// ��δ��������ÿ���ƽ��֡����Ҳ������ÿ֡��ƽ����Ⱦʱ��
	// ��Щͳ��ֵ���ᱻ���ӵ����ڵı�������

	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// ��1��Ϊͳ������������ƽ��֡���Լ�ÿ֡��ƽ����Ⱦʱ��
	if( (mTimer.TotalTime() - timeElapsed) >= 1.0f )
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

        wstring fpsStr = to_wstring(fps);
        wstring mspfStr = to_wstring(mspf);

        wstring windowText = mMainWndCaption +
            L"    fps: " + fpsStr +
            L"   mspf: " + mspfStr;

        SetWindowText(mhMainWnd, windowText.c_str());
		
		// Ϊ������һ��ƽ��ֵ������
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

//����������Ĵ�����ö��һ��ϵͳ�е�����������
void D3DApp::LogAdapters()
{
    UINT i = 0;
    IDXGIAdapter* adapter = nullptr;
    std::vector<IDXGIAdapter*> adapterList;
    while(mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);

        std::wstring text = L"***Adapter: ";
        text += desc.Description;
        text += L"\n";

        OutputDebugString(text.c_str());

        adapterList.push_back(adapter);
        
        ++i;
    }

    for(size_t i = 0; i < adapterList.size(); ++i)
    {
        LogAdapterOutputs(adapterList[i]);
        ReleaseCom(adapterList[i]);
    }
}

//ͨ�����´��룬�Ϳ���ö�ٳ���ĳ��������������������ʾ���
void D3DApp::LogAdapterOutputs(IDXGIAdapter* adapter)
{
    UINT i = 0;
    IDXGIOutput* output = nullptr;
    while(adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);
        
        std::wstring text = L"***Output: ";
        text += desc.DeviceName;
        text += L"\n";
        OutputDebugString(text.c_str());

        LogOutputDisplayModes(output, mBackBufferFormat);

        ReleaseCom(output);

        ++i;
    }
}

//һ��ȷ������ʾģʽ�ľ����ʽ(�� DXGI_FORMAT)��
//����ͨ�����д��룬���ĳ����ʾ����Դ˸�ʽ��֧�ֵ�ȫ����ʾģʽ
void D3DApp::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
    UINT count = 0;
    UINT flags = 0;

    //��nullptr��Ϊ�������ô˺�������ȡ������������ʾģʽ�ĸ���
    output->GetDisplayModeList(format, flags, &count, nullptr);

    std::vector<DXGI_MODE_DESC> modeList(count);
    output->GetDisplayModeList(format, flags, &count, &modeList[0]);

    for(auto& x : modeList)
    {
        UINT n = x.RefreshRate.Numerator;
        UINT d = x.RefreshRate.Denominator;
        std::wstring text =
            L"Width = " + std::to_wstring(x.Width) + L" " +
            L"Height = " + std::to_wstring(x.Height) + L" " +
            L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
            L"\n";

        ::OutputDebugString(text.c_str());
    }
}
//�ڽ���ȫ��ģʽ֮ʱ��ö����ʾģʽ���Ե���Ϊ��Ҫ��
//Ϊ�˻�����ŵ�ȫ�����ܣ�������ָ������ʾģʽ(����ˢ���ʣ�
//һ��Ҫ����ʾ��֧�ֵ���ʾģʽ��ȫƥ�䡣
//����ö�ٳ�������ʾģʽ����ѡ��������Ա�֤��һ�㡣
