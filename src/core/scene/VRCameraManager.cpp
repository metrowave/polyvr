#include "VRCameraManager.h"
#include "../objects/VRCamera.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRCameraManager::VRCameraManager() {
    cout << "Init VRCameraManager\n";
}

VRCameraManager::~VRCameraManager() {;}

VRTransformPtr VRCameraManager::addCamera(string name) {
    VRCameraPtr c = VRCamera::create(name);
    setMActiveCamera(c->getName());
    return c;
}

VRCameraPtr VRCameraManager::getCamera(int ID) {
    int i=0;
    for (auto c : VRCamera::getAll()) { if (i == ID) return c; i++; }
    return 0;
}

void VRCameraManager::setMActiveCamera(string cam) {
    for (auto c : VRCamera::getAll()) { if (c->getName() == cam) active = c; }
}

VRCameraPtr VRCameraManager::getActiveCamera() {
    return active;
}

int VRCameraManager::getActiveCameraIndex() {
    int i=0;
    for (auto c : VRCamera::getAll()) { if (c == active) return i; i++; }
    return -1;
}

vector<string> VRCameraManager::getCameraNames() { vector<string> res; for(auto c : VRCamera::getAll()) res.push_back(c->getName()); return res; }

OSG_END_NAMESPACE;
