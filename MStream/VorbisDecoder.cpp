#include "VorbisDecoder.h"
#include "Downloader.h"


size_t fread_from_downloader(void* buffer, size_t size, size_t num, void* pdownloader)
{
	Downloader* downloader = static_cast<Downloader*>(pdownloader);
	return downloader->getData((byte*)buffer, size * num);
}


VorbisDecoder::VorbisDecoder()
	:	_file(0),
		_info(0),
		_current_section(-1),
		_downloader(0)
{
}

VorbisDecoder::~VorbisDecoder()
{
	close();
}

bool VorbisDecoder::openFile( const std::string& filename )
{
	_file = fopen(filename.c_str(), "rb");
	if(_file == 0)
	{
		printf("VorbisDecoder: Can't open file \"%s\"", filename.c_str());
		return false;
	}

	if(ov_open_callbacks(_file, &_vf, NULL, 0, OV_CALLBACKS_DEFAULT))
	{
		_file = 0;

		printf("VorbisDecoder: Failed to open file %s\n", filename.c_str());
		return false;
	}

	_info = ov_info(&_vf, -1);

	return true;
}

bool VorbisDecoder::open(Downloader* downloader)
{
	_downloader = downloader;

	ov_callbacks callbacks = 
	{
		(size_t (*)(void *, size_t, size_t, void *))  fread_from_downloader,
		(int (*)(void *, ogg_int64_t, int))           NULL,
		(int (*)(void *))                             NULL,
		(long (*)(void *))                            NULL
	};

	if(ov_open_callbacks(_downloader, &_vf, NULL, 0, callbacks))
	{
		_downloader = 0;

		return false;
	}

	_info = ov_info(&_vf, -1);

	return true;
}

long VorbisDecoder::getData( byte* buffer, size_t bufferSize )
{
	assert(_file != 0 || _downloader != 0);

	long bytes = 0;
	while(bytes != bufferSize)
	{
		long ret = ov_read(&_vf, (char*)buffer + bytes, bufferSize - bytes, 0, 2, 1, &_current_section);
		if (ret < 0) 
		{
			if(ret != OV_HOLE)
				printf("VorbisDecoder: ov_read returned %d\n", ret);
			continue;
		}
		else if(ret == 0)
		{
			break;
		}
		else
		{
			bytes += ret;
		}
	}
	
	return bytes;
}

void VorbisDecoder::close()
{
	if(_file == 0 && _downloader == 0)
		return;

	ov_clear(&_vf);
	_file = 0;
	_downloader = 0;
}

int VorbisDecoder::getNumberOfChannels()
{
	return _info->channels;
}

int VorbisDecoder::getNumberOfBitsPerSample()
{
	return 16;
}

int VorbisDecoder::getNumberOfSamplesPerSecond()
{
	return _info->rate;

}

int VorbisDecoder::getInstantBitRate()
{
	return ov_bitrate_instant(&_vf);
}

void VorbisDecoder::getComments( std::vector<std::string>& out )
{
	out.clear();

	vorbis_comment* comments = ov_comment(&_vf, -1);
	if(comments == 0)
		return;

	for(int i=0; i<comments->comments; i++)
	{
		std::string s(comments->user_comments[i], comments->comment_lengths[i]);
		out.push_back(s);
	}
}