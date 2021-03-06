#ifndef OSMMAP_H
#define OSMMAP_H

#include "GISFwd.h"
#include "core/math/VRMathFwd.h"
#include "core/math/polygon.h"
#include <string>
#include <map>
#include <vector>
#include <OpenSG/OSGConfig.h>

#include "core/objects/object/VRObjectT.h"

namespace xmlpp { class Element; }
using namespace std;

OSG_BEGIN_NAMESPACE;

class OSMSAXHandlerCP; //copy paste
class OSMSAXHandlerBM; //build map

class OSMSAXParser;

struct OSMBase {
    string id;
    map<string, string> tags;

    OSMBase(string id);
    OSMBase(XMLElementPtr e);
    virtual string toString();
    void writeTo(XMLElementPtr e);

    map<string, string> getTags();
    string getID();
    bool hasTag(const string& t);
};

struct OSMNode : OSMBase {
    double lat = 0;
    double lon = 0;
    vector<string> ways;

    OSMNode(string id, double lat, double lon);
    OSMNode(XMLElementPtr e);
    string toString();
    Vec2d getPosition();
    void writeTo(XMLElementPtr e);
};

struct OSMWay : OSMBase {
    vector<string> nodes;
    VRPolygon polygon;

    OSMWay(string id);
    OSMWay(XMLElementPtr e, map<string, bool>& invalidIDs);
    string toString();
    VRPolygon getPolygon();
    vector<string> getNodes();
    void writeTo(XMLElementPtr e);
};

struct OSMRelation : OSMBase {
    vector<string> ways;
    vector<string> nodes;

    OSMRelation(string id);
    OSMRelation(XMLElementPtr e, map<string, bool>& invalidIDs);
    string toString();
    vector<string> getWays();
    vector<string> getNodes();
    void writeTo(XMLElementPtr e);
};

class OSMMap {
    private:
        string filepath;
        BoundingboxPtr bounds;
        map<string, OSMWayPtr> ways;
        map<string, OSMNodePtr> nodes;
        map<string, OSMRelationPtr> relations;
        map<string, bool> invalidElements;

        bool isValid(XMLElementPtr e);

        void readBounds(XMLElementPtr element);
        void readNode(XMLElementPtr element);
        void readWay(XMLElementPtr element, map<string, bool>& invalidIDs);
        void readRelation(XMLElementPtr element, map<string, bool>& invalidIDs);
        void writeBounds(XMLElementPtr parent);

        int filterFileStreaming(string path, vector<pair<string, string>> whitelist);

    public:
        OSMMap();
        OSMMap(string filepath);

        static OSMMapPtr create();
        static OSMMapPtr create(string filepath);
        static OSMMapPtr loadMap(string filepath);
        static OSMMapPtr parseMap(string filepath);

        void readFile(string path);
        void writeFile(string path);
        int readFileStreaming(string path);
        void filterFileStreaming(string path, vector<vector<string>> wl);

        OSMMapPtr subArea(double latMin, double latMax, double lonMin, double lonMax);
        void clear();
        void reload();

        map<string, OSMWayPtr> getWays();
        map<string, OSMNodePtr> getNodes();
        map<string, OSMRelationPtr> getRelations();
        OSMNodePtr getNode(string id);
        OSMWayPtr getWay(string id);
        OSMRelationPtr getRelation(string id);

        double getMemoryConsumption();

        vector<OSMWayPtr> splitWay(OSMWayPtr way, int segN);
};

OSG_END_NAMESPACE;

#endif // OSMMAP_H
