#ifndef GAMETIMER_H
#define GAMETIMER_H


class GameTimer
{
public:
	GameTimer();

	float TotalTime()const; // ������Ϊ��λ
	float DeltaTime()const; // ������Ϊ��λ

	void Reset(); // �ڿ�ʼ��Ϣѭ��֮ǰ����
	void Start(); // �����ʱ����ͣʱ����
	void Stop();  // ��ͣ��ʱ��ʱ����
	void Tick();  // ÿ֡��Ҫ����

private:
	double mSecondsPerCount;
	double mDeltaTime;

	__int64 mBaseTime;//Ӧ�ó���Ŀ�ʼʱ��
	__int64 mPausedTime;//������ͣʱ��֮��
	__int64 mStopTime;//��ͣ��ʱ��
	__int64 mPrevTime;
	__int64 mCurrTime;

	bool mStopped;
};

#endif // GAMETIMER_H