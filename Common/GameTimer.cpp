#include <windows.h>
#include "GameTimer.h"

GameTimer::GameTimer(): mSecondsPerCount(0.0), mDeltaTime(-1.0), mBaseTime(0), mPausedTime(0), mPrevTime(0), mCurrTime(0), mStopped(false)
{
	//为了精确地度量时间，我们将采用性能计时器( performance timer。或称性能计数器，performance counter)。
	//如果希望调用查询性能计时器的 Win32函数,我们必须引入头文件#include<windows.h>。

	//用QueryPerformanceFrequency函数来获取性能计时器的频率（单位:计数/秒)
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);

	//每个计数所代表的秒数（或称几分之一秒)，即为上述性能计时器频率的倒数
	mSecondsPerCount = 1.0 / (double)countsPerSec;
}

//返回自调用 Reset() 以来经过的总时间，
//不计算任何时间时钟停止的时间。
float GameTimer::TotalTime()const
{
	//如果正处于停止状态，则忽略本次停止时刻至当前时刻的这段时间。此外，如果之前已有过暂停的情况，
	//那么也不应统计mStopTime - mBaseTime这段时间内的暂停时间
    //为了做到这一点，可以从mStopTime 中再减去暂停时间mPausedTime

	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mStopTime    mCurrTime

	if( mStopped )
	{
		return (float)(((mStopTime - mPausedTime)-mBaseTime)*mSecondsPerCount);
	}

	//我们并不希望统计mCurrTime - mBaseTime内的暂停时间
	//可以通过从mcurrTime 中再减去暂停时间mPausedTime来实现这一点
	// 
	//  (mCurrTime - mPausedTime) - mBaseTime 
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mCurrTime
	
	else
	{
		return (float)(((mCurrTime-mPausedTime)-mBaseTime)*mSecondsPerCount);
	}
}

float GameTimer::DeltaTime()const
{
	return (float)mDeltaTime;
}

void GameTimer::Reset()
{
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	mBaseTime = currTime;
	mPrevTime = currTime;
	mStopTime = 0;
	mStopped  = false;
}

void GameTimer::Start()
{
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);


	// 累加调用stop和start这对方法之间的暂停时刻间隔
	//
	//                     |<-------d------->|
	// ----*---------------*-----------------*------------> time
	//  mBaseTime       mStopTime        startTime     

	//如果从停止状态继续计时的话…
	if( mStopped )
	{
		//累加暂停时间
		mPausedTime += (startTime - mStopTime);	

		//在重新开启计时器时，前一帧的时间mPrevTime是无效的，这是因为它存储的是暂停时前一
		//帧的开始时刻，因此需要将它重置为当前时刻
		mPrevTime = startTime;

		//已经不是停止状态
		mStopTime = 0;
		mStopped  = false;
	}
}

void GameTimer::Stop()
{
	//如果已经处于停止状态，那就什么也不做
	if( !mStopped )
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

		//否则，保存停止的时刻，并设置布尔标志, 指示计时器已经停止
		mStopTime = currTime;
		mStopped  = true;
	}
}

void GameTimer::Tick()
{
	if( mStopped )
	{
		mDeltaTime = 0.0;
		return;
	}

	//获得本帧开始显示的时刻
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	mCurrTime = currTime;

	// 本帧与前一帧的时间差
	mDeltaTime = (mCurrTime - mPrevTime)*mSecondsPerCount;

	// 准备计算本帧与下一帧的时间差
	mPrevTime = mCurrTime;

	//使时间差为非负值。DXSDK 中的CDXUTTimer示例注释里提到:如果处理器处于节能模式，或者在
	//计算两帧间时间差的过程中切换到另一个处理器时(即QueryPerformanceCounter函数的两次调
	//用并非在同一处理器上)，则mDeltaTime有可能会成为负值

	if(mDeltaTime < 0.0)
	{
		mDeltaTime = 0.0;
	}
}

