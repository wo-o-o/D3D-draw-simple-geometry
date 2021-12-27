/*
D3DApp类是一种基础的Direct3D应用程序类,它提供了创建应用程序主窗口、
运行程序消息循环、处理窗口消息以及初始化Direct3D等多种功能的函数。
此外，该类还为应用程序例程定义了一组框架函数。我们可以根据需求通过
实例化一个继承自 D3DApp 的类，重写(override)框架的虚函数，
以此从D3DApp类中派生出自定义的用户代码。 
*/
#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "d3dUtil.h"
#include "GameTimer.h"

// 链接所需的d3d12库
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

class D3DApp
{
protected:

    D3DApp(HINSTANCE hInstance);//构造函数，简单地将数据成员默认化为初始值
    D3DApp(const D3DApp& rhs) = delete;//删除拷贝构造函数
    D3DApp& operator=(const D3DApp& rhs) = delete;//删除拷贝赋值运算符
    virtual ~D3DApp();//析构函数

public:

    static D3DApp* GetApp();
    
	HINSTANCE AppInst()const;//返回应用程序实例句柄
	HWND      MainWnd()const;//返回主窗口句柄
	float     AspectRatio()const;//返回纵横比（后台缓冲区的宽度与高度之比）


    bool Get4xMsaaState()const;//如果启用4X MSAA就返回true，否则返回false。
    void Set4xMsaaState(bool value);//开启或禁用4X MSAA功能。

	int Run();//封装了应用程序的消息循环
 
    virtual bool Initialize();
    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
    virtual void CreateRtvAndDsvDescriptorHeaps();
	virtual void OnResize(); 
	virtual void Update(const GameTimer& gt)=0;
    virtual void Draw(const GameTimer& gt)=0;

	// 便于重写鼠标输入消息的处理流程
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

    HINSTANCE mhAppInst = nullptr; // 应用程序实例句柄
    HWND      mhMainWnd = nullptr; // 主窗口句柄
	bool      mAppPaused = false;  // 应用程序是否暂停？
	bool      mMinimized = false;  // 应用程序是否最小化？
	bool      mMaximized = false;  // 应用程序是否最大化？
    bool      mResizing = false;   // 大小调整栏是否收到拖拽？
    bool      mFullscreenState = false;// 是否开启全屏模式

	// 若将该选项设置为true，则使用4X MSAA技术。默认值为false
    bool      m4xMsaaState = false;    // 是否开启4X MSAA 
    UINT      m4xMsaaQuality = 0;      // 4X MSAA的质量级别

	// 用于记录"delta-time"(帧之间的时间间隔）和游戏总时间
	GameTimer mTimer;
	
    //IDXGIFactory是 DXGI中的关键接口之一,
    //主要用于创建IDXGISwapChain接口以及枚举显示适配器。
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

	//用户应该在派生类的派生构造函数中自定义这些初始值
	std::wstring mMainWndCaption = L"d3d App";
	D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	int mClientWidth = 800;
	int mClientHeight = 600;
};

