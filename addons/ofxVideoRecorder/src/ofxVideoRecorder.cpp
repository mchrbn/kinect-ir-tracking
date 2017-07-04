//
//  ofxVideoRecorder.cpp
//  ofxVideoRecorderExample
//
//  Created by Timothy Scaffidi on 9/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

//ffmpeg -y  -an -r 30 -s 1024x848 -f rawvideo -pix_fmt rgb24 -i \\.\pipe\videoPipe805 -vcodec mpeg4 -b:v 2000k C:\Users\Bopuc Beach\Documents\of_v0.9.8_vs_release\apps\myApps\ir-bike-tracking\bin\data\session-2017-06-13-18-34-17-491.mp4

#include "ofxVideoRecorder.h"
#if defined( TARGET_OSX ) || defined( TARGET_LINUX )
#include <unistd.h>
#endif
#ifdef TARGET_WIN32
#include <io.h>
#endif
#include <fcntl.h>

int setNonBlocking(int fd)
{
#if defined( TARGET_OSX ) || defined( TARGET_LINUX )
	int flags;

	/* If they have O_NONBLOCK, use the Posix way to do it */
#if defined(O_NONBLOCK)
	/* Fixme: O_NONBLOCK is defined but broken on SunOS 4.1.x and AIX 3.2.5. */
	if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
		flags = 0;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
	/* Otherwise, use the old way of doing it */
	flags = 1;
	return ioctl(fd, FIOBIO, &flags);
#endif
#endif
	return 0;
}

//===============================
execThread::execThread() {
	execCommand = "";
}

void execThread::setup(string command) {
	execCommand = command;
	startThread(true);
}

void execThread::threadedFunction() {
	if (isThreadRunning()) {
		system(execCommand.c_str());
	}
}

//===============================
ofxVideoDataWriterThread::ofxVideoDataWriterThread() {
	thread.setName("Video Thread");
};
#if defined( TARGET_OSX ) || defined( TARGET_LINUX )
void ofxVideoDataWriterThread::setup(string filePath, lockFreeQueue<ofPixels *> * q) {
	this->filePath = filePath;
	fd = -1;
	queue = q;
	bIsWriting = false;
	bClose = false;
	startThread(true);
}
#endif
#ifdef TARGET_WIN32
void ofxVideoDataWriterThread::setup(HANDLE videoHandle_, lockFreeQueue<ofPixels *> * q) {
	queue = q;
	bIsWriting = false;
	bClose = false;
	videoHandle = videoHandle_;
	startThread(true);
}
#endif
void ofxVideoDataWriterThread::threadedFunction() {
#if defined( TARGET_OSX ) || defined( TARGET_LINUX )
	if (fd == -1) {
		fd = ::open(filePath.c_str(), O_WRONLY);
	}
#endif
	//maybe create file here? these threads act as the client and the main thread as the server?
	while (isThreadRunning())
	{
		ofPixels * frame = NULL;
		if (queue->Consume(frame) && frame) {
			bIsWriting = true;
			int b_offset = 0;
			int b_remaining = frame->getWidth()*frame->getHeight()*frame->getBytesPerPixel();

			while (b_remaining > 0 && isThreadRunning())
			{
#if defined( TARGET_OSX ) || defined( TARGET_LINUX )
				errno = 0;

				int b_written = ::write(fd, ((char *)frame->getPixels()) + b_offset, b_remaining);
#endif
#ifdef TARGET_WIN32
				DWORD b_written;
				if (!WriteFile(videoHandle, ((char *)frame->getPixels()) + b_offset, b_remaining, &b_written, 0)) {
					LPTSTR errorText = NULL;

					FormatMessageW(
						// use system message tables to retrieve error text
						FORMAT_MESSAGE_FROM_SYSTEM
						// allocate buffer on local heap for error text
						| FORMAT_MESSAGE_ALLOCATE_BUFFER
						// Important! will fail otherwise, since we're not 
						// (and CANNOT) pass insertion parameters
						| FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,    // unused with FORMAT_MESSAGE_FROM_SYSTEM
						GetLastError(),
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
						(LPTSTR)&errorText,  // output 
						0, // minimum size for output buffer
						NULL);   // arguments - see note 
					wstring ws = errorText;
					string error(ws.begin(), ws.end());
					ofLogNotice("Video Thread") << "WriteFile to pipe failed: " << error;
					break;
				}
#endif
				if (b_written > 0) {
					b_remaining -= b_written;
					b_offset += b_written;
					if (b_remaining != 0) {
						ofLogWarning("ofxVideoDataWriterThread") << ofGetTimestampString("%H:%M:%S:%i") << " - b_remaining is not 0 -> " << b_written << " - " << b_remaining << " - " << b_offset << ".";
						//break;
					}
				}
				else if (b_written < 0) {
					ofLogError("ofxVideoDataWriterThread") << ofGetTimestampString("%H:%M:%S:%i") << " - write to PIPE failed with error -> " << errno << " - " << strerror(errno) << ".";
					break;
				}
				else {
					if (bClose) {
						ofLogVerbose("ofxVideoDataWriterThread") << ofGetTimestampString("%H:%M:%S:%i") << " - Nothing was written and bClose is TRUE.";
						break; // quit writing so we can close the file
					}
					ofLogWarning("ofxVideoDataWriterThread") << ofGetTimestampString("%H:%M:%S:%i") << " - Nothing was written. Is this normal?";
				}

				if (!isThreadRunning()) {
					ofLogWarning("ofxVideoDataWriterThread") << ofGetTimestampString("%H:%M:%S:%i") << " - The thread is not running anymore let's get out of here!";
				}
			}
			bIsWriting = false;
			frame->clear();
			delete frame;
		}
		else {
			conditionMutex.lock();
			condition.wait(conditionMutex);
			conditionMutex.unlock();
		}
	}

#if defined( TARGET_OSX ) || defined( TARGET_LINUX )
	::close(fd);
#endif
#ifdef TARGET_WIN32
	FlushFileBuffers(videoHandle);
	DisconnectNamedPipe(videoHandle);
	CloseHandle(videoHandle);
#endif
}

void ofxVideoDataWriterThread::signal() {
	condition.signal();
}

void ofxVideoDataWriterThread::setPipeNonBlocking() {
	setNonBlocking(fd);
}

//===============================
ofxAudioDataWriterThread::ofxAudioDataWriterThread() {
	thread.setName("Audio Thread");
};

#if defined( TARGET_OSX ) || defined( TARGET_LINUX )
void ofxAudioDataWriterThread::setup(string filePath, lockFreeQueue<audioFrameShort *> *q) {
	this->filePath = filePath;
	fd = -1;
	queue = q;
	bIsWriting = false;
	startThread(true);
}
#endif
#ifdef TARGET_WIN32
void ofxAudioDataWriterThread::setup(HANDLE audioHandle_, lockFreeQueue<audioFrameShort *> *q) {
	queue = q;
	bIsWriting = false;
	audioHandle = audioHandle_;
	startThread(true);
}
#endif
void ofxAudioDataWriterThread::threadedFunction() {
#if defined( TARGET_OSX ) || defined( TARGET_LINUX )

	if (fd == -1) {
		//write only, fd is the handle what is the windows eqivalent
		fd = ::open(filePath.c_str(), O_WRONLY);
	}
#endif
	while (isThreadRunning())
	{
		audioFrameShort * frame = NULL;
		if (queue->Consume(frame) && frame) {
			bIsWriting = true;
			int b_offset = 0;
			int b_remaining = frame->size * sizeof(short);
			while (b_remaining > 0) {
#if defined( TARGET_OSX ) || defined( TARGET_LINUX )
				int b_written = ::write(fd, ((char *)frame->data) + b_offset, b_remaining);
#endif
#ifdef TARGET_WIN32
				DWORD b_written;
				if (!WriteFile(audioHandle, ((char *)frame->data) + b_offset, b_remaining, &b_written, 0)) {
					LPTSTR errorText = NULL;

					FormatMessageW(
						// use system message tables to retrieve error text
						FORMAT_MESSAGE_FROM_SYSTEM
						// allocate buffer on local heap for error text
						| FORMAT_MESSAGE_ALLOCATE_BUFFER
						// Important! will fail otherwise, since we're not 
						// (and CANNOT) pass insertion parameters
						| FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,    // unused with FORMAT_MESSAGE_FROM_SYSTEM
						GetLastError(),
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
						(LPTSTR)&errorText,  // output 
						0, // minimum size for output buffer
						NULL);   // arguments - see note 
					wstring ws = errorText;
					string error(ws.begin(), ws.end());
					ofLogNotice("Audio Thread") << "WriteFile to pipe failed: " << error;
				}
#endif
				if (b_written > 0) {
					b_remaining -= b_written;
					b_offset += b_written;
				}
				else if (b_written < 0) {
					ofLogError("ofxAudioDataWriterThread") << ofGetTimestampString("%H:%M:%S:%i") << " - write to PIPE failed with error -> " << errno << " - " << strerror(errno) << ".";
					break;
				}
				else {
					if (bClose) {
						break; // quit writing so we can close the file
					}
				}

				if (!isThreadRunning()) {
					ofLogWarning("ofxAudioDataWriterThread") << ofGetTimestampString("%H:%M:%S:%i") << " - The thread is not running anymore let's get out of here!";
				}
			}
			bIsWriting = false;
			delete[] frame->data;
			delete frame;
		}
		else {
			conditionMutex.lock();
			condition.wait(conditionMutex);
			conditionMutex.unlock();
		}
	}

#if defined( TARGET_OSX ) || defined( TARGET_LINUX )
	::close(fd);
#endif
#ifdef TARGET_WIN32
	FlushFileBuffers(audioHandle);
	DisconnectNamedPipe(audioHandle);
	CloseHandle(audioHandle);
#endif
}
void ofxAudioDataWriterThread::signal() {
	condition.signal();
}

void ofxAudioDataWriterThread::setPipeNonBlocking() {
	setNonBlocking(fd);
}

//===============================
ofxVideoRecorder::ofxVideoRecorder()
{
	bIsInitialized = false;
	ffmpegLocation = "ffmpeg";
	videoCodec = "mpeg4";
	audioCodec = "pcm_s16le";
	videoBitrate = "2000k";
	audioBitrate = "128k";
	pixelFormat = "rgb24";
	movFileExt = ".mp4";
	audioFileExt = ".m4a";
	aThreadRunning = false;
	vThreadRunning = false;
}

bool ofxVideoRecorder::setup(string fname, int w, int h, float fps, int sampleRate, int channels, bool sysClockSync, bool silent)
{
	if (bIsInitialized)
	{
		close();
	}

	fileName = fname;
	filePath = ofFilePath::getAbsolutePath(fileName);
	ofStringReplace(filePath, " ", "\ ");

	return setupCustomOutput(w, h, fps, sampleRate, channels, filePath);
}

bool ofxVideoRecorder::setupCustomOutput(int w, int h, float fps, string outputString, bool sysClockSync, bool silent) {
	return setupCustomOutput(w, h, fps, 0, 0, outputString, sysClockSync, silent);
}

bool ofxVideoRecorder::setupCustomOutput(int w, int h, float fps, int sampleRate, int channels, string outputString, bool sysClockSync, bool silent)
{
	if (bIsInitialized)
	{
		close();
	}

	bIsSilent = silent;
	bSysClockSync = sysClockSync;

	bRecordAudio = (sampleRate > 0 && channels > 0);
	bRecordVideo = (w > 0 && h > 0 && fps > 0);
	bFinishing = false;

	videoFramesRecorded = 0;
	audioSamplesRecorded = 0;

	if (!bRecordVideo && !bRecordAudio) {
		ofLogWarning() << "ofxVideoRecorder::setupCustomOutput(): invalid parameters, could not setup video or audio stream.\n"
			<< "video: " << w << "x" << h << "@" << fps << "fps\n"
			<< "audio: " << "channels: " << channels << " @ " << sampleRate << "Hz\n";
		return false;
	}
	videoPipePath = "";
	audioPipePath = "";
	pipeNumber = requestPipeNumber();
	if (bRecordVideo) {
		width = w;
		height = h;
		frameRate = fps;

#if defined( TARGET_OSX ) || defined( TARGET_LINUX )
		// recording video, create a FIFO pipe
		videoPipePath = ofFilePath::getAbsolutePath("ofxvrpipe" + ofToString(pipeNumber));

		ofStringReplace(videoPipePath, " ", "\\ ");

		if (!ofFile::doesFileExist(videoPipePath)) {
			string cmd = "bash --login -c 'mkfifo " + videoPipePath + "'";
			system(cmd.c_str());
		}
#endif
#ifdef TARGET_WIN32

		char vpip[128];
		int num = ofRandom(1024);
		sprintf(vpip, "\\\\.\\pipe\\videoPipe%d", num);
		vPipename = convertCharArrayToLPCWSTR(vpip);

		hVPipe = CreateNamedPipe(
			vPipename, // name of the pipe
			PIPE_ACCESS_OUTBOUND, // 1-way pipe -- send only
			PIPE_TYPE_BYTE, // send data as a byte stream
			1, // only allow 1 instance of this pipe
			0, // outbound buffer defaults to system default
			0, // no inbound buffer
			0, // use default wait time
			NULL // use default security attributes
		);

		if (!(hVPipe != INVALID_HANDLE_VALUE)) {
			if (GetLastError() != ERROR_PIPE_BUSY)
			{
				ofLogError("Video Pipe") << "Could not open video pipe.";
			}
			// All pipe instances are busy, so wait for 5 seconds. 
			if (!WaitNamedPipe(vPipename, 5000))
			{
				ofLogError("Video Pipe") << "Could not open video pipe: 5 second wait timed out.";
			}
		}

#endif
	}

	if (bRecordAudio) {
		this->sampleRate = sampleRate;
		audioChannels = channels;

#if defined( TARGET_OSX ) || defined( TARGET_LINUX )
		// recording video, create a FIFO pipe
		audioPipePath = ofFilePath::getAbsolutePath("ofxarpipe" + ofToString(pipeNumber));

		ofStringReplace(audioPipePath, " ", "\\ ");

		if (!ofFile::doesFileExist(audioPipePath)) {

			string cmd = "bash --login -c 'mkfifo " + audioPipePath + "'";
			system(cmd.c_str());
		}
#endif
#ifdef TARGET_WIN32


		char apip[128];
		int num = ofRandom(1024);
		sprintf(apip, "\\\\.\\pipe\\videoPipe%d", num);
		aPipename = convertCharArrayToLPCWSTR(apip);

		hAPipe = CreateNamedPipe(
			aPipename,
			PIPE_ACCESS_OUTBOUND, // 1-way pipe -- send only
			PIPE_TYPE_BYTE, // send data as a byte stream
			1, // only allow 1 instance of this pipe
			0, // outbound buffer defaults to system default
			0, // no inbound buffer
			0, // use default wait time
			NULL // use default security attributes
		);

		if (!(hAPipe != INVALID_HANDLE_VALUE)) {
			if (GetLastError() != ERROR_PIPE_BUSY)
			{
				ofLogError("Audio Pipe") << "Could not open audio pipe.";
			}
			// All pipe instances are busy, so wait for 5 seconds. 
			if (!WaitNamedPipe(aPipename, 5000))
			{
				ofLogError("Audio Pipe") << "Could not open pipe: 5 second wait timed out.";
			}
		}

#endif
	}

	stringstream cmd;
	// basic ffmpeg invocation, -y option overwrites output file
#if defined( TARGET_OSX ) || defined( TARGET_LINUX )
	cmd << "bash --login -c '" << ffmpegLocation << (bIsSilent ? " -loglevel quiet " : " ") << "-y";
	if (bRecordAudio) {
		cmd << " -acodec pcm_s16le -f s16le -ar " << sampleRate << " -ac " << audioChannels << " -i " << audioPipePath;
	}
	else { // no audio stream
		cmd << " -an";
	}
	if (bRecordVideo) { // video input options and file
		cmd << " -r " << fps << " -s " << w << "x" << h << " -f rawvideo -pix_fmt " << pixelFormat << " -i " << videoPipePath << " -r " << fps;
	}
	else { // no video stream
		cmd << " -vn";
	}
	cmd << " " + outputString + "' &";

	//cerr << cmd.str();

	ffmpegThread.setup(cmd.str()); // start ffmpeg thread, will wait for input pipes to be opened

	if (bRecordAudio) {
		audioThread.setup(audioPipePath, &audioFrames);
	}
	if (bRecordVideo) {
		videoThread.setup(videoPipePath, &frames);
	}
#endif
#ifdef TARGET_WIN32
	//evidently there are issues with multiple named pipes http://trac.ffmpeg.org/ticket/1663

	if (bRecordAudio && bRecordVideo) {
		bool fSuccess;

		// Audio Thread

		stringstream aCmd;
		aCmd << ffmpegLocation << " -y " << " -f s16le -acodec " << audioCodec << " -ar " << sampleRate << " -ac " << audioChannels;
		aCmd << " -i " << convertWideToNarrow(aPipename) << " -b:a " << audioBitrate << " " << outputString << "_atemp" << audioFileExt;

		ffmpegAudioThread.setup(aCmd.str());
		ofLogNotice("FFMpeg Command") << aCmd.str() << endl;

		fSuccess = ConnectNamedPipe(hAPipe, NULL);
		if (!fSuccess)
		{
			LPTSTR errorText = NULL;

			FormatMessageW(
				// use system message tables to retrieve error text
				FORMAT_MESSAGE_FROM_SYSTEM
				// allocate buffer on local heap for error text
				| FORMAT_MESSAGE_ALLOCATE_BUFFER
				// Important! will fail otherwise, since we're not 
				// (and CANNOT) pass insertion parameters
				| FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,    // unused with FORMAT_MESSAGE_FROM_SYSTEM
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR)&errorText,  // output 
				0, // minimum size for output buffer
				NULL);   // arguments - see note 
			wstring ws = errorText;
			string error(ws.begin(), ws.end());
			ofLogError("Audio Pipe") << "SetNamedPipeHandleState failed: " << error;
		}
		else {
			ofLogNotice("Audio Pipe") << "\n==========================\nAudio Pipe Connected Successfully\n==========================\n" << endl;
			audioThread.setup(hAPipe, &audioFrames);
		}

		// Video Thread

		stringstream vCmd;
		vCmd << ffmpegLocation << " -y " << " -r " << fps << " -s " << w << "x" << h << " -f rawvideo -pix_fmt " << pixelFormat;
		vCmd << " -i " << convertWideToNarrow(vPipename) << " -vcodec " << videoCodec << " -b:v " << videoBitrate << " " << outputString << "_vtemp" << movFileExt;

		ffmpegVideoThread.setup(vCmd.str());
		ofLogNotice("FFMpeg Command") << vCmd.str() << endl;

		fSuccess = ConnectNamedPipe(hVPipe, NULL);
		if (!fSuccess)
		{
			LPTSTR errorText = NULL;

			FormatMessageW(
				// use system message tables to retrieve error text
				FORMAT_MESSAGE_FROM_SYSTEM
				// allocate buffer on local heap for error text
				| FORMAT_MESSAGE_ALLOCATE_BUFFER
				// Important! will fail otherwise, since we're not 
				// (and CANNOT) pass insertion parameters
				| FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,    // unused with FORMAT_MESSAGE_FROM_SYSTEM
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR)&errorText,  // output 
				0, // minimum size for output buffer
				NULL);   // arguments - see note 
			wstring ws = errorText;
			string error(ws.begin(), ws.end());
			ofLogError("Video Pipe") << "SetNamedPipeHandleState failed: " << error;
		}
		else {
			ofLogNotice("Video Pipe") << "\n==========================\nVideo Pipe Connected Successfully\n==========================\n" << endl;
			videoThread.setup(hVPipe, &frames);
		}
	}
	else {
		cmd << ffmpegLocation << " -y ";
		if (bRecordAudio) {
			cmd << " -f s16le -acodec " << audioCodec << " -ar " << sampleRate << " -ac " << audioChannels << " -i " << convertWideToNarrow(aPipename);
		}
		else { // no audio stream
			cmd << " -an";
		}
		if (bRecordVideo) { // video input options and file
			cmd << " -r " << fps << " -s " << w << "x" << h << " -f rawvideo -pix_fmt " << pixelFormat << " -i " << convertWideToNarrow(vPipename);
		}
		else { // no video stream
			cmd << " -vn";
		}
		if (bRecordAudio)
			cmd << " -b:a " << audioBitrate;
		if (bRecordVideo)
			cmd << " -vcodec " << videoCodec << " -b:v " << videoBitrate;
		cmd << " \"" << outputString << movFileExt << "\"";

		//cmd.clear();
		//cmd << ffmpegLocation << " -f rawvideo -vcodec rawvideo -s 1920x1080 -pix_fmt rgb24 -r 24 -i " << convertWideToNarrow(vPipename) << " - -an " << " \"" << outputString << movFileExt << "\"";

		ofLogNotice("FFMpeg Command") << cmd.str() << endl;

		ffmpegThread.setup(cmd.str()); // start ffmpeg thread, will wait for input pipes to be opened

		if (bRecordAudio) {
			//this blocks, so we have to call it after ffmpeg is listening for a pipe
			bool fSuccess = ConnectNamedPipe(hAPipe, NULL);
			if (!fSuccess)
			{
				LPTSTR errorText = NULL;

				FormatMessageW(
					// use system message tables to retrieve error text
					FORMAT_MESSAGE_FROM_SYSTEM
					// allocate buffer on local heap for error text
					| FORMAT_MESSAGE_ALLOCATE_BUFFER
					// Important! will fail otherwise, since we're not 
					// (and CANNOT) pass insertion parameters
					| FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,    // unused with FORMAT_MESSAGE_FROM_SYSTEM
					GetLastError(),
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPTSTR)&errorText,  // output 
					0, // minimum size for output buffer
					NULL);   // arguments - see note 
				wstring ws = errorText;
				string error(ws.begin(), ws.end());
				ofLogError("Audio Pipe") << "SetNamedPipeHandleState failed: " << error;
			}
			else {
				ofLogNotice("Audio Pipe") << "\n==========================\nAudio Pipe Connected Successfully\n==========================\n" << endl;
				audioThread.setup(hAPipe, &audioFrames);
			}
		}
		if (bRecordVideo) {
			//this blocks, so we have to call it after ffmpeg is listening for a pipe
			bool fSuccess = ConnectNamedPipe(hVPipe, NULL);
			if (!fSuccess)
			{
				LPTSTR errorText = NULL;

				FormatMessageW(
					// use system message tables to retrieve error text
					FORMAT_MESSAGE_FROM_SYSTEM
					// allocate buffer on local heap for error text
					| FORMAT_MESSAGE_ALLOCATE_BUFFER
					// Important! will fail otherwise, since we're not 
					// (and CANNOT) pass insertion parameters
					| FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,    // unused with FORMAT_MESSAGE_FROM_SYSTEM
					GetLastError(),
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPTSTR)&errorText,  // output 
					0, // minimum size for output buffer
					NULL);   // arguments - see note 
				wstring ws = errorText;
				string error(ws.begin(), ws.end());
				ofLogError("Video Pipe") << "SetNamedPipeHandleState failed: " << error;
			}
			else {
				ofLogNotice("Video Pipe") << "\n==========================\nVideo Pipe Connected Successfully\n==========================\n" << endl;
				videoThread.setup(hVPipe, &frames);
			}
		}

	}
#endif

	bIsInitialized = true;
	bIsRecording = false;
	bIsPaused = false;

	startTime = 0;
	recordingDuration = 0;
	totalRecordingDuration = 0;

	return bIsInitialized;
}

bool ofxVideoRecorder::runCustomScript(string script)
{
	stringstream cmd;
	cmd << ffmpegLocation << " -y ";


	ofLogNotice("FFMpeg Command") << script << endl;
	ffmpegThread.setup(script);

	bIsInitialized = true;

	return bIsInitialized;
}

bool ofxVideoRecorder::addFrame(const ofPixels &pixels)
{
	if (!bIsRecording || bIsPaused) return false;

	if (bIsInitialized && bRecordVideo)
	{
		int framesToAdd = 1; //default add one frame per request

		if ((bRecordAudio || bSysClockSync) && !bFinishing) {

			double syncDelta;
			double videoRecordedTime = videoFramesRecorded / frameRate;

			if (bRecordAudio) {
				//if also recording audio, check the overall recorded time for audio and video to make sure audio is not going out of sync
				//this also handles incoming dynamic framerate while maintaining desired outgoing framerate
				double audioRecordedTime = (audioSamplesRecorded / audioChannels) / (double)sampleRate;
				syncDelta = audioRecordedTime - videoRecordedTime;
			}
			else {
				//if just recording video, synchronize the video against the system clock
				//this also handles incoming dynamic framerate while maintaining desired outgoing framerate
				syncDelta = systemClock() - videoRecordedTime;
			}

			if (syncDelta > 1.0 / frameRate) {
				//no enought video frames, we need to send extra video frames.
				int numFramesCopied = 0;
				while (syncDelta > 1.0 / frameRate) {
					framesToAdd++;
					syncDelta -= 1.0 / frameRate;
				}
				ofLogVerbose() << "ofxVideoRecorder: recDelta = " << syncDelta << ". Not enough video frames for desired frame rate, copied this frame " << framesToAdd << " times.\n";
			}
			else if (syncDelta < -1.0 / frameRate) {
				//more than one video frame is waiting, skip this frame
				framesToAdd = 0;
				ofLogVerbose() << "ofxVideoRecorder: recDelta = " << syncDelta << ". Too many video frames, skipping.\n";
				return false;
			}
		}

		for (int i = 0; i < framesToAdd; i++) {
			//add desired number of frames
			frames.Produce(new ofPixels(pixels));
			videoFramesRecorded++;
		}
		videoThread.signal();
		return true;
	}
	return false;
}

void ofxVideoRecorder::addAudioSamples(float *samples, int bufferSize, int numChannels) {
	if (!bIsRecording || bIsPaused) return;

	if (bIsInitialized && bRecordAudio) {
		int size = bufferSize*numChannels;
		audioFrameShort * shortSamples = new audioFrameShort;
		shortSamples->data = new short[size];
		shortSamples->size = size;

		for (int i = 0; i < size; i++) {
			shortSamples->data[i] = (short)(samples[i] * 32767.0f);
		}
		audioFrames.Produce(shortSamples);
		audioThread.signal();
		audioSamplesRecorded += size;
	}
}

void ofxVideoRecorder::start()
{
	if (!bIsInitialized) return;

	if (bIsRecording) {
		//  We are already recording. No need to go further.
		return;
	}

	// Start a recording.
	bIsRecording = true;
	bIsPaused = false;
	startTime = ofGetElapsedTimef();

	ofLogVerbose() << "Recording." << endl;
}

void ofxVideoRecorder::setPaused(bool bPause)
{
	if (!bIsInitialized) return;

	if (!bIsRecording || bIsPaused == bPause) {
		//  We are not recording or we are already paused. No need to go further.
		return;
	}

	// Pause the recording
	bIsPaused = bPause;

	if (bIsPaused) {
		totalRecordingDuration += recordingDuration;

		// Log
		ofLogVerbose() << "Paused." << endl;
	}
	else {
		startTime = ofGetElapsedTimef();

		// Log
		ofLogVerbose() << "Recording." << endl;
	}
}

void ofxVideoRecorder::close()
{
	if (!bIsInitialized) return;

	bIsRecording = false;

#if defined( TARGET_OSX ) || defined( TARGET_LINUX )

	if (bRecordVideo && bRecordAudio) {
		//set pipes to non_blocking so we dont get stuck at the final writes
		audioThread.setPipeNonBlocking();
		videoThread.setPipeNonBlocking();

		while (frames.size() > 0 && audioFrames.size() > 0) {
			// if there are frames in the queue or the thread is writing, signal them until the work is done.
			videoThread.signal();
			audioThread.signal();
		}
	}
	else if (bRecordVideo) {
		//set pipes to non_blocking so we dont get stuck at the final writes
		videoThread.setPipeNonBlocking();

		while (frames.size() > 0) {
			// if there are frames in the queue or the thread is writing, signal them until the work is done.
			videoThread.signal();
		}
	}
	else if (bRecordAudio) {
		//set pipes to non_blocking so we dont get stuck at the final writes
		audioThread.setPipeNonBlocking();

		while (audioFrames.size() > 0) {
			// if there are frames in the queue or the thread is writing, signal them until the work is done.
			audioThread.signal();
		}
	}

	//at this point all data that ffmpeg wants should have been consumed
	// one of the threads may still be trying to write a frame,
	// but once close() gets called they will exit the non_blocking write loop
	// and hopefully close successfully

	bIsInitialized = false;

	if (bRecordVideo) {
		videoThread.close();
	}
	if (bRecordAudio) {
		audioThread.close();
	}

	retirePipeNumber(pipeNumber);

	ffmpegThread.waitForThread();
#endif
#ifdef TARGET_WIN32 
	if (bRecordVideo) {
		videoThread.close();
	}
	if (bRecordAudio) {
		audioThread.close();
	}

	//at this point all data that ffmpeg wants should have been consumed
	// one of the threads may still be trying to write a frame,
	// but once close() gets called they will exit the non_blocking write loop
	// and hopefully close successfully

	if (bRecordAudio && bRecordVideo) {
		ffmpegAudioThread.waitForThread();
		ffmpegVideoThread.waitForThread();

		//need to do one last script here to join the audio and video recordings

		stringstream finalCmd;

		/*finalCmd << ffmpegLocation << " -y " << " -i " << filePath << "_vtemp" << movFileExt << " -i " << filePath << "_atemp" << movFileExt << " \\ ";
		finalCmd << "-filter_complex \"[0:0] [1:0] concat=n=2:v=1:a=1 [v] [a]\" \\";
		finalCmd << "-map \"[v]\" -map \"[a]\" ";
		finalCmd << " -vcodec " << videoCodec << " -b:v " << videoBitrate << " -b:a " << audioBitrate << " ";
		finalCmd << filePath << movFileExt;*/

		finalCmd << ffmpegLocation << " -y " << " -i " << filePath << "_vtemp" << movFileExt << " -i " << filePath << "_atemp" << audioFileExt << " ";
		finalCmd << "-c:v copy -c:a copy -strict experimental ";
		finalCmd << filePath << movFileExt;

		ofLogNotice("FFMpeg Merge") << "\n==============================================\n Merge Command \n==============================================\n";
		ofLogNotice("FFMpeg Merge") << finalCmd.str();
		//ffmpegThread.setup(finalCmd.str());
		system(finalCmd.str().c_str());

		//delete the unmerged files
		stringstream removeCmd;
		ofStringReplace(filePath, "/", "\\");
		removeCmd << "DEL " << filePath << "_vtemp" << movFileExt << " " << filePath << "_atemp" << audioFileExt;
		system(removeCmd.str().c_str());

	}

	ffmpegThread.waitForThread();

#endif
	// TODO: kill ffmpeg process if its taking too long to close for whatever reason.
	ofLogNotice("ofxVideoRecorder") << "\n==============================================\n Closed ffmpeg \n==============================================\n";
	bIsInitialized = false;
}

float ofxVideoRecorder::systemClock()
{
	recordingDuration = ofGetElapsedTimef() - startTime;
	return totalRecordingDuration + recordingDuration;
}

set<int> ofxVideoRecorder::openPipes;

int ofxVideoRecorder::requestPipeNumber() {
	int n = 0;
	while (openPipes.find(n) != openPipes.end()) {
		n++;
	}
	openPipes.insert(n);
	return n;
}

void ofxVideoRecorder::retirePipeNumber(int num) {
	if (!openPipes.erase(num)) {
		ofLogNotice() << "ofxVideoRecorder::retirePipeNumber(): trying to retire a pipe number that is not being tracked: " << num << endl;
	}
}