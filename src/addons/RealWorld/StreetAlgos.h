#ifndef STREETALGOS_H
#define	STREETALGOS_H

#include <OpenSG/OSGVector.h>
#include <string>
#include <vector>
#include <map>

OSG_BEGIN_NAMESPACE;
using namespace std;

struct StreetBorder;
class StreetSegment;
class StreetJoint;
class JointPoints;

struct StreetBorder {
    string streetSegmentId;
    Vec2d start;
    Vec2d end;

    StreetBorder(string streetSegmentId, Vec2d start, Vec2d end);
    Vec2d dir();
};

class StreetAlgos {
    public:
        /** returns start && end point of the left border of a street segment */
        static StreetBorder* segmentGetLeftBorderTo(StreetSegment* seg, string jointId, map<string, StreetJoint*> streetJoints);

        /** returns start && end point of the right border of a street segment */
        static StreetBorder* segmentGetRightBorderTo(StreetSegment* seg, string jointId, map<string, StreetJoint*> streetJoints);

        /** returns imporant points to a joint for one street segment */
        static JointPoints* segmentGetPointsFor(StreetSegment* seg, string jointId);

        /** calculates the important points of a given joint */
        static void calcSegments(StreetJoint* joint, map<string, StreetSegment*> streetSegments, map<string, StreetJoint*> streetJoints);

        /** calculates the important points to a given joint */
        static vector<JointPoints*> calcJoints(StreetJoint* joint, map<string, StreetSegment*> streetSegments, map<string, StreetJoint*> streetJoints);

        /** orders points of joint, so they are at the right position to work with*/
        static void jointOrderSegments(StreetJoint* joint, map<string, StreetSegment*> streetSegments, map<string, StreetJoint*> streetJoints);
};

OSG_END_NAMESPACE;

#endif	/* STREETALGOS_H */
