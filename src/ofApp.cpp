#include "ofApp.h"

#define WINDOW_W 1024
#define WINDOW_H 848

using namespace cv;
using namespace ofxCv;

int NBR_TRACKERS_TO_DETECT = 6;

//--------------------------------------------------------------
void ofApp::setup() {

	//Init Kinect
	kinect.open();
	kinect.initColorSource();
	kinect.initInfraredSource();
	kinect.initBodySource();
	kinect.initBodyIndexSource();

	//FBO for video
	fbo.allocate(WINDOW_W, WINDOW_H, GL_RGB);
	framePxls.allocate(WINDOW_W, WINDOW_H, OF_IMAGE_COLOR);

	//Allocate OpenCV images
	irImg.allocate(512, 424);

	//Set window size + bg
	ofSetWindowShape(WINDOW_W, WINDOW_H);
	ofSetBackgroundColor(255);

	//Setup GUI
	font.loadFont("arial.ttf", 13);
	gui.setup();
	gui.add(minArea.set("Min area", 2, 1, 50));
	gui.add(maxArea.set("Max area", 20, 1, 50));
	gui.add(persistance.set("Persistance", 50, 1, 100));
	gui.add(maxDist.set("Max distance", 100, 1, 100));
	gui.add(threshold.set("Threshold", 220, 0, 255));
	gui.add(clrPos.set("Color Image Pos", ofVec2f(0, 0), ofVec2f(-200, -200), ofVec2f(200, 200)));
	gui.add(clrSize.set("Color Image Size", ofVec2f(0, 0), ofVec2f(-200, -200), ofVec2f(300, 300)));
	gui.add(includeBikePoint.set("Triathlon Bike", false));
	gui.loadFromFile("settings.xml");

}

//--------------------------------------------------------------
void ofApp::update() {

	if (includeBikePoint) NBR_TRACKERS_TO_DETECT = 7;
	else NBR_TRACKERS_TO_DETECT = 6;

	//Kinect update
	kinect.update();
	if (!kinect.isFrameNew()) return;

	//Construct image from kinect Infrared sensor
	ofShortPixels pix = kinect.getInfraredSource()->getPixels();
	irImg.setFromPixels(pix);
	irImg.threshold(threshold);
	irImg.invert();

	//Parameter the finder & tracker
	finder.setMinAreaRadius(minArea);
	finder.setMaxAreaRadius(maxArea);
	finder.setFindHoles(true);

	finder.getTracker().setPersistence(persistance);
	finder.getTracker().setMaximumDistance(maxDist);

	//Find blobs
	finder.findContours(irImg);

	skeleton.update(finder, includeBikePoint, NBR_TRACKERS_TO_DETECT);

	if (isRecording) {
		fbo.readToPixels(framePxls);
		recorder.addFrame(framePxls);
	}
}

//--------------------------------------------------------------
void ofApp::draw() {

	fbo.begin();
	ofClear(0, 0);

	//IR image
	ofSetColor(255);
	if(drawMode == DrawMode::IR) irImg.draw(0, 0, WINDOW_W, WINDOW_H);
	else kinect.getColorSource()->draw(clrPos->x, clrPos->y, WINDOW_W + clrSize->x, WINDOW_H + clrSize->y);

	//Detected blobs
	ofSetColor(255, 0, 0);
	ofPushMatrix();
	ofScale(2, 2);
	finder.draw();
	ofPopMatrix();

	skeleton.draw();

	//Background rectangle
	ofFill(); 
	ofSetColor(0, 170);
	ofRect(WINDOW_W - 250, 10, 240, 130);

	//Text
	int trackersDetected = finder.size();
	ofSetColor(255);

	if (skeleton.armAngle >= 155 && skeleton.armAngle <= 165) ofSetColor(0, 255, 0);
	else ofSetColor(255, 69, 0);
	string angleStr = "Elbow angle (15° - 25°): " + ofToString(180 - skeleton.armAngle);
	ofRectangle r = font.getStringBoundingBox(angleStr, 0, 0);
	font.drawString(angleStr, ofGetWidth() - r.width - 20, 50);

	if (skeleton.shoulderAngle >= 87 && skeleton.shoulderAngle <= 93) ofSetColor(0, 255, 0);
	else ofSetColor(255, 69, 0);
	angleStr = "Shoulder angle: " + ofToString(skeleton.shoulderAngle);
	r = font.getStringBoundingBox(angleStr, 0, 0);
	font.drawString(angleStr, ofGetWidth() - r.width - 20, 70);

	ofSetColor(0, 255, 0);
	angleStr = "Ankle angle: " + ofToString(skeleton.ankleAngle);
	r = font.getStringBoundingBox(angleStr, 0, 0);
	font.drawString(angleStr, ofGetWidth() - r.width - 20, 90);

	//Optimal angle for ankle, knee, foot is beetween 138 and 145 degree
	if (skeleton.legAngle >= 138 && skeleton.legAngle <= 145 && includeBikePoint ||
		skeleton.legAngle >= 143 && skeleton.legAngle <= 153 && !includeBikePoint) ofSetColor(0, 255, 0);
	else ofSetColor(255, 69, 0);

	angleStr = "Leg angle (27° - 37°): " + ofToString(180 - skeleton.legAngle);
	r = font.getStringBoundingBox(angleStr, 0, 0);
	font.drawString(angleStr, ofGetWidth() - r.width - 20, 110);

	if (includeBikePoint) {
		//Optimal angle for the BB, ankle, shoulder is between 97 and 105
		if (skeleton.bikeAngle >= 97 && skeleton.bikeAngle <= 105) ofSetColor(0, 255, 0);
		else ofSetColor(255, 69, 0);

		angleStr = "Bike angle: " + ofToString(skeleton.bikeAngle);
		r = font.getStringBoundingBox(angleStr, 0, 0);
		font.drawString(angleStr, ofGetWidth() - r.width - 20, 130);
	}

	//GUI
	if (showGui) {
		gui.draw();
		ofSetColor(255, 0, 0);
		for (auto bone : skeleton.bones) {
			string typeStr = skeleton.jointTypeToString(bone.first.type);
			ofDrawBitmapString(typeStr, bone.first.pos.x + 20, bone.first.pos.y);
			typeStr = skeleton.jointTypeToString(bone.second.type);
			ofDrawBitmapString(typeStr, bone.second.pos.x + 20, bone.second.pos.y);
		}
	}

	fbo.end();
	fbo.draw(0, 0, WINDOW_W, WINDOW_H);
}

//--------------------------------------------------------------
int itBones = 0;
vector<ofVec2f> bonesAtlas;
void ofApp::keyPressed(int key) {

	//Show Hide GUI / G 
	if (key == 'g') showGui = !showGui;

	//Build skeleton / SPACE
	else if (key == 32) {
		itBones = 0;
		bonesAtlas.clear();
		skeleton.skeletonExists = false;
		skeleton.buildSkeletonFromTrackers();
	}

	//Change background image
	else if (key == '1') drawMode = DrawMode::IR;
	else if (key == '2') drawMode = DrawMode::COLOR;

	//Record video
	else if (key == 'r') {
		if (isRecording) {
			isRecording = false;
			recorder.close();
		}
		else {
			isRecording = true;
			recorder.setVideoCodec("h264");
			recorder.setup("sessions/session-" + ofGetTimestampString(), WINDOW_W, WINDOW_H, 30);
			recorder.start();
		}
	}

	//Create bone by pressing A on joints
	else if (key == 'a') {
		bonesAtlas.push_back(ofVec2f(ofGetMouseX(), ofGetMouseY()));
		itBones++;
		if (itBones == NBR_TRACKERS_TO_DETECT) {
			skeleton.buildSkeletonFromAtlas(bonesAtlas);
			bonesAtlas.clear();
			itBones = 0;
		}
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {

}

//--------------------------------------------------------------

void ofApp::mousePressed(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {

}
