#include "Downloader.h"

#include <algorithm>
#include <assert.h>

#include <boost/bind.hpp>


bool Downloader::_requestExit = false;

Downloader::Downloader(size_t bufferSize, boost::function<void()> onBufferingComplete)
	:	_initialBufferSize(bufferSize),
		_onBufferingComplete(onBufferingComplete),
		_isDownloading(false),
		_downloadSpeed(0.0),
		_isBuffering(false),
		_bufferedBytesCount(0),
		_needDataSize((size_t)-1)
{
	_newDataEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	curl_global_init(CURL_GLOBAL_ALL);

	_curl_handle = curl_easy_init();

	//setup
	curl_easy_setopt(_curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	curl_easy_setopt(_curl_handle, CURLOPT_ERRORBUFFER, _errorMsg);
	curl_easy_setopt(_curl_handle, CURLOPT_FAILONERROR, 1);
#ifdef _DEBUG
	curl_easy_setopt(_curl_handle, CURLOPT_VERBOSE, 1);
#endif

	//write callback
	curl_easy_setopt(_curl_handle, CURLOPT_WRITEFUNCTION, &Downloader::curlWriteMemoryCallback);
	curl_easy_setopt(_curl_handle, CURLOPT_WRITEDATA, this);

	//progress
	curl_easy_setopt(_curl_handle, CURLOPT_PROGRESSFUNCTION, &Downloader::curl_progress);
	curl_easy_setopt(_curl_handle, CURLOPT_NOPROGRESS, 0);


	//curl_easy_setopt(_curl_handle, CURLOPT_HEADERFUNCTION, header_function);

	InitializeCriticalSection(&cs);
}

Downloader::~Downloader()
{
	stop();
	
	curl_easy_cleanup(_curl_handle);
	curl_global_cleanup();

	DeleteCriticalSection(&cs);

	CloseHandle(_newDataEvent);
}

size_t Downloader::getData( byte* buffer, size_t bufferSize )
{
	//wait data
	for( ; isDownloading() ; )
	{
		size_t availableBytesCount;
		{
			ScopedCS lock(cs);
			availableBytesCount = _downloaderBuffer.size();
		}

		bool hasAllData = availableBytesCount >= bufferSize;
		if(hasAllData)
		{
			_needDataSize = (size_t)-1;
			break;
		}
		else
		{
			_needDataSize = bufferSize;
			WaitForSingleObject(_newDataEvent, INFINITE);
		}
	}

	if(!isDownloading())
		return 0;

	{
		ScopedCS lock(cs);

		memcpy(buffer, &_downloaderBuffer[0], bufferSize);
		_downloaderBuffer.erase(_downloaderBuffer.begin(), _downloaderBuffer.begin() + bufferSize);
	}

	return bufferSize;
}

size_t Downloader::curlWriteMemoryCallback( void* ptr, size_t size, size_t nmemb, void* data )
{
	Downloader* downloader = (Downloader*)data;
	return downloader->writeMemoryCallback(ptr, size, nmemb);
}

size_t Downloader::writeMemoryCallback( void* ptr, size_t size, size_t nmemb )
{
	ScopedCS lock(cs);

	size_t realSize = size * nmemb;
	size_t currentEnd = _downloaderBuffer.size();
	_downloaderBuffer.resize(currentEnd + realSize);
	memcpy(&_downloaderBuffer[currentEnd], ptr, realSize);

	if(_bufferedBytesCount < _initialBufferSize)
	{
		_bufferedBytesCount = _downloaderBuffer.size();
		if(_bufferedBytesCount >= _initialBufferSize)
		{
			_onBufferingComplete();
			_isBuffering = false;
		}
	}

	getInfo();

	if(_downloaderBuffer.size() >= _needDataSize)
		SetEvent(_newDataEvent);

	return realSize;
}

void Downloader::download( const char* url )
{
	curl_easy_setopt(_curl_handle, CURLOPT_URL, url);

	_isDownloading = true;
	_isBuffering = true;
	_bufferedBytesCount = 0;
	
	_thread = boost::thread(boost::bind(&Downloader::download, this));
	//_thread = boost::thread( [this]{download();} );
}

void Downloader::download()
{
	CURLcode code = curl_easy_perform(_curl_handle);
	_isDownloading = false;

	printf("curl_easy_perform finished with %d - %s\n", code, getError());
}

void Downloader::stop()
{
	_requestExit = true;
	_thread.join();

	_requestExit = false;
	{
		ScopedCS lock(cs);
		_bufferedBytesCount = 0;
		_isBuffering = false;
		_downloaderBuffer.clear();
	}

	SetEvent(_newDataEvent); //wake up decoder thread - it'll check isDownloading() and exit
}

bool Downloader::isDownloading() const
{
	return _isDownloading;
}

const char* Downloader::getError() const
{
	return _errorMsg;
}

void Downloader::getInfo()
{
	curl_easy_getinfo(_curl_handle, CURLINFO_SPEED_DOWNLOAD, &_downloadSpeed);
}

double Downloader::getDownloadSpeed()
{
	ScopedCS lock(cs);
	return _downloadSpeed;
}

size_t Downloader::getBufferedBytesCount()
{
	ScopedCS lock(cs);
	return _bufferedBytesCount;
}

bool Downloader::isBuffering()
{
	return _isBuffering;
}

size_t Downloader::getInitialBufferSize() const
{
	return _initialBufferSize;
}

int Downloader::curl_progress( void *clientp, double dltotal, double dlnow, double ultotal, double ulnow )
{
	//printf("pr:total %f  now %f  total2 %d  now2 %d\n", dltotal, dlnow, ultotal, ulnow);

	if(_requestExit)
	{
		_requestExit = false;
		return 1;
	}
	
	return 0;
}
