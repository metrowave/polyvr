#ifndef VRSCENEGRAPHINTERFACE_H_INCLUDED
#define VRSCENEGRAPHINTERFACE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <map>

#include "core/objects/object/VRObject.h"
#include "core/scene/VRSceneFwd.h"
#include "core/networking/VRNetworkingFwd.h"
#include "core/tools/selection/VRSelectionFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRScenegraphInterface : public VRObject {
    private:
        int port = 5555;
        int clientID = 0;

        VRSelectorPtr selector;
        VRSocketPtr socket;
        VRFunction<void*>* cb = 0;

        map<string, VRMaterialPtr> materials;
        map<string, VRObjectPtr> objects;
        map<string, VRGeometryPtr> meshes;
        map<string, VRTransformPtr> transforms;

        void resetWebsocket();
        void ws_callback(void* args);

    public:
        VRScenegraphInterface(string name);
        ~VRScenegraphInterface();

        static VRScenegraphInterfacePtr create(string name = "ScenegraphInterface");
        VRScenegraphInterfacePtr ptr();

        void select(VRObjectPtr obj);
        void clear();
        void setPort(int p);
        void handle(string msg);

        void loadStream(string path);
};

OSG_END_NAMESPACE;

#endif // VRSCENEGRAPHINTERFACE_H_INCLUDED