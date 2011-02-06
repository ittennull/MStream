#include "stdafx.h"
#include "MusicPlayer.h"

#include <windows.h>

#include <boost/bind.hpp>

namespace Audio
{

struct StreamingVoiceContext : public IXAudio2VoiceCallback
{
	HANDLE hBufferEndEvent;

	StreamingVoiceContext() : hBufferEndEvent( CreateEvent(NULL, FALSE, FALSE, NULL) )
	{
	}
	virtual ~StreamingVoiceContext()
	{
		CloseHandle( hBufferEndEvent );
	}

	STDMETHOD_( void, OnVoiceProcessingPassStart )( UINT32 ) { }
	STDMETHOD_( void, OnVoiceProcessingPassEnd )() { }
	STDMETHOD_( void, OnStreamEnd )() { }
	STDMETHOD_( void, OnBufferStart )( void* ) { }
	STDMETHOD_( void, OnBufferEnd )( void* ) 
	{
		SetEvent( hBufferEndEvent );
	}
	STDMETHOD_( void, OnLoopEnd )( void* ) { }
	STDMETHOD_( void, OnVoiceError )( void*, HRESULT ) { }
};


MusicPlayer::MusicPlayer(IXAudio2* pXAudio2)
	:	_XAudio2(pXAudio2),
		_player(0),
		_requestStop(false),
		_bufferSize(128*1024),
		_sourceVoice(0)
{
	_voiceCallback = new StreamingVoiceContext;

	_hmodule = LoadLibrary(L"MStream.dll");
	if(_hmodule == NULL)
	{
		printf("Failed to open MStream.dll\n");
	}

	createPlayer_t createPlayer = (createPlayer_t)GetProcAddress(_hmodule, "createPlayer");
	_player = createPlayer(5);

	const int buffersCount = sizeof(_buffers) / sizeof(XAUDIO2_BUFFER);
	for(int i=0; i<buffersCount; i++)
	{
		ZeroMemory(&_buffers[i], sizeof(XAUDIO2_BUFFER));
		_buffers[i].pAudioData = new BYTE[_bufferSize];
	}

	state.playerState = Stopped;
	state.instantBitRate = 0;

	InitializeCriticalSection(&csState);
	InitializeCriticalSection(&csControl);
}

MusicPlayer::~MusicPlayer()
{
	stop();

	delete _voiceCallback;
	delete _player;

	for(int i=0; i<sizeof(_buffers)/sizeof(XAUDIO2_BUFFER); i++)
	{
		delete [] _buffers[i].pAudioData;
	}
	
	if(_hmodule != NULL)
		FreeLibrary(_hmodule);

	DeleteCriticalSection(&csState);
	DeleteCriticalSection(&csControl);
}

void MusicPlayer::play( const char* uri )
{
	stop();

	_thread = boost::thread(boost::bind(&MusicPlayer::threadProc, this, uri));
}

void MusicPlayer::stop()
{
	{
		ScopedCS lock(csControl);
		_requestStop = true;
	}

	SetEvent(static_cast<StreamingVoiceContext*>(_voiceCallback)->hBufferEndEvent);
	_thread.join();
	
	_requestStop = false;
}

float MusicPlayer::getVolume()
{
	if(_sourceVoice == 0)
		return 0.0f;

	float volume;
	_sourceVoice->GetVolume(&volume);
	return volume;
}

void MusicPlayer::setVolume( float volume )
{
	if(_sourceVoice)
		_sourceVoice->SetVolume(volume);
}

void MusicPlayer::threadProc(const char* uri)
{
	{
		ScopedCS lock(csState);
		state.playerState = Opening;
	}

	HANDLE hBufferEndEvent = static_cast<StreamingVoiceContext*>(_voiceCallback)->hBufferEndEvent;
	ResetEvent(hBufferEndEvent);

	_player->play(uri, boost::bind<BOOL>(SetEvent, hBufferEndEvent));
	WaitForSingleObject(hBufferEndEvent, INFINITE);

	{
		ScopedCS lock(csControl);
		if(_requestStop)
		{
			stopAndCleanup();
			return;
		}
	}
	if(_player->getState() == IPlayer::Stopped)
	{
		printf("Failed to open %s\n", uri);
		stopAndCleanup();
		return;
	}


	{
		ScopedCS lock(csState);
		state.playerState = Playing;
	}

	WAVEFORMATEX waveFormat;
	waveFormat.cbSize = 0;
	waveFormat.nChannels = _player->getNumberOfChannels();
	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.wBitsPerSample = _player->getNumberOfBitsPerSample();
	waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
	waveFormat.nSamplesPerSec = _player->getNumberOfSamplesPerSecond();
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;

	HRESULT hr;
	if(FAILED(hr = _XAudio2->CreateSourceVoice(&_sourceVoice, &waveFormat, 0, 1.0f, _voiceCallback)))
	{
		printf( "\nError %#X creating source voice\n", hr );
		stopAndCleanup();
		return;
	}

	const int buffersCount = sizeof(_buffers) / sizeof(XAUDIO2_BUFFER);
	for(int i=0; i<buffersCount; i++)
		_buffers[i].Flags = 0;

	int current_buffer;
	for(current_buffer=0; current_buffer<2; current_buffer++)
	{
		if(_player->getState() == IPlayer::Playing)
		{
			readBuffer(_player, _buffers[current_buffer]);
			_sourceVoice->SubmitSourceBuffer(&_buffers[current_buffer]);
		}
		else
			break;
	}

	_sourceVoice->Start( 0, 0 );

	XAUDIO2_VOICE_STATE voiseState;
	IPlayer::PlayerState playerState;
	bool needexit = false;
	for( ; ; )
	{
		playerState = _player->getState();

		
		//get state
		if(playerState == IPlayer::Playing)
		{
			ScopedCS lock(csState);
			state.instantBitRate = _player->getInstantBitRate();
			_player->getComments(state.comments);
		}


		_sourceVoice->GetState(&voiseState);
		for(; playerState == IPlayer::Stopped || voiseState.BuffersQueued >= buffersCount; _sourceVoice->GetState(&voiseState))
		{
			WaitForSingleObject(hBufferEndEvent, INFINITE);

			{
				ScopedCS lock(csControl);
				if(_requestStop)
				{
					needexit = true;
					break;
				}
			}

			if(playerState == IPlayer::Stopped)
			{
				_sourceVoice->GetState(&voiseState);
				if(voiseState.BuffersQueued == 0)
				{
					needexit = true;
					break;	//no buffers to play - exit
				}
				else
					continue; //there rest some buffers to play - continue sleep
			}
		}
		if(needexit)
			break;

		
		readBuffer(_player, _buffers[current_buffer]);
		_sourceVoice->SubmitSourceBuffer(&_buffers[current_buffer]);
		current_buffer = (current_buffer + 1) % buffersCount;
	}

	//Cleanup
	stopAndCleanup();
}

bool MusicPlayer::readBuffer( IPlayer* player, XAUDIO2_BUFFER& buffer )
{
	buffer.AudioBytes = player->getData((byte*)buffer.pAudioData, _bufferSize);
	if(buffer.AudioBytes != _bufferSize)
	{
		buffer.Flags = XAUDIO2_END_OF_STREAM;
		return false;
	}

	return true;
}

void MusicPlayer::stopAndCleanup()
{
	if(_sourceVoice != 0)
	{
		_sourceVoice->DestroyVoice();
		_sourceVoice = 0;
	}

	_player->stop();

	{
		ScopedCS lock(csState);
		state.playerState = Stopped;
		state.comments.clear();
		state.instantBitRate = 0;
	}
}

void MusicPlayer::getState( State& state )
{
	ScopedCS lock(csState);
	state = this->state;
}

int MusicPlayer::getBufferingPercentComplete()
{
	return _player->getBufferingPercentComplete();
}

}
