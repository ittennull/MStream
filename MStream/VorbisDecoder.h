#ifndef VORBIS_DECODER_H
#define VORBIS_DECODER_H

#include "IDecoder.h"
#include <stdio.h>

#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"


class VorbisDecoder : public IDecoder
{
	OggVorbis_File _vf;
	vorbis_info* _info;
	FILE* _file;
	int _current_section;

	Downloader* _downloader;

public:
	VorbisDecoder();
	virtual ~VorbisDecoder();

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
};

#endif
