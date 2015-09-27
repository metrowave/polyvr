#include "VRObjectManager.h"
#include "core/objects/VRGroup.h"
#include "core/objects/VRTransform.h"
#include "core/utils/VRFunction.h"

OSG_BEGIN_NAMESPACE;
using namespace std;


void VRObjectManager::addGroup(string group) {
    groups[group] = new list<VRGroupPtr>();
}

void VRObjectManager::updateObjects() {
    //update the Transform changelists
    for ( auto t : VRTransform::changedObjects ) {
        if (auto sp = t.lock()) sp->update();
    }
    VRTransform::changedObjects.clear();

    //update the dynamic objects
    for ( auto t : VRTransform::dynamicObjects ) {
        if (auto sp = t.lock()) sp->update();
        // TODO: else: remove the t from dynamicObjects
    }
}

VRObjectManager::VRObjectManager() {
    updateObjectsFkt = VRFunction<int>::create("ObjectManagerUpdate", boost::bind(&VRObjectManager::updateObjects, this));
}

//GROUPS------------------------

void VRObjectManager::addToGroup(VRGroupPtr obj, string group) {
    if (!groups.count(group)) addGroup(group);
    groups[group]->push_back(obj);
    obj->setGroup(group);
}

list<VRGroupPtr>* VRObjectManager::getGroup(string group) {
    if (groups.count(group))
        return groups[group];
    else return 0;
}

vector<string> VRObjectManager::getGroupList() {
    vector<string> grps;
    for (map<string, list<VRGroupPtr>* >::iterator it = groups.begin(); it != groups.end(); it++) {
        grps.push_back(it->first);
    }
    return grps;
}

OSG_END_NAMESPACE;
