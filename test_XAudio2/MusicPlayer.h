#pragma once
#include "..\MStream\IPlayer.h"
#include "IMusicPlayer.h"
#include <xaudio2.h>

#define BOOST_ALL_DYN_LINK
#include <boost/thread.hpp>

namespace Audio
{

struct ScopedCS
{
	CRITICAL_SECTION* p;
	ScopedCS(CRITICAL_SECTION& ref) :p(&ref) { EnterCriticalSection(p); }
	~ScopedCS() { LeaveCriticalSection(p); }
};

struct StreamingVoiceContext;

class MusicPlayer : public IMusicPlayer
{
	HMODULE _hmodule;
	IPlayer* _player;
	IXAudio2* _XAudio2;
	IXAudio2SourceVoice* _sourceVoice;
	StreamingVoiceContext* _voiceCallback;
	XAUDIO2_BUFFER _buffers[3];
	boost::thread _thread;
	bool _requestStop;
	CRITICAL_SECTION csState;
	CRITICAL_SECTION csControl;
	const size_t _bufferSize;
	float _volume;

	State state;
	
public:
	MusicPlayer(IXAudio2* pXAudio2, size_t downloaderBufferSizeKB=5);
	~MusicPlayer();

	virtual void play(const char* uri);
	virtual void stop();

	virtual float getVolume();
	virtual void setVolume(float volume);

	virtual int getBufferingPercentComplete();
	virtual void getState(State& state);

	virtual IXAudio2SourceVoice* getSourceVoice();

private:
	void threadProc(const char* uri);
	bool readBuffer(IPlayer* player, XAUDIO2_BUFFER& buffer);
	void stopAndCleanup();
};

}
