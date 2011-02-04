#include "FlacDecoder.h"
#include "Downloader.h"


FLAC__StreamDecoderWriteStatus g_write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data)
{
	return static_cast<FLACDecoder*>(client_data)->write_callback(decoder, frame, buffer);
}
void g_metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	static_cast<FLACDecoder*>(client_data)->metadata_callback(decoder, metadata);
}
void g_error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	static_cast<FLACDecoder*>(client_data)->error_callback(decoder, status);
}
//FLAC__StreamDecoderReadStatus g_read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
//{
//	if(*bytes > 0) 
//	{
//		Downloader* downloader = static_cast<Downloader*>(client_data);
//		*bytes = downloader->getData((byte*)buffer, *bytes);
//		if(*bytes == 0)
//			return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
//		else
//			return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
//	}
//	else
//		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
//}


FLACDecoder::FLACDecoder()
	:	_file(0),
		_downloader(0),
		_streamDecoder(0),
		_numberOfChannels(0), _numberOfBitsPerSample(0), _numberOfSamplesPerSecond(0)
{
}

FLACDecoder::~FLACDecoder()
{
	close();
}

bool FLACDecoder::openFile( FILE*& file )
{
	_streamDecoder = FLAC__stream_decoder_new();
	if(_streamDecoder == 0)
	{
		printf("FLAC__stream_decoder_new failed\n");
		return false;
	}

	_file = &file;
	
	FLAC__StreamDecoderInitStatus status = FLAC__stream_decoder_init_FILE(_streamDecoder, *_file, 	
		g_write_callback, g_metadata_callback, g_error_callback, this);
	if(status != FLAC__STREAM_DECODER_INIT_STATUS_OK)
	{
		close();
		printf("FLAC__stream_decoder_init_FILE failed\n");
		return false;
	}

	FLAC__bool res = FLAC__stream_decoder_process_until_end_of_metadata(_streamDecoder);
	if(!res)
	{
		close();
		printf("FLAC__stream_decoder_process_until_end_of_metadata failed\n");
		return false;
	}

	return true;
}

bool FLACDecoder::open( Downloader* downloader )
{
	return false;

	/*_streamDecoder = FLAC__stream_decoder_new();
	if(_streamDecoder == 0)
	{
		printf("FLAC__stream_decoder_new failed\n");
		return false;
	}

	FLAC__StreamDecoderInitStatus status = FLAC__stream_decoder_init_stream(_streamDecoder, 
		g_read_callback, NULL, NULL, NULL, NULL, g_write_callback, g_metadata_callback, g_error_callback, downloader);
	if(status != FLAC__STREAM_DECODER_INIT_STATUS_OK)
	{
		close();
		printf("FLAC__stream_decoder_init_stream failed");
		return false;
	}

	FLAC__bool res = FLAC__stream_decoder_process_until_end_of_metadata(_streamDecoder);
	if(!res)
	{
		close();
		printf("FLAC__stream_decoder_process_until_end_of_metadata failed");
		return false;
	}

	return true;*/
}

void FLACDecoder::close()
{
	if(_streamDecoder != 0)
	{
		FLAC__stream_decoder_finish(_streamDecoder);
		FLAC__stream_decoder_delete(_streamDecoder);
		_streamDecoder = 0;
	}

	if(_file != 0)
	{
		*_file = 0; //FLAC__stream_decoder_finish have closed file already but we must notify Player about that
		_file = 0;
	}

	_downloader = 0;
	_data.clear();
}

long FLACDecoder::getData( byte* buffer, size_t bufferSize )
{
	_buffer = buffer;
	_bufferPosition = 0;
	_bufferSize = bufferSize;

	if(!_data.empty())
	{
		size_t sizeToCopy = std::min(_data.size(), bufferSize);
		memcpy(buffer, &_data[0], sizeToCopy);
		_bufferPosition += sizeToCopy;
		_data.erase(_data.begin(), _data.begin() + sizeToCopy);

		if(_bufferPosition == bufferSize)
			return _bufferPosition;
	}

	FLAC__bool res = FLAC__stream_decoder_process_single(_streamDecoder);
	if(res)
	{
		return _bufferPosition;
	}
	else
	{
		FLAC__StreamDecoderState state = FLAC__stream_decoder_get_state(_streamDecoder);
		printf("FLAC__StreamDecoderState after FLAC__stream_decoder_process_single is %d - \"%s\"", state, FLAC__StreamDecoderStateString[state]);
		return -1;
	}
}

int FLACDecoder::getNumberOfChannels()
{
	return _numberOfChannels;
}

int FLACDecoder::getNumberOfBitsPerSample()
{
	return _numberOfBitsPerSample;
}

int FLACDecoder::getNumberOfSamplesPerSecond()
{
	return _numberOfSamplesPerSecond;
}

int FLACDecoder::getInstantBitRate()
{
	return 0;
}

void FLACDecoder::getComments( std::vector<std::string>& out )
{

}

FLAC__StreamDecoderWriteStatus FLACDecoder::write_callback( const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[] )
{
	size_t channelSampleBytesCount = _numberOfBitsPerSample / 8;

	size_t blockSize = std::min(frame->header.blocksize, (_bufferSize - _bufferPosition) / (_numberOfChannels * channelSampleBytesCount));

	for(unsigned int i=0; i<blockSize; i++) 
	{
		for(int j=0; j<_numberOfChannels; j++)
		{
			memcpy(_buffer + _bufferPosition, &buffer[j][i], channelSampleBytesCount);
			_bufferPosition += channelSampleBytesCount;
		}
	}

	if(frame->header.blocksize - blockSize > 0)
	{
		_data.resize((frame->header.blocksize - blockSize) * (_numberOfChannels * channelSampleBytesCount));
		size_t dataPos = 0;
		
		for(unsigned int i=blockSize; i<frame->header.blocksize; i++) 
		{
			for(int j=0; j<_numberOfChannels; j++)
			{
				memcpy(&_data[dataPos], &buffer[j][i], channelSampleBytesCount);
				dataPos += channelSampleBytesCount;
			}
		}
	}

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void FLACDecoder::metadata_callback( const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata )
{
	if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
	{
		_numberOfChannels = metadata->data.stream_info.channels;
		_numberOfBitsPerSample = metadata->data.stream_info.bits_per_sample;
		_numberOfSamplesPerSecond = metadata->data.stream_info.sample_rate;
	}
}

void FLACDecoder::error_callback( const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status )
{
	printf("FLACDecoder error: %d - %s\n", status, FLAC__StreamDecoderErrorStatusString[status]);
}