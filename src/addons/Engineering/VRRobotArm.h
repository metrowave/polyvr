#ifndef VRROBOTARM_H_INCLUDED
#define VRROBOTARM_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include "core/utils/VRFunctionFwd.h"
#include "core/objects/VRObjectFwd.h"
#include "core/tools/VRToolsFwd.h"
#include "core/math/VRMathFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRRobotArm {
    private:
        struct job {
            PathPtr p = 0;
            PathPtr po = 0;
            float t0 = 0;
            float t1 = 1;
            bool loop = false;
            float d = 1;
            job(PathPtr p, PathPtr po = 0, float t0 = 0, float t1 = 1, float d = 1, bool loop = false) : p(p), po(po), t0(t0), t1(t1), loop(loop), d(d) {;}
        };

        VRAnalyticGeometryPtr ageo = 0;
        VRAnimationPtr anim = 0;
        VRAnimCbPtr animPtr;
        VRUpdateCbPtr updatePtr;
        PathPtr animPath = 0;
        PathPtr robotPath = 0;
        PathPtr orientationPath = 0;
        PosePtr lastPose = 0;

        list<job> job_queue;

        int N = 5;
        float grab = 0;
        float pathPos = 0;
        bool showModel = false;
        bool moving = false;
        float maxSpeed = 0.01;
        string type = "kuka";

        vector<VRTransformPtr> parts;
        vector<float> angles;
        vector<float> angle_targets;
        vector<float> angle_offsets;
        vector<int> angle_directions;
        vector<float> lengths;
        vector<int> axis;

        PosePtr getKukaPose();
        PosePtr getAuboPose();
        void calcReverseKinematicsKuka(PosePtr p);
        void calcReverseKinematicsAubo(PosePtr p);

        void update();
        void applyAngles();
        void calcReverseKinematics(PosePtr p);
        void animOnPath(float t);
        void addJob(job j);

    public:
        VRRobotArm(string type);
        ~VRRobotArm();

        static shared_ptr<VRRobotArm> create(string type);
        void showAnalytics(bool b);

        void setParts(vector<VRTransformPtr> parts);
        void setAngleOffsets(vector<float> offsets);
        void setAngleDirections(vector<int> directions);
        void setAxis(vector<int> axis);
        void setLengths(vector<float> lengths);
        void setMaxSpeed(float s);

        vector<VRTransformPtr> getParts();
        vector<float> getAngles();
        PosePtr getPose();

        void move();
        void pause();
        void stop();
        bool isMoving();

        void moveTo(PosePtr p);
        void setAngles(vector<float> angles);
        void setGrab(float g);
        void toggleGrab();

        void setPath(PathPtr p, PathPtr po = 0);
        PathPtr getPath();
        PathPtr getOrientationPath();
        void moveOnPath(float t0, float t1, bool loop = false, float durationMultiplier = 1);
};

typedef shared_ptr<VRRobotArm> VRRobotArmPtr;

OSG_END_NAMESPACE;

#endif // VRROBOTARM_H_INCLUDED
