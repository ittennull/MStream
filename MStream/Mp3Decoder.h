#ifndef MP3_DECODER_H
#define MP3_DECODER_H

#include "IDecoder.h"
#include <mpg123.h>


class Mp3Decoder : public IDecoder
{
	mpg123_handle* _handle;
	mpg123_frameinfo _frameinfo;

	int _numberOfChannels;
	int _numberOfBitsPerSample;
	int _numberOfSamplesPerSecond;

	FILE* _file;
	Downloader* _downloader;

public:
	Mp3Decoder();
	virtual ~Mp3Decoder();

	virtual bool openFile(FILE*& file);
	virtual bool open(Downloader* downloader);

	virtual long getData(byte* buffer, size_t bufferSize);

	virtual int getNumberOfChannels();
	virtual int getNumberOfBitsPerSample();
	virtual int getNumberOfSamplesPerSecond();

	virtual int getInstantBitRate();
	virtual void getComments(std::vector<std::string>& out);

private:
	void close();
	bool afterOpen();
};

#endif
