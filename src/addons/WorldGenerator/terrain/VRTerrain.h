#ifndef VRTERRAIN_H_INCLUDED
#define VRTERRAIN_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include "core/objects/geometry/VRGeometry.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"
#include "addons/WorldGenerator/VRWorldModule.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class Action;

class VRTerrain : public VRGeometry, public VRWorldModule {
    private:
        static string vertexShader;
        static string fragmentShader;
        static string tessControlShader;
        static string tessEvaluationShader;

        Vec2d size = Vec2d(100,100);
        Vec2f texelSize = Vec2f(0.01,0.01); // shader parameter
        float resolution = 1; // shader parameter
        float heightScale = 1; // shader parameter
        double grid = 64;
        VRTexturePtr tex;
        VRMaterialPtr mat;
        shared_ptr<vector<float>> physicsHeightBuffer;

        void updateTexelSize();
        void setupGeo();
        void setupMat();

    public:
        VRTerrain(string name);
        ~VRTerrain();
        static VRTerrainPtr create(string name = "terrain");

        void setSimpleNoise();

        void setParameters( Vec2d size, double resolution, double heightScale );
        void setMap( VRTexturePtr tex, int channel = 3 );
        void loadMap( string path, int channel = 3 );

        virtual bool applyIntersectionAction(Action* ia);

        void physicalize(bool b);

        void projectOSM();

        float getHeight( const Vec2d& p );
        void elevatePoint( Vec3d& p, float offset = 0 );
        void elevatePose( posePtr p, float offset = 0 );
        void elevateObject( VRTransformPtr p, float offset = 0 );
        void projectTangent( Vec3d& t, Vec3d p);

        void paintHeights(string path);
};

OSG_END_NAMESPACE;

#endif // VRTERRAIN_H_INCLUDED
