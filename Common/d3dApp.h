/*
D3DApp����һ�ֻ�����Direct3DӦ�ó�����,���ṩ�˴���Ӧ�ó��������ڡ�
���г�����Ϣѭ������������Ϣ�Լ���ʼ��Direct3D�ȶ��ֹ��ܵĺ�����
���⣬���໹ΪӦ�ó������̶�����һ���ܺ��������ǿ��Ը�������ͨ��
ʵ����һ���̳��� D3DApp ���࣬��д(override)��ܵ��麯����
�Դ˴�D3DApp�����������Զ�����û����롣 
*/
#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "d3dUtil.h"
#include "GameTimer.h"

// ���������d3d12��
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

class D3DApp
{
protected:

    D3DApp(HINSTANCE hInstance);//���캯�����򵥵ؽ����ݳ�ԱĬ�ϻ�Ϊ��ʼֵ
    D3DApp(const D3DApp& rhs) = delete;//ɾ���������캯��
    D3DApp& operator=(const D3DApp& rhs) = delete;//ɾ��������ֵ�����
    virtual ~D3DApp();//��������

public:

    static D3DApp* GetApp();
    
	HINSTANCE AppInst()const;//����Ӧ�ó���ʵ�����
	HWND      MainWnd()const;//���������ھ��
	float     AspectRatio()const;//�����ݺ�ȣ���̨�������Ŀ����߶�֮�ȣ�


    bool Get4xMsaaState()const;//�������4X MSAA�ͷ���true�����򷵻�false��
    void Set4xMsaaState(bool value);//���������4X MSAA���ܡ�

	int Run();//��װ��Ӧ�ó������Ϣѭ��
 
    virtual bool Initialize();
    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
    virtual void CreateRtvAndDsvDescriptorHeaps();
	virtual void OnResize(); 
	virtual void Update(const GameTimer& gt)=0;
    virtual void Draw(const GameTimer& gt)=0;

	// ������д���������Ϣ�Ĵ�������
	virtual void OnMouseDown(WPARAM btnState, int x, int y){ }
	virtual void OnMouseUp(WPARAM btnState, int x, int y)  { }
	virtual void OnMouseMove(WPARAM btnState, int x, int y){ }

protected:

	bool InitMainWindow();
	bool InitDirect3D();
	void CreateCommandObjects();
    void CreateSwapChain();

	void FlushCommandQueue();

	ID3D12Resource* CurrentBackBuffer()const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

	void CalculateFrameStats();

    void LogAdapters();
    void LogAdapterOutputs(IDXGIAdapter* adapter);
    void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

protected:

    static D3DApp* mApp;

    HINSTANCE mhAppInst = nullptr; // Ӧ�ó���ʵ�����
    HWND      mhMainWnd = nullptr; // �����ھ��
	bool      mAppPaused = false;  // Ӧ�ó����Ƿ���ͣ��
	bool      mMinimized = false;  // Ӧ�ó����Ƿ���С����
	bool      mMaximized = false;  // Ӧ�ó����Ƿ���󻯣�
    bool      mResizing = false;   // ��С�������Ƿ��յ���ק��
    bool      mFullscreenState = false;// �Ƿ���ȫ��ģʽ

	// ������ѡ������Ϊtrue����ʹ��4X MSAA������Ĭ��ֵΪfalse
    bool      m4xMsaaState = false;    // �Ƿ���4X MSAA 
    UINT      m4xMsaaQuality = 0;      // 4X MSAA����������

	// ���ڼ�¼"delta-time"(֮֡���ʱ����������Ϸ��ʱ��
	GameTimer mTimer;
	
    //IDXGIFactory�� DXGI�еĹؼ��ӿ�֮һ,
    //��Ҫ���ڴ���IDXGISwapChain�ӿ��Լ�ö����ʾ��������
    Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;
    Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
    Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;

    Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
    UINT64 mCurrentFence = 0;
	
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

	static const int SwapChainBufferCount = 2;
	int mCurrBackBuffer = 0;
    Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
    Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

    D3D12_VIEWPORT mScreenViewport; 
    D3D12_RECT mScissorRect;

	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;

	//�û�Ӧ������������������캯�����Զ�����Щ��ʼֵ
	std::wstring mMainWndCaption = L"d3d App";
	D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	int mClientWidth = 800;
	int mClientHeight = 600;
};

