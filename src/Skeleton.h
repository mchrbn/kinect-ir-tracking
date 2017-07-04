#pragma once
#include "ofMain.h"
#include "ofxCv.h"
#include "Bone.h"


class Skeleton {
public:

	void buildSkeletonFromTrackers();
	void buildSkeletonFromAtlas(vector<ofVec2f> _atlas);
	void createBones();
	void update(ofxCv::ContourFinder _finder, bool _ibp, int _nttd);
	void draw();
	JointT getJointTypeFromPos(ofVec2f pos);
	JointPoint getJointFromType(JointT type);
	Bone getBoneFromJoints(JointT joint1, JointT joint2);
	float getSkeletalAngle(Bone b1, Bone b2, Bone b3);
	string jointTypeToString(JointT _type);

	bool skeletonExists = false;
	vector<Bone> bones;
	vector<JointPoint> joints;

	ofxCv::ContourFinder finder;

	float armAngle, shoulderAngle, ankleAngle, legAngle, bikeAngle;

	bool includeBikePoint;
	int NBR_TRACKERS_TO_DETECT;

};