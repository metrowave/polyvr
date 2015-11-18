#ifndef VRSTEP_H_INCLUDED
#define VRSTEP_H_INCLUDED

class STEPfile;
class Registry;
class InstMgr;
class SDAI_Application_instance;
typedef SDAI_Application_instance STEPentity;
class STEPaggregate;
class SDAI_Select;
class STEPattribute;

#include <memory>
#include <string>
#include <map>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGLine.h>
#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/math/pose.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSTEP {
    private: // TODO: use ints (data IDs) instead of data directly
        struct Circle {
            pose p;
            double r;
        };

        struct Cylinder {
            pose p;
            double r1, r2, h;
        };

        struct Box {
            pose p;
            double X, Y, Z;
        };

        struct EdgeCurve {
            Vec3f start, end;
            Circle c;
            Line l;
        };

        struct OrientedEdge {
            EdgeCurve ec;
            bool dir;
        };

    public:
        typedef shared_ptr<Registry> RegistryPtr;
        typedef shared_ptr<InstMgr> InstMgrPtr;
        typedef shared_ptr<STEPfile> STEPfilePtr;

        struct Type {
            string path;
            shared_ptr< VRFunction<STEPentity*> > cb;
        };

    private:
        RegistryPtr registry;
        InstMgrPtr instances;
        STEPfilePtr sfile;

        map<string, int> histogram;

        string redBeg  = "\033[0;38;2;255;150;150m";
        string colEnd = "\033[0m";

        map<int, Vec3f> resVec3f;
        map<int, pose> resPose;
        map<int, Circle> resCircle;
        map<int, EdgeCurve> resEdgeCurve;
        map<int, OrientedEdge> resOrientedEdge;
        map<int, VRGeometryPtr> resObject;
        bool parseVector(STEPentity* se);
        bool parsePose(STEPentity* se);
        bool parseCircle(STEPentity* se);
        bool parseEdgeCurve(STEPentity* se);
        bool parseOrientedEdge(STEPentity* se);
        bool parseAdvancedBrepShapeRepresentation(STEPentity* se);

        template<class T> T query(STEPentity* e, string path);
        template<class V, class T> V queryVec(STEPentity* se, string paths);

        map<string, Type> types;
        void addType(string type, string path);
        void parse(STEPentity* e, string path);

        void loadT(string file, STEPfilePtr sfile, bool* done);
        void open(string file);
        string indent(int lvl);
        vector<STEPentity*> getAggregateEntities(STEPattribute* attr);

        void traverseEntity(STEPentity* se, int lvl);
        void traverseSelect(SDAI_Select* s, int lvl, STEPattribute* attr);
        void traverseAggregate(STEPaggregate* sa, int type, int lvl);
        void build();

    public:
        VRSTEP();

        VRTransformPtr load(string file);
};

OSG_END_NAMESPACE;


#endif // VRSTEP_H_INCLUDED