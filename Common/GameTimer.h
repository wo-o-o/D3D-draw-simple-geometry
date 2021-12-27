#ifndef GAMETIMER_H
#define GAMETIMER_H


class GameTimer
{
public:
	GameTimer();

	float TotalTime()const; // 用秒作为单位
	float DeltaTime()const; // 用秒作为单位

	void Reset(); // 在开始消息循环之前调用
	void Start(); // 解除计时器暂停时调用
	void Stop();  // 暂停计时器时调用
	void Tick();  // 每帧都要调用

private:
	double mSecondsPerCount;
	double mDeltaTime;

	__int64 mBaseTime;//应用程序的开始时刻
	__int64 mPausedTime;//所有暂停时间之和
	__int64 mStopTime;//暂停的时刻
	__int64 mPrevTime;
	__int64 mCurrTime;

	bool mStopped;
};

#endif // GAMETIMER_H