#ifndef FLAC_DECODER_H
#define FLAC_DECODER_H

#include "IDecoder.h"
#include <stdio.h>

#include <FLAC/all.h>


class FLACDecoder : public IDecoder
{
	FILE** _file;

	FLAC__StreamDecoder* _streamDecoder;
	int _numberOfChannels;
	int _numberOfBitsPerSample;
	int _numberOfSamplesPerSecond;

	byte* _buffer;
	size_t _bufferPosition;
	size_t _bufferSize;

	std::vector<byte> _data;

	Downloader* _downloader;

	std::vector<std::string> comments;

public:
	FLACDecoder();
	virtual ~FLACDecoder();

	virtual bool openFile(FILE*& file);
	virtual bool open(Downloader* downloader);

	virtual long getData(byte* buffer, size_t bufferSize);

	virtual int getNumberOfChannels();
	virtual int getNumberOfBitsPerSample();
	virtual int getNumberOfSamplesPerSecond();

	virtual int getInstantBitRate();
	virtual void getComments(std::vector<std::string>& out);


	FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[]);
	void metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata);
	void error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status);
	FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes);

private:
	void close();
};

#endif
