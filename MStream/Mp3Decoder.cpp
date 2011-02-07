#include "Mp3Decoder.h"
#include "Downloader.h"
#include <assert.h>


struct Mpeg123Initializer
{
	Mpeg123Initializer()
	{
		mpg123_init();
	}
	~Mpeg123Initializer()
	{
		mpg123_exit();
	}
};


ssize_t g_read(void* iohandle, void* buffer, size_t bytes)
{
	Downloader* downloader = static_cast<Downloader*>(iohandle);
	return (ssize_t)downloader->getData((byte*)buffer, bytes);
}


Mp3Decoder::Mp3Decoder()
	:	_file(0),
		_downloader(0),
		_handle(NULL)
{
	static Mpeg123Initializer mpeg123Initializer;
}

Mp3Decoder::~Mp3Decoder()
{
	close();
}

bool Mp3Decoder::openFile( FILE*& file )
{
	_handle = mpg123_new(NULL, NULL);
	if(_handle == NULL)
	{
		printf("mpg123_new failed\n");
		return false;
	}

	_file = file;

	int res = mpg123_open_fd(_handle, _file->_file);
	if(res != MPG123_OK)
	{
		printf("mpg123_open_handle failed\n");
		return false;
	}

	return afterOpen();
}

bool Mp3Decoder::open( Downloader* downloader )
{
	_handle = mpg123_new(NULL, NULL);
	if(_handle == NULL)
	{
		printf("mpg123_new failed\n");
		return false;
	}

	mpg123_replace_reader_handle(_handle, g_read, NULL, NULL);
	int res = mpg123_param(_handle, MPG123_FLAGS, MPG123_FUZZY | MPG123_SEEKBUFFER | MPG123_GAPLESS, 0);
	assert(res == MPG123_OK);

	res = mpg123_open_handle(_handle, downloader);
	if(res != MPG123_OK)
	{
		printf("mpg123_open_handle failed\n");
		return false;
	}

	_downloader = downloader;

	return afterOpen();
}

bool Mp3Decoder::afterOpen()
{
	int encoding;
	long numberOfSamplesPerSecond;
	int res = mpg123_getformat(_handle, &numberOfSamplesPerSecond, &_numberOfChannels, &encoding);
	if(res != MPG123_OK)
	{
		printf("mpg123_getformat failed\n");
		return false;
	}
	if(encoding != MPG123_ENC_SIGNED_16)
	{
		printf("encoding != MPG123_ENC_SIGNED_16\n");
		return false;
	}

	/* Ensure that this output format will not change (it could, when we allow it). */
	res = mpg123_format_none(_handle);
	assert(res == MPG123_OK);
	res = mpg123_format(_handle, numberOfSamplesPerSecond, _numberOfChannels, encoding);
	assert(res == MPG123_OK);

	_numberOfSamplesPerSecond = numberOfSamplesPerSecond;
	_numberOfBitsPerSample = 16;

	dprintf("\nsample rate: %d\n", _numberOfSamplesPerSecond);

	return true;
}

void Mp3Decoder::close()
{
	if(_handle != NULL)
	{
		mpg123_close(_handle);
		mpg123_delete(_handle);
		_handle = NULL;
	}

	_downloader = 0;
	_file = 0;
}

long Mp3Decoder::getData( byte* buffer, size_t bufferSize )
{
	assert(_file != 0 || _downloader != 0);

	long bytes = 0;
	while(bytes != bufferSize)
	{
		size_t readBytes;
		int ret = mpg123_read(_handle, buffer + bytes, bufferSize - bytes, &readBytes);
		if(ret == MPG123_DONE)
			break;

		bytes += readBytes;
	}

	return bytes;
}

int Mp3Decoder::getNumberOfChannels()
{
	return _numberOfChannels;
}

int Mp3Decoder::getNumberOfBitsPerSample()
{
	return _numberOfBitsPerSample;
}

int Mp3Decoder::getNumberOfSamplesPerSecond()
{
	return _numberOfSamplesPerSecond;
}

int Mp3Decoder::getInstantBitRate()
{
	mpg123_info(_handle, &_frameinfo);
	return _frameinfo.bitrate * 1000;
}

void Mp3Decoder::getComments( std::vector<std::string>& out )
{

}


