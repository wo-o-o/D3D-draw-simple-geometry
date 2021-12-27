#include <windows.h>
#include "GameTimer.h"

GameTimer::GameTimer(): mSecondsPerCount(0.0), mDeltaTime(-1.0), mBaseTime(0), mPausedTime(0), mPrevTime(0), mCurrTime(0), mStopped(false)
{
	//Ϊ�˾�ȷ�ض���ʱ�䣬���ǽ��������ܼ�ʱ��( performance timer��������ܼ�������performance counter)��
	//���ϣ�����ò�ѯ���ܼ�ʱ���� Win32����,���Ǳ�������ͷ�ļ�#include<windows.h>��

	//��QueryPerformanceFrequency��������ȡ���ܼ�ʱ����Ƶ�ʣ���λ:����/��)
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);

	//ÿ���������������������Ƽ���֮һ��)����Ϊ�������ܼ�ʱ��Ƶ�ʵĵ���
	mSecondsPerCount = 1.0 / (double)countsPerSec;
}

//�����Ե��� Reset() ������������ʱ�䣬
//�������κ�ʱ��ʱ��ֹͣ��ʱ�䡣
float GameTimer::TotalTime()const
{
	//���������ֹͣ״̬������Ա���ֹͣʱ������ǰʱ�̵����ʱ�䡣���⣬���֮ǰ���й���ͣ�������
	//��ôҲ��Ӧͳ��mStopTime - mBaseTime���ʱ���ڵ���ͣʱ��
    //Ϊ��������һ�㣬���Դ�mStopTime ���ټ�ȥ��ͣʱ��mPausedTime

	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mStopTime    mCurrTime

	if( mStopped )
	{
		return (float)(((mStopTime - mPausedTime)-mBaseTime)*mSecondsPerCount);
	}

	//���ǲ���ϣ��ͳ��mCurrTime - mBaseTime�ڵ���ͣʱ��
	//����ͨ����mcurrTime ���ټ�ȥ��ͣʱ��mPausedTime��ʵ����һ��
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


	// �ۼӵ���stop��start��Է���֮�����ͣʱ�̼��
	//
	//                     |<-------d------->|
	// ----*---------------*-----------------*------------> time
	//  mBaseTime       mStopTime        startTime     

	//�����ֹͣ״̬������ʱ�Ļ���
	if( mStopped )
	{
		//�ۼ���ͣʱ��
		mPausedTime += (startTime - mStopTime);	

		//�����¿�����ʱ��ʱ��ǰһ֡��ʱ��mPrevTime����Ч�ģ�������Ϊ���洢������ͣʱǰһ
		//֡�Ŀ�ʼʱ�̣������Ҫ��������Ϊ��ǰʱ��
		mPrevTime = startTime;

		//�Ѿ�����ֹͣ״̬
		mStopTime = 0;
		mStopped  = false;
	}
}

void GameTimer::Stop()
{
	//����Ѿ�����ֹͣ״̬���Ǿ�ʲôҲ����
	if( !mStopped )
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

		//���򣬱���ֹͣ��ʱ�̣������ò�����־, ָʾ��ʱ���Ѿ�ֹͣ
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

	//��ñ�֡��ʼ��ʾ��ʱ��
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	mCurrTime = currTime;

	// ��֡��ǰһ֡��ʱ���
	mDeltaTime = (mCurrTime - mPrevTime)*mSecondsPerCount;

	// ׼�����㱾֡����һ֡��ʱ���
	mPrevTime = mCurrTime;

	//ʹʱ���Ϊ�Ǹ�ֵ��DXSDK �е�CDXUTTimerʾ��ע�����ᵽ:������������ڽ���ģʽ��������
	//������֡��ʱ���Ĺ������л�����һ��������ʱ(��QueryPerformanceCounter���������ε�
	//�ò�����ͬһ��������)����mDeltaTime�п��ܻ��Ϊ��ֵ

	if(mDeltaTime < 0.0)
	{
		mDeltaTime = 0.0;
	}
}

