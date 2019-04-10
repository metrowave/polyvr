#include "VRPyKinematics.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/objects/VRTransform.h"
#include "VRPyPose.h"
#include "VRPyGraph.h"
#include "VRPyMath.h"

using namespace OSG;
simpleVRPyType(Kinematics, New_ptr);

PyMethodDef VRPyKinematics::methods[] = {
    {"addJoint", PyWrap( Kinematics, addJoint, "add joint - setGraph(graph)", int, int, int, VRConstraintPtr ) },
    {"addBody", PyWrapOpt( Kinematics, addBody, "add body - setGraph(graph)4", "1", int, VRTransformPtr, bool) },
    {"getTransform", PyWrap( Kinematics, getTransform, "get Transform - setGraph(graph)4", VRTransformPtr, int) },
    {"addHinge", PyWrap( Kinematics, addHinge, "Set graph - setGraph(graph)1", int, int, int, PosePtr, PosePtr, int, float, float) },
    {"addBallJoint", PyWrap( Kinematics, addBallJoint, "Set graph - setGraph(graph)2", int, int, int, PosePtr, PosePtr) },
    {"addFixedJoint", PyWrap( Kinematics, addFixedJoint, "Set graph - setGraph(graph)2", int, int, int, PosePtr, PosePtr) },
    {"addCustomJoint", PyWrap( Kinematics, addCustomJoint, "Set graph - setGraph(graph)3", int, int, int, PosePtr, PosePtr, vector<int>, vector<float>, vector<float>) },
    {"getGraph", PyWrap( Kinematics, getGraph, "Set graph - setGraph(graph)4", GraphPtr) },
    {"setDynamic", PyWrap( Kinematics, setDynamic, "Set graph - setGraph(graph)4", void, int, bool) },
    {"clear", PyWrap( Kinematics, clear, "Set graph - setGraph(graph)4", void) },


    {NULL}  /* Sentinel */
};