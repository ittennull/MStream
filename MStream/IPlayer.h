#ifndef IPLAYER_H
#define IPLAYER_H

#include <string>
#include <vector>
#include <boost/function.hpp>


class IPlayer
{
public:
	enum PlayerState
	{
		Opening,
		Playing,
		Stopped
	};

public:
	virtual ~IPlayer() {}

	virtual void play(const char* uri, boost::function<void()> onOpened) = 0;
	virtual void stop() = 0;
	virtual size_t getData(unsigned char* buffer, size_t bufferSize) = 0;

	virtual PlayerState getState() = 0;
	virtual int getBufferingPercentComplete() = 0;

	virtual int getNumberOfChannels() = 0;
	virtual int getNumberOfBitsPerSample() = 0;
	virtual int getNumberOfSamplesPerSecond() = 0;

	virtual int getInstantBitRate() = 0;
	virtual void getComments(std::vector<std::string>& out) = 0;
};


typedef IPlayer*(*createPlayer_t)(size_t);
extern "C" __declspec(dllexport) IPlayer* createPlayer(size_t downloaderBufferSizeKB);


#endif
