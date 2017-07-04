# Kinect IR Tracking - Side Skeleton

![alt text](http://i.imgur.com/A9myI79.gif)

Accurate tracking of the side body using the Kinect IR camera with IR reflective tapes.

The software was developed for a specific use case which is to detect if a cyclist's position on its bike is optimal or not ([bike fitting](https://www.competitivecyclist.com/Store/catalog/fitCalculatorBike.jsp)) - by calculating various angles such as knee flexion, trunk angle, shoulder angle and elbow angle.

A piece of reflective tape is placed on different joints of the cyclist's body in order to let the software create and track the side skeleton. This solution was used as the skeleton tracking provided by the Kinect SDK wasn't accurate and precise enough.

The software can work for both a road bike fit test as well as a triathlon bike fit test. It automatically detects the skeleton when the users is sitted on its bike.

Made with [openframeworks 0.9.8](http://openframeworks.cc/)

## Use
- 'r' : Record / Stop record a video (saved in /bin/data/sessions/)
- 'g' : Show / Hide settings
- '1' : IR image
- '2' : Color image
- 'Space' : Reset skeleton
- 'a' : Manually construct the skeleton by putting the mouse pointer on each blob and pressing 'a'. In the following order : wrist -> elbow -> shoulder -> ankle -> knee -> foot

## Dependencies
Install [Kinect SDK 2.0](https://www.microsoft.com/en-gb/download/details.aspx?id=44561) for Windows
Install [ffmpeg](http://ffmpeg.zeranoe.com/builds/) and add ffmpeg to user PATH variable

## OF Addons
- ofxGui
- ofxKinectForWindows2
- ofxCv
- ofxOpenCv
- ofxVideoRecorder (modified use the one in addons/ folder)