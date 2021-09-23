#pragma once

#include "ofMain.h"
#include "ofxKinectForWindows2.h"
#include "ofxOpenCv.h"
#include "ofxCv.h"
#include "ofxGui.h"
#include "ofxVideoRecorder.h"
#include "Bone.h"
#include "Skeleton.h"


enum DrawMode {
	COLOR,
	IR
};

class ofApp : public ofBaseApp {

public:
	void setup();
	void update();
	void draw();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

	ofxKFW2::Device kinect;

	ofxCvGrayscaleImage irImg;
	ofxCv::ContourFinder finder;

	ofTrueTypeFont font;

	bool showGui = false;
	ofxPanel gui;
	ofParameter<int> minArea, maxArea, persistance, maxDist, threshold;
	ofParameter<bool> includeBikePoint;
	ofParameter<ofVec2f> clrPos, clrSize;

	Skeleton skeleton;

	DrawMode drawMode = DrawMode::COLOR;

	ofxVideoRecorder recorder;
	ofFbo fbo;
	ofPixels framePxls;
	bool isRecording = false;
};
