#include "stdafx.h"
#include <windows.h>
#include <xaudio2.h>

#include "MusicPlayer.h"

#include <boost/format.hpp>

#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }


int run(int argc, _TCHAR* argv[]);



int _tmain(int argc, _TCHAR* argv[])
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF);
#endif

	std::string err;

	try
	{
		run(argc, argv);
	}
	catch (const char* e)
	{
		err = e;
	}
	catch (std::string e)
	{
		err = e;
	}
	catch(...)
	{
		err = "Unknown exception";
	}

	if(!err.empty())
	{
		cout<<err<<endl;
		//cin>>argc;
	}

	return 0;
}

int run(int argc, _TCHAR* argv[])
{
	HRESULT hr;

	//
	// Initialize XAudio2
	//
	CoInitializeEx( NULL, COINIT_MULTITHREADED );

	IXAudio2* pXAudio2 = NULL;

	UINT32 flags = 0;
#ifdef _DEBUG
	flags |= XAUDIO2_DEBUG_ENGINE;
#endif

	if( FAILED( hr = XAudio2Create( &pXAudio2, flags ) ) )
	{
		wprintf( L"Failed to init XAudio2 engine: %#X\n", hr );
		CoUninitialize();
		return 0;
	}

	//
	// Create a mastering voice
	//
	IXAudio2MasteringVoice* pMasteringVoice = NULL;

	if( FAILED( hr = pXAudio2->CreateMasteringVoice( &pMasteringVoice ) ) )
	{
		wprintf( L"Failed creating mastering voice: %#X\n", hr );
		SAFE_RELEASE( pXAudio2 );
		CoUninitialize();
		return 0;
	}

	Audio::IMusicPlayer* player = new Audio::MusicPlayer(pXAudio2);

	std::vector<std::string> playlist;
	//playlist.push_back("http://192.168.1.128:8000/example1.ogg");
	playlist.push_back("http://animeradio.su:8000/");
	playlist.push_back("http://82.199.118.10:8000/radionami");
	playlist.push_back("1.mp3");
	playlist.push_back("2.flac");
	playlist.push_back("http://radio.cesnet.cz:8000/cro-d-dur.flac");
	playlist.push_back("http://acrobs.net:8000/acrobs.ogg");
	
	playlist.push_back("1.ogg");
	
	//playlist.push_back("d:\\Docs\\music\\collapse_music\\04 NewTone - Run!!!!.ogg");
	playlist.push_back("http://rainwave.cc:8000/rainwave.ogg");

	//player->play("c:\\Users\\Dima\\Documents\\collapse_music\\04 NewTone - Run!!!!.ogg");
	
	//player->play("d:\\Docs\\music\\collapse_music\\04 NewTone - Run!!!!.ogg");

	player->play(playlist[0].c_str());

	Audio::IMusicPlayer::State state;
	std::vector<std::string> comments;

	float volume = 1.0f;
	player->setVolume(volume);

	auto playN = [=] (size_t n)
	{
		if(n < playlist.size())
		{
			printf("\n---%d---\n", n);
			player->play(playlist[n].c_str());
			player->setVolume(volume);
		}
	};


	for(size_t i=0; ; ++i)
	{
		if( GetAsyncKeyState( VK_ESCAPE ) )
		{
			break;
		}

		if( GetAsyncKeyState( VK_NUMPAD0 ) )
		{
			player->stop();
			printf("Stopped");
		}

		if( GetAsyncKeyState( VK_NUMPAD1 ) ) playN(0);
		if( GetAsyncKeyState( VK_NUMPAD2 ) ) playN(1);
		if( GetAsyncKeyState( VK_NUMPAD3 ) ) playN(2);
		if( GetAsyncKeyState( VK_NUMPAD4 ) ) playN(3);
		if( GetAsyncKeyState( VK_NUMPAD5 ) ) playN(4);
		if( GetAsyncKeyState( VK_NUMPAD6 ) ) playN(5);
		
		if( GetAsyncKeyState( VK_ADD ) )
		{
			volume = min(1, volume+0.1f);
			player->setVolume(volume);
		}
		if( GetAsyncKeyState( VK_SUBTRACT ) )
		{
			volume = max(0, volume-0.1f);
			player->setVolume(volume);
		}

		player->getState(state);
		if(comments != state.comments)
		{
			comments = state.comments;

			printf("\n\n");
			for(size_t j=0; j<comments.size(); j++)
			{
				size_t k = comments[j].find('=');
				std::string name = comments[j].substr(0, k);
				std::string value = comments[j].substr(k + 1);

				printf("%s: %s\n", name.c_str(), value.c_str());
			}
		}

		std::string bitrate = str(boost::format("%1% kbs") %(state.instantBitRate / 1000));
		printf("\r%s", bitrate.c_str());


		Sleep(300);
	}


	delete player;
	
	pMasteringVoice->DestroyVoice();
	pXAudio2->StopEngine();
	pXAudio2->Release();

	return 0;
}
