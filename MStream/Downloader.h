#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include "base.h"

#define NOMINMAX
#include <curl/curl.h>

#include <vector>
#include <boost/function.hpp>

#define BOOST_ALL_DYN_LINK
#include <boost/thread.hpp>

struct ScopedCS
{
	CRITICAL_SECTION* p;
	ScopedCS(CRITICAL_SECTION& ref) :p(&ref) { EnterCriticalSection(p); }
	~ScopedCS() { LeaveCriticalSection(p); }
};



class Downloader
{
	CURL* _curl_handle;
	char _errorMsg[CURL_ERROR_SIZE];

	std::vector<byte> _downloaderBuffer;

	CRITICAL_SECTION cs;

	bool _isDownloading;
	double _downloadSpeed;
	size_t _bufferedBytesCount;
	bool _isBuffering;

	static bool _requestExit;

	boost::function<void()> _onBufferingComplete;
	const size_t _initialBufferSize;

	boost::thread _thread;
	HANDLE _newDataEvent;
	size_t _needDataSize;

public:
	Downloader(size_t bufferSize, boost::function<void()> onBufferingComplete);
	~Downloader();

	void download(const char* url);
	void stop();

	bool isDownloading() const;
	const char* getError() const;
	double getDownloadSpeed();
	size_t getBufferedBytesCount();
	bool isBuffering();
	size_t getInitialBufferSize() const;


	size_t getData(byte* buffer, size_t bufferSize);

private:
	static size_t curlWriteMemoryCallback(void* ptr, size_t size, size_t nmemb, void* data);
	size_t writeMemoryCallback(void* ptr, size_t size, size_t nmemb);

	static int curl_progress(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);

	void download();
	void getInfo();
};

#endif
