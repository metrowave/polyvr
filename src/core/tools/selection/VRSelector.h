#ifndef VRSELECTOR_H_INCLUDED
#define VRSELECTOR_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <string>
#include <map>
#include "core/objects/VRObjectFwd.h"
#include "VRSelection.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSelector {
    private:
        Vec3f color;
        int width = 5;
        bool smooth = true;
        struct MatStore {
            VRGeometryWeakPtr geo;
            VRMaterialPtr mat;
            MatStore(VRGeometryPtr geo);
        };
        vector<MatStore> orig_mats;
        VRSelectionPtr selection;
        VRGeometryPtr subselection;

        void deselect();
        VRMaterialPtr getMat();

    public:
        VRSelector();

        void select(VRObjectPtr obj);
        void select(VRSelectionPtr s);
        VRSelectionPtr getSelection();
        void setColor(Vec3f c);
        void setBorder(int width, bool smooth = true);
        void update();
        void clear();
};

OSG_END_NAMESPACE;

#endif // VRSELECTOR_H_INCLUDED