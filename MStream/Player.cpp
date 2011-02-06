#include "Player.h"
#include "VorbisDecoder.h"
#include "FlacDecoder.h"
#include "Mp3Decoder.h"
#include <boost/bind.hpp>
#include <boost/algorithm/string/case_conv.hpp>


IPlayer* createPlayer(size_t downloaderBufferSizeKB)
{
	Player* r = new Player(downloaderBufferSizeKB);
	return r;
}


Player::Player(size_t downloaderBufferSizeKB)
	:	_downloaderBufferSize(std::max((size_t)1, downloaderBufferSizeKB) * 1024),
		_decoder(0),
		_downloader(0),
		_playerState(Stopped),
		_currentFile(0)
{
	InitializeCriticalSection(&cs);
}

Player::~Player()
{
	stop();

	delete _downloader;

	DeleteCriticalSection(&cs);
}


void Player::play( const char* uri )
{
	stop();
	
	std::string uriString = uri;
	bool isNetStream = (uriString.compare(0, 4, "http") == 0);

	std::string ext;
	size_t pointIndex = uriString.rfind('.');
	if(pointIndex != std::string::npos)
	{
		ext = uriString.substr(pointIndex + 1);
		boost::to_lower(ext);
	}

	if(ext == "ogg")
	{
		_decoder = new VorbisDecoder();
	}
	else if(ext == "flac")
	{
		_decoder = new FLACDecoder();
	}
	else if(ext == "mp3" || isNetStream)
	{
		_decoder = new Mp3Decoder();
	}
	else
	{
		printf("Bad uri \"%s\"\n", uriString.c_str());
		return;
	}

	printf("Opening %s\n", uri);
	
	if(isNetStream)
	{
		if(_downloader == 0)
			_downloader = new Downloader(_downloaderBufferSize, boost::bind(&Player::onBufferingComplete, this));

		_playerState = Opening;
		_downloader->download(uri);
	}
	else
	{
		_currentFile = fopen(uri, "rb");
		if(_currentFile == 0)
		{
			printf("Player: Can't open file \"%s\"", uri);
			stop();
			return;
		}
		
		if(_decoder->openFile(_currentFile))
			_playerState = Playing;
		else
			stop();
	}
}

void Player::onBufferingComplete()
{
	_openThread = boost::thread( 
		[this]
		{
			bool ok = _decoder->open(_downloader);
			if(ok)
			{
				ScopedCS lock(cs);
				_playerState = Playing;
			}
			else
			{
				_openThread.detach();
				stop();
			}
		}
	);
}

void Player::stop()
{
	{
		ScopedCS lock(cs);
		if(_playerState == Stopped)
			return;
		_playerState = Stopped;
	}

	if(_downloader != 0)
	{
		_downloader->stop();
	}

	_openThread.join();

	if(_decoder != 0)
	{
		delete _decoder;
		_decoder = 0;
	}

	if(_currentFile != 0)
	{
		fclose(_currentFile);
		_currentFile = 0;
	}
}

size_t Player::getData( byte* buffer, size_t bufferSize )
{
	assert(_decoder != 0);

	long ret = _decoder->getData(buffer, bufferSize);
	if (ret <= 0) 
	{
		stop();
	}

	return ret;
}

int Player::getNumberOfChannels()
{
	return _decoder->getNumberOfChannels();
}

int Player::getNumberOfBitsPerSample()
{
	return _decoder->getNumberOfBitsPerSample();
}

int Player::getNumberOfSamplesPerSecond()
{
	return _decoder->getNumberOfSamplesPerSecond();
}

int Player::getInstantBitRate()
{
	return _decoder->getInstantBitRate();
}

IPlayer::PlayerState Player::getState()
{
	ScopedCS lock(cs);
	return _playerState;
}

int Player::getBufferingPercentComplete()
{
	if(_downloader == 0)
		return 0;

	size_t bufferedBytesCount = std::min(_downloader->getBufferedBytesCount(), _downloader->getInitialBufferSize());
	return (int)(100.0f * (float)bufferedBytesCount / _downloader->getInitialBufferSize());
}

void Player::getComments( std::vector<std::string>& out )
{
	_decoder->getComments(out);
}