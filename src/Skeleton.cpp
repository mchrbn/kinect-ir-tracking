#include "Skeleton.h"

using namespace cv;
using namespace ofxCv;

void Skeleton::buildSkeletonFromTrackers() {

	printf("* Create skeleton\n");
	bones.clear();
	joints.clear();

	//Calculate position of each point to build the skeleton
	RectTracker tracker = finder.getTracker();

	for (int i = 0; i < finder.size(); i++) {
		//Only take into account blobs that aren't noise
		JointPoint jp;
		jp.id = finder.getLabel(i);
		Point2f tmp = finder.getCenter(i);
		jp.pos.set(tmp.x * 2, tmp.y * 2);
		jp.type = getJointTypeFromPos(ofVec2f(tmp.x, tmp.y));
		joints.push_back(jp);
	}

	bool hasUndefinedJoints = false;
	for (auto joint : joints) {
		if (joint.type == JointT::UNDEFINED) {
			hasUndefinedJoints = true;
			break;
		}
	}

	if (joints.size() < NBR_TRACKERS_TO_DETECT || hasUndefinedJoints) return;

	createBones();
}

void Skeleton::buildSkeletonFromAtlas(vector<ofVec2f> _atlas) {

	bones.clear();
	joints.clear();

	for (int i = 0; i < _atlas.size(); i++) {
		JointPoint jp;
		switch (i) {
			case 0: jp.type = JointT::WRIST; break;
			case 1: jp.type = JointT::ELBOW; break;
			case 2: jp.type = JointT::SHOULDER; break;
			case 3: jp.type = JointT::ANKLE; break;
			case 4: jp.type = JointT::KNEE; break;
			case 5: jp.type = JointT::FOOT; break;
			case 6: jp.type = JointT::BIKE_PEDAL; break;
		}

		for (int j = 0; j < finder.size(); j++) {
			Point2f tmp = finder.getCenter(j);
			tmp *= 2;
			if (_atlas[i].x > tmp.x - 30 && _atlas[i].x < tmp.x + 30 &&
				_atlas[i].y > tmp.y - 30 && _atlas[i].y < tmp.y + 30) {
				jp.id = finder.getLabel(j);
				jp.pos.set(tmp.x, tmp.y);

				joints.push_back(jp);

				break;
			}
		}
	}

	createBones();
}

void Skeleton::createBones() {

	//Create the bones
	Bone b;
	b.init(getJointFromType(JointT::WRIST), getJointFromType(JointT::ELBOW), true);
	bones.push_back(b);
	b.init(getJointFromType(JointT::ELBOW), getJointFromType(JointT::SHOULDER), true);
	bones.push_back(b);
	b.init(getJointFromType(JointT::SHOULDER), getJointFromType(JointT::ANKLE), true);
	bones.push_back(b);
	b.init(getJointFromType(JointT::ANKLE), getJointFromType(JointT::KNEE), true);
	bones.push_back(b);
	b.init(getJointFromType(JointT::KNEE), getJointFromType(JointT::FOOT), true);
	bones.push_back(b);
	b.init(getJointFromType(JointT::SHOULDER), getJointFromType(JointT::ANKLE), true);
	bones.push_back(b);

	if (includeBikePoint) {
		b.init(getJointFromType(JointT::ANKLE), getJointFromType(JointT::BIKE_PEDAL), true);
		bones.push_back(b);

		b.init(getJointFromType(JointT::SHOULDER), getJointFromType(JointT::BIKE_PEDAL), false);
		bones.push_back(b);
	}


	//Construct the "fake" bones. Which are just here to calculate the angle later
	b.init(getJointFromType(JointT::WRIST), getJointFromType(JointT::SHOULDER), false);
	bones.push_back(b);
	b.init(getJointFromType(JointT::ELBOW), getJointFromType(JointT::ANKLE), false);
	bones.push_back(b);
	b.init(getJointFromType(JointT::SHOULDER), getJointFromType(JointT::KNEE), false);
	bones.push_back(b);
	b.init(getJointFromType(JointT::ANKLE), getJointFromType(JointT::FOOT), false);
	bones.push_back(b);


	skeletonExists = true;
}

JointT Skeleton::getJointTypeFromPos(ofVec2f pos) {
	vector<float> xvals;
	vector<float> yvals;

	//Put x and y values in separate list so we can sort them
	for (int i = 0; i < finder.size(); i++) {
		Point2f pt = finder.getCenter(i);
		xvals.push_back(pt.x);
		yvals.push_back(pt.y);
	}
	std::sort(xvals.begin(), xvals.end());
	std::sort(yvals.begin(), yvals.end());


	//Triathlon bike template for auto detection
	if (includeBikePoint) {
		if (pos.x == xvals[xvals.size() - 1] && getJointFromType(JointT::WRIST).type == JointT::UNDEFINED) return JointT::WRIST;
		else if (pos.y == yvals[0] && getJointFromType(JointT::SHOULDER).type == JointT::UNDEFINED) return JointT::SHOULDER;
		else if (pos.y == yvals[1] && getJointFromType(JointT::ANKLE).type == JointT::UNDEFINED) return JointT::ANKLE;
		else if (pos.x == xvals[xvals.size() - 2] && getJointFromType(JointT::ELBOW).type == JointT::UNDEFINED) return JointT::ELBOW;
		else if (pos.y == yvals[yvals.size() - 3] && getJointFromType(JointT::KNEE).type == JointT::UNDEFINED) return JointT::KNEE;
		else if (pos.y == yvals[yvals.size() - 2] && getJointFromType(JointT::FOOT).type == JointT::UNDEFINED) return JointT::FOOT;
		else if (pos.y == yvals[yvals.size() - 1] && getJointFromType(JointT::BIKE_PEDAL).type == JointT::UNDEFINED) return JointT::BIKE_PEDAL;
	}
	//Road bike template for auto detection
	else {
		if (pos.x == xvals[xvals.size() - 1] && getJointFromType(JointT::WRIST).type == JointT::UNDEFINED) return JointT::WRIST;
		else if (pos.x == xvals[xvals.size() - 2] && getJointFromType(JointT::ELBOW).type == JointT::UNDEFINED) return JointT::ELBOW;
		else if (pos.y == yvals[yvals.size() - 1] && getJointFromType(JointT::SHOULDER).type == JointT::UNDEFINED) return JointT::SHOULDER;
		else if (pos.x == xvals[0] && getJointFromType(JointT::ANKLE).type == JointT::UNDEFINED) return JointT::ANKLE;
		else if (pos.y == yvals[0] && getJointFromType(JointT::FOOT).type == JointT::UNDEFINED) return JointT::FOOT;
		else if (pos.y == yvals[1] && getJointFromType(JointT::KNEE).type == JointT::UNDEFINED) return JointT::KNEE;
	}

	return JointT::UNDEFINED;
}

/*
	Get a joint from a given type
*/
JointPoint Skeleton::getJointFromType(JointT type) {
	for (auto joint : joints) {
		if (type == joint.type) return joint;
	}

	//Return undefined joint if not found
	JointPoint jp;
	jp.type = JointT::UNDEFINED;
	return jp;
}

/*
	Get a bone from two given joints
*/
Bone Skeleton::getBoneFromJoints(JointT joint1, JointT joint2) {
	for (auto bone : bones) {
		if ((joint1 == bone.first.type && joint2 == bone.second.type) ||
			(joint1 == bone.second.type && joint2 == bone.first.type))
			return bone;
	}
}

/*
	Calculate a specific angle, knowing three bones
	The specific bone is composed by bone #1 and bone #2
*/
float Skeleton::getSkeletalAngle(Bone b1, Bone b2, Bone b3) {
	//Calculate the size/length of each bone
	ofVec2f deltaX = b1.second.pos - b1.first.pos;
	float x = sqrt(pow(deltaX.x, 2) + pow(deltaX.y, 2));

	ofVec2f deltaZ = b2.second.pos - b2.first.pos;
	float z = sqrt(pow(deltaZ.x, 2) + pow(deltaZ.y, 2));

	ofVec2f deltaY = b3.second.pos - b3.first.pos;
	float y = sqrt(pow(deltaY.x, 2) + pow(deltaY.y, 2));

	//Formula in case the biggest length is on the opposite side of the angle we want
	if (y > x && y > z) {
		float a = pow(x, 2) + pow(z, 2) - pow(y, 2);
		a = a / (2 * x * z);
		a = ofRadToDeg(acos(a));
		return a;
	}
	//If it's not the case, first calculate angle opposite of the biggest side
	//and then calculate the angle we want
	else {
		float biggest = x;
		float smallest = z;

		if (z > x) {
			biggest = z;
			smallest = x;
		}

		//Calculate biggest angle
		float bigA = pow(smallest, 2) + pow(y, 2) - pow(biggest, 2);
		bigA = bigA / (2 * smallest * y);
		bigA = acos(bigA);

		//Once we get the biggest angle, calculate the desired angle
		float a = y * sin(bigA);
		a = a / biggest;
		a = ofRadToDeg(asin(a));

		return a;
	}
}

string Skeleton::jointTypeToString(JointT _type) {
	switch (_type) {
		case JointT::WRIST: return "WRIST";
		case JointT::ELBOW: return "ELBOW";
		case JointT::SHOULDER: return "SHOULDER";
		case JointT::ANKLE: return "ANKLE";
		case JointT::KNEE: return "KNEE";
		case JointT::FOOT: return "FOOT";
		case JointT::BIKE_PEDAL: return "BIKE_PEDAL";
		default: return "UNDEFINED";
	}
}

void Skeleton::update(ofxCv::ContourFinder _finder, bool _ibp, int _nttd) {

	finder = _finder;
	includeBikePoint = _ibp;
	NBR_TRACKERS_TO_DETECT = _nttd;

	//Algorithm for IR tracking
	//--------------------------

	//0. If skeleton has already been built previously but lost tracker then set skeletonExists flag to false so
	//we can rebuilt it
	if (skeletonExists && finder.size() < NBR_TRACKERS_TO_DETECT) skeletonExists = false;

	//1. Check if there are 6 blobs detect (for the 6 body parts) and if the body has been constructed
	if (finder.size() >= NBR_TRACKERS_TO_DETECT && !skeletonExists) buildSkeletonFromTrackers();

	//Don't go further if skeleton doesn't exist or if trackers are lost
	if (finder.size() < NBR_TRACKERS_TO_DETECT || !skeletonExists) return;

	//2. For each blobs update their position
	//iterate through joint list + 
	for (int i = 0; i < bones.size(); i++) {
		bones[i].update(finder);
	}

	//3. Calculate the angles
	armAngle = getSkeletalAngle(getBoneFromJoints(JointT::WRIST, JointT::ELBOW), getBoneFromJoints(JointT::ELBOW, JointT::SHOULDER),
		getBoneFromJoints(JointT::WRIST, JointT::SHOULDER));

	shoulderAngle = getSkeletalAngle(getBoneFromJoints(JointT::ELBOW, JointT::SHOULDER), getBoneFromJoints(JointT::SHOULDER, JointT::ANKLE),
		getBoneFromJoints(JointT::ELBOW, JointT::ANKLE));

	ankleAngle = getSkeletalAngle(getBoneFromJoints(JointT::SHOULDER, JointT::ANKLE), getBoneFromJoints(JointT::ANKLE, JointT::KNEE),
		getBoneFromJoints(JointT::SHOULDER, JointT::KNEE));

	legAngle = getSkeletalAngle(getBoneFromJoints(JointT::ANKLE, JointT::KNEE), getBoneFromJoints(JointT::KNEE, JointT::FOOT),
		getBoneFromJoints(JointT::ANKLE, JointT::FOOT));

	if (includeBikePoint)
		bikeAngle = getSkeletalAngle(getBoneFromJoints(JointT::SHOULDER, JointT::ANKLE), getBoneFromJoints(JointT::ANKLE, JointT::BIKE_PEDAL),
			getBoneFromJoints(JointT::SHOULDER, JointT::BIKE_PEDAL));
}

void Skeleton::draw() {
	//Skeleton
	for (auto bone : bones) bone.draw();

}