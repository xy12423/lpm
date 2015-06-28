#include "stdafx.h"
#include "download.h"

double sizeAll = 0, sizeDownloaded = 0;
size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
	dataBuf *myBuf = static_cast<dataBuf*>(userp);
	char *dataBuf = static_cast<char*>(buffer);
	size_t sizePart = size * nmemb;
	for (size_t i = 0; i < sizePart; i++)
	{
		myBuf->push_back(*dataBuf);
		dataBuf++;
	}
	sizeDownloaded += sizePart;
	if (prCallbackP != NULL)
		(*prCallbackP)(sizeDownloaded * 100 / sizeAll);
	return sizePart;
}

size_t write_data_to_file(void *buffer, size_t size, size_t nmemb, void *userp)
{
	std::ofstream *fs = static_cast<std::ofstream*>(userp);
	char *dataBuf = static_cast<char*>(buffer);
	size_t sizePart = size * nmemb;
	fs->write(dataBuf, sizePart);
	sizeDownloaded += sizePart;
	if (prCallbackP != NULL)
		(*prCallbackP)(sizeDownloaded * 100 / sizeAll);
	return sizePart;
}

size_t save_header(void *ptr, size_t size, size_t nmemb, void *data)
{
	return size * nmemb;
}

errInfo download(const std::string &add, dataBuf *buf)
{
	std::vector<char> errBuf;
	errBuf.reserve(2048);
	CURL *handle = curl_easy_init();
	char *addCStr = str2cstr(add);
	curl_easy_setopt(handle, CURLOPT_URL, addCStr);
	curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, errBuf.data());
	CURLcode success;
	infoStream << "I:Connecting" << std::endl;
	if (prCallbackP != NULL)
	{
		curl_easy_setopt(handle, CURLOPT_HEADER, 1);
		curl_easy_setopt(handle, CURLOPT_NOBODY, 1);
		curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, save_header);
		success = curl_easy_perform(handle);
		if (success != CURLcode::CURLE_OK)
		{
			curl_easy_cleanup(handle);
			return errInfo(std::string("E:network:") + errBuf.data());
		}
		success = curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &sizeAll);
		if (success != CURLcode::CURLE_OK)
		{
			curl_easy_cleanup(handle);
			return errInfo(std::string("E:network:") + errBuf.data());
		}
		curl_easy_setopt(handle, CURLOPT_HEADER, 0);
		curl_easy_setopt(handle, CURLOPT_NOBODY, 0);
		(*prCallbackP)(0);
	}

	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, buf);
	infoStream << "I:Downloading data" << std::endl;
	sizeDownloaded = 0;
	success = curl_easy_perform(handle);
	if (success != CURLcode::CURLE_OK)
	{
		curl_easy_cleanup(handle);
		return errInfo(std::string("E:network:") + errBuf.data());
	}
	if (prCallbackP != NULL)
		(*prCallbackP)(100);
	curl_easy_cleanup(handle);
	
	infoStream << "I:Data downloaded" << std::endl;
	return errInfo();
}

errInfo download(const std::string &add, std::string path)
{
	std::vector<char> errBuf;
	errBuf.reserve(2048);
	CURL *handle = curl_easy_init();
	char *addCStr = str2cstr(add);
	curl_easy_setopt(handle, CURLOPT_URL, addCStr);
	curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, errBuf.data());
	CURLcode success;
	infoStream << "I:Connecting" << std::endl;
	if (prCallbackP != NULL)
	{
		curl_easy_setopt(handle, CURLOPT_HEADER, 1);
		curl_easy_setopt(handle, CURLOPT_NOBODY, 1);
		curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, save_header);
		success = curl_easy_perform(handle);
		if (success != CURLcode::CURLE_OK)
		{
			curl_easy_cleanup(handle);
			return errInfo(std::string("E:network:") + errBuf.data());
		}
		success = curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &sizeAll);
		if (success != CURLcode::CURLE_OK)
		{
			curl_easy_cleanup(handle);
			return errInfo(std::string("E:network:") + errBuf.data());
		}
		curl_easy_setopt(handle, CURLOPT_HEADER, 0);
		curl_easy_setopt(handle, CURLOPT_NOBODY, 0);
		(*prCallbackP)(0);
	}

	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data_to_file);
	std::ofstream fout(path, std::ios_base::out | std::ios_base::binary);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &fout);
	infoStream << "I:Downloading data" << std::endl;
	sizeDownloaded = 0;
	success = curl_easy_perform(handle);
	if (success != CURLcode::CURLE_OK)
	{
		curl_easy_cleanup(handle);
		return errInfo(std::string("E:network:") + errBuf.data());
	}
	if (prCallbackP != NULL)
		(*prCallbackP)(100);
	curl_easy_cleanup(handle);

	infoStream << "I:Data downloaded" << std::endl;
	return errInfo();
}
