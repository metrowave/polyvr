#include "VRPointCloud.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/VRLod.h"
#include "core/math/Octree.h"
#include "core/utils/toString.h"

using namespace OSG;
using namespace std;

VRPointCloud::VRPointCloud(string name) : VRTransform(name) {
    octree = Octree::create(10);
    mat = VRMaterial::create("pcmat");
}

VRPointCloud::~VRPointCloud() {}

VRPointCloudPtr VRPointCloud::create(string name) { return VRPointCloudPtr( new VRPointCloud(name) ); }

void VRPointCloud::setupMaterial(bool lit, int pointsize) {
    mat->setLit(lit);
    mat->setPointSize(pointsize);
}

VRMaterialPtr VRPointCloud::getMaterial() { return mat; }
OctreePtr VRPointCloud::getOctree() { return octree; }

void VRPointCloud::addLevel(float distance, int downsampling) {
    levels++;
    downsamplingRate.push_back(downsampling);
    lodDistances.push_back(distance);
}

void VRPointCloud::setupLODs() {
    for (auto leaf : octree->getAllLeafs()) {
        Vec3d center = leaf->getCenter();

        auto lod = VRLod::create("chunk");
        lod->setCenter(center);
        addChild(lod);

        for (int lvl=0; lvl<levels; lvl++) {
            auto geo = VRGeometry::create("lvl"+toString(lvl+1));
            geo->setMaterial(mat);
            geo->setFrom(center);
            lod->addChild(geo);
            if (lvl > 0) lod->addDistance(lodDistances[lvl-1]);

            VRGeoData chunk;
            for (int i = 0; i < leaf->dataSize(); i+=downsamplingRate[lvl]) {
                void* data = leaf->getData(i);
                Vec3d pos = leaf->getPoint(i);
                Color3f col = *((Color3f*)data);
                chunk.pushVert(pos - center, Vec3d(0,1,0), col);
                chunk.pushPoint();
            }
            if (chunk.size() > 0) chunk.apply( geo );
        }

        leaf->delContent<Color3f>();
    }

    //addChild(octree->getVisualization());
}
