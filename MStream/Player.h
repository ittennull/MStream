#ifndef PLAYER_H
#define PLAYER_H

#include "IPlayer.h"
#include "IDecoder.h"
#include "Downloader.h"


class Player : public IPlayer
{
	IDecoder* _decoder;
	Downloader* _downloader;
	PlayerState _playerState;

	const size_t _downloaderBufferSize;

	boost::thread _openThread;

	CRITICAL_SECTION cs;

	FILE* _currentFile;

public:
	Player(size_t downloaderBufferSizeKB);
	~Player();

	virtual void play(const char* uri);
	virtual void stop();
	virtual size_t getData(byte* buffer, size_t bufferSize);

	virtual PlayerState getState();
	virtual int getBufferingPercentComplete();

	virtual int getNumberOfChannels();
	virtual int getNumberOfBitsPerSample();
	virtual int getNumberOfSamplesPerSecond();

	virtual int getInstantBitRate();
	virtual void getComments(std::vector<std::string>& out);

private:
	void onBufferingComplete();
};

#endif

