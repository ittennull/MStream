#ifndef IDECODER_H
#define IDECODER_H

#include "base.h"
#include <string>
#include <vector>

class Downloader;

class IDecoder
{
public:
	virtual ~IDecoder() {}

	virtual bool openFile(const std::string& filename) = 0;
	virtual bool open(Downloader* downloader) = 0;

	virtual long getData(byte* buffer, size_t bufferSize) = 0;

	virtual int getNumberOfChannels() = 0;
	virtual int getNumberOfBitsPerSample() = 0;
	virtual int getNumberOfSamplesPerSecond() = 0;

	virtual int getInstantBitRate() = 0;
	virtual void getComments(std::vector<std::string>& out) = 0;
};

#endif
