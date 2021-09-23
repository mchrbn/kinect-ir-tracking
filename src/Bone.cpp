#include "Bone.h"

void Bone::init(JointPoint _first, JointPoint _second, bool _real) {
	first = _first;
	second = _second;
	real = _real;
}

void Bone::update(ofxCv::ContourFinder finder) {
	for (int i = 0; i < finder.size(); i++) {
		cv::Point2f pt = finder.getCenter(i);
		if (finder.getLabel(i) == first.id) {
			first.pos.set(pt.x * 2, pt.y * 2);
		}
		else if (finder.getLabel(i) == second.id) {
			second.pos.set(pt.x * 2, pt.y * 2);
		}
	}
}

void Bone::draw() {

	if (!real) return;

	//Draw joints
	ofNoFill();
	ofSetLineWidth(3);
	ofSetColor(0, 255, 0);
	ofCircle(first.pos, 10);
	ofCircle(second.pos, 10);

	//Draw bone between joints
	ofSetColor(0, 0, 255);
	ofLine(first.pos, second.pos);

	ofSetColor(255);
	ofFill();
	ofSetLineWidth(1);
}