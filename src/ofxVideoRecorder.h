#pragma once

#include "ofMain.h"
#include "Poco/Condition.h"
#include <set>

#ifdef TARGET_WIN32
#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#endif

template <typename T>
struct lockFreeQueue {
	lockFreeQueue() {
		list.push_back(T());
		iHead = list.begin();
		iTail = list.end();
	}
	void Produce(const T& t) {
		list.push_back(t);
		iTail = list.end();
		list.erase(list.begin(), iHead);
	}
	bool Consume(T& t) {
		typename TList::iterator iNext = iHead;
		++iNext;
		if (iNext != iTail)
		{
			iHead = iNext;
			t = *iHead;
			return true;
		}
		return false;
	}
	int size() { return distance(iHead, iTail) - 1; }
	typename std::list<T>::iterator getHead() { return iHead; }
	typename std::list<T>::iterator getTail() { return iTail; }


private:
	typedef std::list<T> TList;
	TList list;
	typename TList::iterator iHead, iTail;
};

class execThread : public ofThread {
public:
	execThread();
	void setup(string command);
	void threadedFunction();
private:
	string execCommand;
};

struct audioFrameShort {
	short * data;
	int size;
};

class ofxVideoDataWriterThread : public ofThread {
public:
	ofxVideoDataWriterThread();
#if defined( TARGET_OSX ) || defined( TARGET_LINUX )
	void setup(string filePath, lockFreeQueue<ofPixels *> * q);
#endif
#ifdef TARGET_WIN32
	void setup(HANDLE pipeHandle, lockFreeQueue<ofPixels *> * q);
	HANDLE videoHandle;
	HANDLE fileHandle;
#endif
	void threadedFunction();
	void signal();
	void setPipeNonBlocking();
	bool isWriting() { return bIsWriting; }
	void close() { bClose = true; stopThread(); signal(); }
private:
	ofMutex conditionMutex;
	Poco::Condition condition;
	string filePath;
	int fd;
	lockFreeQueue<ofPixels *> * queue;
	bool bIsWriting;
	bool bClose;
};

class ofxAudioDataWriterThread : public ofThread {
public:
	ofxAudioDataWriterThread();
#if defined( TARGET_OSX ) || defined( TARGET_LINUX )
	void setup(string filePath, lockFreeQueue<audioFrameShort *> * q);
#endif
#ifdef TARGET_WIN32
	void setup(HANDLE pipeHandle, lockFreeQueue<audioFrameShort *> * q);
	HANDLE audioHandle;
	HANDLE fileHandle;
#endif
	void threadedFunction();
	void signal();
	void setPipeNonBlocking();
	bool isWriting() { return bIsWriting; }
	void close() { bClose = true; stopThread(); signal(); }
private:
	ofMutex conditionMutex;
	Poco::Condition condition;
	string filePath;
	int fd;
	lockFreeQueue<audioFrameShort *> * queue;
	bool bIsWriting;
	bool bClose;
};

class ofxVideoRecorder
{
public:
	ofxVideoRecorder();
	bool setup(string fname, int w, int h, float fps, int sampleRate = 0, int channels = 0, bool sysClockSync = false, bool silent = false);
	bool setupCustomOutput(int w, int h, float fps, string outputString, bool sysClockSync = false, bool silent = false);
	bool setupCustomOutput(int w, int h, float fps, int sampleRate, int channels, string outputString, bool sysClockSync = false, bool silent = false);
	bool runCustomScript(string script);
	bool addFrame(const ofPixels &pixels);
	void addAudioSamples(float * samples, int bufferSize, int numChannels);

	void start();
	void close();
	void setPaused(bool bPause);

	void setFfmpegLocation(string loc) { ffmpegLocation = loc; }
	void setMovFileExtension(string extension) { movFileExt = extension; }
	void setAudioFileExtension(string extension) { audioFileExt = extension; }
	void setVideoCodec(string codec) { videoCodec = codec; }
	void setVideoBitrate(string bitrate) { videoBitrate = bitrate; }

	void setPixelFormat(string pixelF) { //rgb24 || gray, default is rgb24
		pixelFormat = pixelF;
	};

	unsigned long long getNumVideoFramesRecorded() { return videoFramesRecorded; }
	unsigned long long getNumAudioSamplesRecorded() { return audioSamplesRecorded; }

	int getVideoQueueSize() { return frames.size(); }
	int getAudioQueueSize() { return audioFrames.size(); }

	bool isInitialized() { return bIsInitialized; }
	bool isRecording() { return bIsRecording; };
	bool isPaused() { return bIsPaused; };
	bool isSyncAgainstSysClock() { return bSysClockSync; };

	string getMoviePath() { return filePath; }
	int getWidth() { return width; }
	int getHeight() { return height; }

private:

	std::wstring convertNarrowToWide(const std::string& as) {
		if (as.empty())    return std::wstring();
		size_t reqLength = ::MultiByteToWideChar(CP_UTF8, 0, as.c_str(), (int)as.length(), 0, 0);
		std::wstring ret(reqLength, L'\0');
		::MultiByteToWideChar(CP_UTF8, 0, as.c_str(), (int)as.length(), &ret[0], (int)ret.length());
		return ret;
	}

	wchar_t *convertCharArrayToLPCWSTR(const char* charArray)
	{
		wchar_t* wString = new wchar_t[4096];
		MultiByteToWideChar(CP_ACP, 0, charArray, -1, wString, 4096);
		return wString;
	}

	std::string convertWideToNarrow(const wchar_t *s, char dfault = '?',
		const std::locale& loc = std::locale())
	{
		std::ostringstream stm;

		while (*s != L'\0') {
			stm << std::use_facet< std::ctype<wchar_t> >(loc).narrow(*s++, dfault);
		}
		return stm.str();
	}

	string filePath;
	string fileName;
	string movFileExt;
	string audioFileExt;
	string videoPipePath, audioPipePath;
	string ffmpegLocation;
	string videoCodec, audioCodec, videoBitrate, audioBitrate, pixelFormat;
	int width, height, sampleRate, audioChannels;
	float frameRate;

	bool bIsInitialized;
	bool bRecordAudio;
	bool bRecordVideo;
	bool bIsRecording;
	bool bIsPaused;
	bool bFinishing;
	bool bIsSilent;

	bool bSysClockSync;
	float startTime;
	float recordingDuration;
	float totalRecordingDuration;
	float systemClock();

	lockFreeQueue<ofPixels *> frames;
	lockFreeQueue<audioFrameShort *> audioFrames;
	unsigned long long audioSamplesRecorded;
	unsigned long long videoFramesRecorded;
	ofxVideoDataWriterThread videoThread;
	ofxAudioDataWriterThread audioThread;
	execThread ffmpegThread;
	execThread ffmpegVideoThread;
	execThread ffmpegAudioThread;
	bool vThreadRunning, aThreadRunning;
	int videoPipeFd, audioPipeFd;
	int pipeNumber;

	static set<int> openPipes;
	static int requestPipeNumber();
	static void retirePipeNumber(int num);

#ifdef TARGET_WIN32
	HANDLE hVPipe;
	HANDLE hAPipe;
	LPTSTR vPipename;
	LPTSTR aPipename;
#endif
};