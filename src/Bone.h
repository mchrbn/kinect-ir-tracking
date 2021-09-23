#pragma once
#include "ofxCv.h"

enum JointT {
	WRIST,
	ELBOW,
	SHOULDER,
	ANKLE,
	KNEE,
	FOOT,
	BIKE_PEDAL,
	UNDEFINED
};

struct JointPoint {
	int id;
	JointT type;
	ofVec2f pos;
};

class Bone {
public:
	JointPoint first;
	JointPoint second;
	bool real;

	void init(JointPoint _first, JointPoint _second, bool _real);
	void update(ofxCv::ContourFinder finder);
	void draw();
};