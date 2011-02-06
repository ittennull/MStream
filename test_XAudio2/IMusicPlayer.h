#pragma once
#include <vector>
#include <string>

namespace Audio
{

class IMusicPlayer
{
public:
	enum EPlayerState
	{
		Opening,
		Playing,
		Stopped
	};

	struct State
	{
		EPlayerState playerState;
		int instantBitRate;
		std::vector<std::string> comments;
	};

public:
	virtual ~IMusicPlayer() {}

	virtual void play(const char* uri) = 0;
	virtual void stop() = 0;

	virtual float getVolume() = 0;
	virtual void setVolume(float volume) = 0;

	virtual int getBufferingPercentComplete() = 0;
	virtual void getState(State& state) = 0;
};

}
