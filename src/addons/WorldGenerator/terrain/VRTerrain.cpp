#include "VRTerrain.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTexture.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/utils/VRFunction.h"
#include "core/math/boundingbox.h"
#include "core/math/polygon.h"
#include "core/math/triangulator.h"
#include "core/scene/import/GIS/VRGDAL.h"
#include "addons/WorldGenerator/GIS/OSMMap.h"

#include <OpenSG/OSGIntersectAction.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

#define GLSL(shader) #shader

using namespace OSG;

VRTerrain::VRTerrain(string name) : VRGeometry(name) { setupMat(); }
VRTerrain::~VRTerrain() {}
VRTerrainPtr VRTerrain::create(string name) { return VRTerrainPtr( new VRTerrain(name) ); }

void VRTerrain::setParameters( Vec2f s, float r, float h ) {
    size = s;
    resolution = r;
    heightScale = h;
    grid = r*64;
    setupGeo();
    mat->setShaderParameter("resolution", resolution);
    mat->setShaderParameter("heightScale", h);
    updateTexelSize();
}

void VRTerrain::setMap( VRTexturePtr t, int channel ) {
    tex = t;
    mat->setTexture(t);
	mat->setShaderParameter("channel", channel);
    updateTexelSize();
}

void VRTerrain::updateTexelSize() {
    if (!tex) return;
    Vec3i s = tex->getSize();
    texelSize[0] = size[0]/s[0];
    texelSize[1] = size[1]/s[1];
	mat->setShaderParameter("texelSize", texelSize);
}

void VRTerrain::setupGeo() {
    Vec2i gridN = Vec2i(size*1.0/grid);
    if (gridN[0] < 1) gridN[0] = 1;
    if (gridN[1] < 1) gridN[1] = 1;
    Vec2f gridS = size;
    gridS[0] /= gridN[0];
    gridS[1] /= gridN[1];
	Vec2f tcChunk = Vec2f(1.0/gridN[0], 1.0/gridN[1]);

	VRGeoData geo;
    for (int i =0; i < gridN[0]; i++) {
		float px1 = -size[0]*0.5 + i*gridS[0];
		float px2 = px1 + gridS[0];
		float tcx1 = 0 + i*tcChunk[0];
		float tcx2 = tcx1 + tcChunk[0];

		for (int j =0; j < gridN[1]; j++) {
			float py1 = -size[1]*0.5 + j*gridS[1];
			float py2 = py1 + gridS[1];
			float tcy1 = 0 + j*tcChunk[1];
			float tcy2 = tcy1 + tcChunk[1];

			geo.pushVert(Vec3f(px1,0,py1), Vec3f(0,1,0), Vec2f(tcx1,tcy1));
			geo.pushVert(Vec3f(px1,0,py2), Vec3f(0,1,0), Vec2f(tcx1,tcy2));
			geo.pushVert(Vec3f(px2,0,py2), Vec3f(0,1,0), Vec2f(tcx2,tcy2));
			geo.pushVert(Vec3f(px2,0,py1), Vec3f(0,1,0), Vec2f(tcx2,tcy1));
			geo.pushQuad();
		}
	}

	geo.apply(ptr());
	setType(GL_PATCHES);
	setPatchVertices(4);
	setMaterial(mat);
}

void VRTerrain::physicalize(bool b) {
    if (!tex) return;
    auto dim = tex->getSize();

    float Hmax = -1e6;
    physicsHeightBuffer = shared_ptr<vector<float>>( new vector<float>(dim[0]*dim[1]) );
    for (int i = 0; i < dim[0]; i++) {
        for (int j = 0; j < dim[1]; j++) {
            int k = j*dim[0]+i;
            float h = tex->getPixel(Vec3i(i,j,0))[3];
            (*physicsHeightBuffer)[k] = h;
            if (Hmax < h) Hmax = h;
        }
    }

    float R = resolution*0.94; // Hack, there is a scaling error somewhere, either in the shape or the visualisation
    auto shape = new btHeightfieldTerrainShape(dim[0], dim[1], &(*physicsHeightBuffer)[0], 1, -Hmax, Hmax, 1, PHY_FLOAT, false);
    shape->setLocalScaling(btVector3(R,1,R));
    getPhysics()->setCustomShape( shape );
    getPhysics()->setPhysicalized(true);
}

void VRTerrain::setupMat() {
	auto defaultMat = VRMaterial::get("defaultTerrain");
	tex = defaultMat->getTexture();
	if (!tex) {
        Vec4f w = Vec4f(1,1,1,1);
        VRTextureGenerator tg;
        tg.setSize(Vec3i(128,128,1),true);
        tg.add("Perlin", 1.0, w*0.97, w);
        tg.add("Perlin", 1.0/2, w*0.95, w);
        tg.add("Perlin", 1.0/4, w*0.85, w);
        tg.add("Perlin", 1.0/8, w*0.8, w);
        tg.add("Perlin", 1.0/16, w*0.7, w);
        tg.add("Perlin", 1.0/32, w*0.5, w);
        tex = tg.compose(0);
        defaultMat->setTexture(tex);
	}

	mat = VRMaterial::create("terrain");
	mat->setWireFrame(0);
	mat->setLineWidth(1);
	mat->setVertexShader(vertexShader, "terrainVS");
	mat->setFragmentShader(fragmentShader, "terrainFS");
	mat->setTessControlShader(tessControlShader, "terrainTCS");
	mat->setTessEvaluationShader(tessEvaluationShader, "terrainTES");
	mat->setShaderParameter("resolution", resolution);
	mat->setShaderParameter("channel", 3);
    updateTexelSize();
	mat->setShaderParameter("texelSize", texelSize);
	mat->setTexture(tex);
}

bool VRTerrain::applyIntersectionAction(Action* action) {
    if (!mesh || !mesh->geo) return false;

    auto tex = getMaterial()->getTexture();

    auto clamp = [&](float x, float a, float b) {
        return max(min(x,b),a);
    };

    auto toUV = [&](Pnt3f p) -> Vec2f {
        auto mw = getWorldMatrix();
        mw.invert();
        //mw.mult(p,p); // transform point in local coords
		float u = clamp(p[0]/size[0] + 0.5, 0, 1);
		float v = clamp(p[2]/size[1] + 0.5, 0, 1);
		return Vec2f(u,v);
    };

	auto distToSurface = [&](Pnt3f p) -> float {
		Vec2f uv = toUV(p);
		auto c = tex->getPixel(uv);
		return p[1] - c[3];
	};

	if (!VRGeometry::applyIntersectionAction(action)) return false;

    IntersectAction* ia = dynamic_cast<IntersectAction*>(action);

    auto ia_line = ia->getLine();
    Pnt3f p0 = ia_line.getPosition();
    Vec3f d = ia_line.getDirection();
    Pnt3f p = ia->getHitPoint();

    Vec3f norm(0,1,0); // TODO
    int N = 1000;
    float step = 10; // TODO
    int dir = 1;
    for (int i = 0; i < N; i++) {
        p = p - d*step*dir; // walk
        float l = distToSurface(p);
        if (l > 0 && l < 0.03) {
            Real32 t = p0.dist( p );
            ia->setHit(t, ia->getActNode(), 0, norm, -1);
            break;
        } else if (l*dir > 0) { // jump over surface
            dir *= -1;
            step *= 0.5;
        }
    }

    return true;
}

void VRTerrain::loadMap( string path, int channel ) {
    cout << "   ----------- VRTerrain::loadMap " << path << " " << channel << endl ;
    auto tex = loadGeoRasterData(path);
    setMap(tex, channel);
}

void VRTerrain::projectOSM(string path) {
    if (!tex) return;
    if (tex->getChannels() != 4) { // fix mono channels
        VRTextureGenerator tg;
        auto dim = tex->getSize();
        tg.setSize(dim, true);
        auto t = tg.compose(0);
        for (int i = 0; i < dim[0]; i++) {
            for (int j = 0; j < dim[1]; j++) {
                float h = tex->getPixel(Vec3i(i,j,0))[0];
                t->setPixel(Vec3i(i,j,0), Vec4f(1.0,1.0,1.0,h));
            }
        }
        setMap(t);
    }

    /*auto dim = tex->getSize(); // a test
    for (int i = 0; i < dim[0]; i++) {
        for (int j = 0; j < dim[1]; j++) {
            float h = tex->getPixel(Vec3i(i,j,0))[3];
            tex->setPixel(Vec3i(i,j,0), Vec4f(1.0,1.0,0.5,h));
        }
    }*/


    vector<VRPolygonPtr> polygons;
    auto map = OSMMap::loadMap(path);
    for (auto way : map->getWays()) {
        bool isZone = way.second->tags.count("natural");
        if (!isZone) continue;
        auto p = way.second->polygon;

        //auto c = p.getBoundingBox().center();
        Vec3f c(119.806, 0, 29.9976);
        p.translate(-c);
        //p.translate(Vec3f(-sphericalCoordinates[0], 0, -sphericalCoordinates[1]) );
        p.scale(Vec3f(1000, 1, 1000));

        Triangulator tri;
        tri.add(p);
        auto geo = tri.compute();
        geo->setPose(Vec3f(0,0,0), Vec3f(0,-1,0), Vec3f(0,0,1));
        addChild(geo);
        geo->hide();

        auto pp = VRPolygon::create();
        *pp = p;
        polygons.push_back(pp);
    }

    auto dim = tex->getSize();
    VRTextureGenerator tg;
    tg.setSize(dim, true);
    for (auto p : polygons) {
        p->scale( Vec3f(-0.01, 1, 0.01) );
        p->translate(Vec3f(1,0,0.5));
        tg.drawPolygon(p, Vec4f(0,1,0,1));
    }
    VRTexturePtr t = tg.compose(0);

    for (int i = 0; i < dim[0]; i++) {
        for (int j = 0; j < dim[1]; j++) {
            Vec3i pix = Vec3i(i,j,0);
            float h = tex->getPixel(pix)[3];
            Vec4f col = t->getPixel(pix);
            col[3] = h;
            t->setPixel(pix, col);
        }
    }
    setMap(t);
}

void VRTerrain::setPlanet(VRPlanetPtr p, Vec2f position) {
    sphericalCoordinates = position;
    planet = p;
}




// --------------------------------- shader ------------------------------------

string VRTerrain::vertexShader =
"#version 120\n"
GLSL(
attribute vec4 osg_Vertex;
attribute vec2 osg_MultiTexCoord0;

void main(void) {
    gl_TexCoord[0] = vec4(osg_MultiTexCoord0,0.0,0.0);
	gl_Position = osg_Vertex;
}
);

string VRTerrain::fragmentShader =
"#version 400 compatibility\n"
GLSL(
uniform sampler2D tex;
const ivec3 off = ivec3(-1,0,1);
const vec3 light = vec3(-1,-1,-0.5);
uniform vec2 texelSize;

vec3 norm;
vec4 color;

vec3 mixColor(vec3 c1, vec3 c2, float t) {
	t = clamp(t, 0.0, 1.0);
	return mix(c1, c2, t);
}

vec3 getColor() {
	return texture2D(tex, gl_TexCoord[0].xy).rgb;
}

void applyBlinnPhong() {
	norm = normalize( gl_NormalMatrix * norm );
	vec3  light = normalize( gl_LightSource[0].position.xyz );// directional light
	float NdotL = max(dot( norm, light ), 0.0);
	vec4  ambient = gl_LightSource[0].ambient * color;
	vec4  diffuse = gl_LightSource[0].diffuse * NdotL * color;
	float NdotHV = max(dot( norm, normalize(gl_LightSource[0].halfVector.xyz)),0.0);
	vec4  specular = gl_LightSource[0].specular * pow( NdotHV, gl_FrontMaterial.shininess );
	gl_FragColor = ambient + diffuse + specular;
	gl_FragColor[3] = 1.0;
}

vec3 getNormal() {
	vec2 tc = gl_TexCoord[0].xy;
    float s11 = texture(tex, tc).a;
    float s01 = textureOffset(tex, tc, off.xy).a;
    float s21 = textureOffset(tex, tc, off.zy).a;
    float s10 = textureOffset(tex, tc, off.yx).a;
    float s12 = textureOffset(tex, tc, off.yz).a;

    vec2 r2 = 2.0*texelSize;
    vec3 va = normalize(vec3(r2.x,s21-s01,0));
    vec3 vb = normalize(vec3(   0,s12-s10,r2.y));
    vec3 n = cross(vb,va);
	return n;
}

void main( void ) {
	norm = getNormal();
	color = vec4(getColor(),1.0);
	applyBlinnPhong();
}
);

string VRTerrain::tessControlShader =
"#version 400 compatibility\n"
"#extension GL_ARB_tessellation_shader : enable\n"
GLSL(
layout(vertices = 4) out;
out vec3 tcPosition[];
out vec2 tcTexCoords[];
uniform float resolution;
)
"\n#define ID gl_InvocationID\n"
GLSL(
void main() {
    tcPosition[ID] = gl_in[ID].gl_Position.xyz;
    tcTexCoords[ID] = gl_in[ID].gl_TexCoord[0].xy;

    if (ID == 0) {
		vec4 mid = (gl_in[0].gl_Position + gl_in[1].gl_Position + gl_in[2].gl_Position + gl_in[3].gl_Position) * 0.25;
		vec4 pos = gl_ModelViewProjectionMatrix * vec4(mid.xyz, 1.0);
		float D = length(pos.xyz);
		//int p = int(5.0/(resolution*D));
		//int res = int(pow(2,p));
		int res = int(resolution*2048/D); // 32*64
		res = clamp(res, 1, 64);
		if (res >= 64) res = 64; // take closest power of two to avoid jumpy effects
		else if (res >= 32) res = 32;
		else if (res >= 16) res = 16;
		else if (res >= 8) res = 8;
		else if (res >= 4) res = 4;
		else if (res >= 2) res = 2;

        gl_TessLevelInner[0] = res;
        gl_TessLevelInner[1] = res;

        gl_TessLevelOuter[0] = 64;
        gl_TessLevelOuter[1] = 64;
        gl_TessLevelOuter[2] = 64;
        gl_TessLevelOuter[3] = 64;
    }
}
);

string VRTerrain::tessEvaluationShader =
"#version 400 compatibility\n"
"#extension GL_ARB_tessellation_shader : enable\n"
GLSL(
layout( quads ) in;
in vec3 tcPosition[];
in vec2 tcTexCoords[];

uniform float heightScale;
uniform int channel;
uniform sampler2D texture;

void main() {
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec2 ta = mix(tcTexCoords[0], tcTexCoords[1], u);
    vec2 tb = mix(tcTexCoords[3], tcTexCoords[2], u);
    vec2 tc = mix(ta, tb, v);
    gl_TexCoord[0] = vec4(tc.x, tc.y, 1.0, 1.0);

    vec3 a = mix(tcPosition[0], tcPosition[1], u);
    vec3 b = mix(tcPosition[3], tcPosition[2], u);
    vec3 tePosition = mix(a, b, v);
    tePosition.y = heightScale * texture2D(texture, gl_TexCoord[0].xy)[channel];
    gl_Position = gl_ModelViewProjectionMatrix * vec4(tePosition, 1);
}
);
