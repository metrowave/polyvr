#include "VRAnnotationEngine.h"

#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRMaterialT.h"
#include "core/objects/material/VRTexture.h"
#include "core/objects/geometry/VRGeoData.h"
#ifndef WITHOUT_PANGO_CAIRO
#include "core/tools/VRText.h"
#endif
#include "core/utils/toString.h"

#define GLSL(shader) #shader

using namespace OSG;

template<> string typeName(const VRAnnotationEngine& t) { return "AnnotationEngine"; }

VRAnnotationEngine::VRAnnotationEngine(string name) : VRGeometry(name) {
    fg = Color4f(0,0,0,1);
    bg = Color4f(1,0,1,0);

    type = "AnnotationEngine";

    mat = VRMaterial::create("AnnEngMat");
    mat->setVertexShader(vp, "annotationVS");
    mat->setFragmentShader(fp, "annotationFS");
    mat->setFragmentShader(dfp, "annotationDFS", true);
    mat->setGeometryShader(gp, "annotationGS");
    mat->setPointSize(5);
    setMaterial(mat);
    updateTexture();

    setSize(0.2);
    setBillboard(false);
    setScreensize(false);
    data = VRGeoData::create();
}

VRAnnotationEnginePtr VRAnnotationEngine::create(string name) { return shared_ptr<VRAnnotationEngine>(new VRAnnotationEngine(name) ); }
VRAnnotationEnginePtr VRAnnotationEngine::ptr() { return static_pointer_cast<VRAnnotationEngine>( shared_from_this() ); }

void VRAnnotationEngine::clear() {
    labels.clear();
    data->reset();
}

void VRAnnotationEngine::setColor(Color4f c) { fg = c; updateTexture(); }
void VRAnnotationEngine::setBackground(Color4f c) { bg = c; updateTexture(); }

bool VRAnnotationEngine::checkUIn(int i) {
    if (i < 0 || i > (int)data->size()) return true;
    return false;
}

void VRAnnotationEngine::resize(Label& l, Vec3d p, int N) {
    int eN = l.entries.size();
    for (int i=0; i<eN; i++) data->setVert(l.entries[i], Vec3d(), Vec3d()); // clear old buffer

    if (N <= eN) return;
    l.entries.resize(N, 0);
    int pN = data->size();

    for (int i=0; i<N-eN; i++) {
        data->pushVert(p, Vec3d());
        data->pushPoint();
        l.entries[eN+i] = pN+i;
    }

    data->apply( ptr() );
}

int VRAnnotationEngine::add(Vec3d p, string s) {
    int i = labels.size();
    set(i,p,s);
    return i;
}

void VRAnnotationEngine::set(int i0, Vec3d p0, string txt) {
#ifndef WITHOUT_PANGO_CAIRO
    auto strings = splitString(txt, '\n');
    int Nlines = strings.size();
    for (int y = 0; y<Nlines; y++) {
        string str = strings[y];
        //cout << "VRAnnotationEngine::set str " << str << endl;
        Vec3d p = p0;
        p[1] -= y*size;
        int i = i0+y;
        if (i < 0) return;
        while (i >= (int)labels.size()) labels.push_back(Label());
        int Ngraphemes = VRText::countGraphemes(str);
        int N = ceil(Ngraphemes/3.0); // number of points, 3 chars per point
        auto& l = labels[i];

        resize(l,p,N + 4); // plus 4 bounding points

        auto graphemes = VRText::splitGraphemes(str);
        for (int j=0; j<N; j++) {
            char c[] = {0,0,0};
            for (int k = 0; k<3; k++) {
                if (j*3+k < (int)graphemes.size()) {
                    string grapheme = graphemes[j*3+k];
                    c[k] = characterIDs[grapheme];
                }
            }
            float f = c[0] + c[1]*256 + c[2]*256*256;
            int k = l.entries[j];
            data->setVert(k, p, Vec3d(f,0,j));
        }

        // bounding points to avoid word clipping
        data->setVert(l.entries[N], p+Vec3d(-0.25*size, -0.5*size, 0), Vec3d(0,0,-1));
        data->setVert(l.entries[N+1], p+Vec3d(-0.25*size,  0.5*size, 0), Vec3d(0,0,-1));
        data->setVert(l.entries[N+2], p+Vec3d((Ngraphemes-0.25)*size, -0.5*size, 0), Vec3d(0,0,-1));
        data->setVert(l.entries[N+3], p+Vec3d((Ngraphemes-0.25)*size,  0.5*size, 0), Vec3d(0,0,-1));
    }
#endif
}

void VRAnnotationEngine::setSize(float f) { mat->setShaderParameter("size", Real32(f)); size = f; }
void VRAnnotationEngine::setBillboard(bool b) { mat->setShaderParameter("doBillboard", Real32(b)); }
void VRAnnotationEngine::setScreensize(bool b) { mat->setShaderParameter("screen_size", Real32(b)); }

void VRAnnotationEngine::setOrientation(Vec3d d, Vec3d u) {
    mat->setShaderParameter("orientationDir", Vec3f(d));
    mat->setShaderParameter("orientationUp", Vec3f(u));
}

void VRAnnotationEngine::updateTexture() {
#ifndef WITHOUT_PANGO_CAIRO
    string txt;
    for (int i=32; i<127; i++) txt += char(i);
    txt += "ÄÜÖäüöß€°^";
    int cN = VRText::countGraphemes(txt);
    int padding = 3;
    auto img = VRText::get()->create(txt, "MONO 20", 20, fg, bg);
    float tW = img->getSize()[0];
    float lW = VRText::get()->layoutWidth;
    float tp = padding / tW;
    float cSize = lW/tW / cN;
    mat->setTexture(img);
    mat->setShaderParameter("texPadding", Real32(tp)); // tested
    mat->setShaderParameter("charTexSize", Real32(cSize));
    //img->write("annChars.png");

    int i=1; // 0 is used for invalid/no char
    for (auto c : VRText::splitGraphemes(txt)) {
        characterIDs[c] = i;
        i++;
    }
#endif
}

string VRAnnotationEngine::vp =
"#version 120\n"
GLSL(
varying vec4 vertex;
varying vec3 normal;
varying mat4 MVP;
varying vec2 texCoord;

attribute vec4 osg_Vertex;
attribute vec4 osg_Normal;

void main( void ) {
    vertex = osg_Vertex;
    gl_Position = gl_ModelViewProjectionMatrix*osg_Vertex;
    normal = osg_Normal.xyz;
    MVP = gl_ModelViewProjectionMatrix;
    texCoord = vec2(0,0);
}
);

string VRAnnotationEngine::fp =
"#version 120\n"
GLSL(
uniform sampler2D texture;

varying vec2 texCoord;

void main( void ) {
    //gl_FragColor = vec4(texCoord.x,texCoord.y,0.0,1.0);
    gl_FragColor = texture2D(texture, texCoord);
}
);

string VRAnnotationEngine::dfp =
"#version 120\n"
GLSL(
uniform sampler2D texture;

varying vec4 geomPos;
varying vec3 geomNorm;
varying vec2 texCoord;

void main( void ) {
    vec3 pos = geomPos.xyz / geomPos.w;
    vec3 diffCol = texture2D(texture, texCoord).rgb;
    gl_FragData[0] = vec4(pos, 1);
    gl_FragData[1] = vec4(normalize(geomNorm), 0);
    gl_FragData[2] = vec4(diffCol, 0);
}
);

string VRAnnotationEngine::gp =
"#version 150\n"
"#extension GL_EXT_geometry_shader4 : enable\n"
GLSL(
layout (points) in;
layout (triangle_strip, max_vertices=60) out;

uniform float doBillboard;
uniform float screen_size;
uniform vec3 orientationDir;
uniform vec3 orientationUp;
uniform float size;
uniform float texPadding;
uniform float charTexSize;
uniform vec2 OSGViewportSize;
in vec4 vertex[];
in vec3 normal[];
in mat4 MVP[];
out vec2 texCoord;
out vec4 geomPos;
out vec3 geomNorm;

vec3 orientationX;

vec4 transform(in float x, in float y) {
    vec3 p = -orientationX*x + orientationUp*y;
    return vec4(p, 0);
}

void emitVertex(in vec4 p, in vec2 tc, in vec4 v) {
    gl_Position = p;
    texCoord = tc;
    geomPos = v;
    geomNorm = vec3(0,0,1);
    EmitVertex();
}

void emitQuad(in float offset, in vec4 tc) {
    float sx = 0.5*size;
    float sy = size;
    float ox = 2*sx*offset;
    vec4 p1;
    vec4 p2;
    vec4 p3;
    vec4 p4;
    vec4 v1;
    vec4 v2;
    vec4 v3;
    vec4 v4;
    vec4 p = gl_PositionIn[0];

    if (screen_size > 0.5) {
        p.xyz = p.xyz/p.w;
        p.w = 1;
    }

    if (doBillboard < 0.5) {
        p1 = p+MVP[0]*transform(-sx+ox,-sy);
        p2 = p+MVP[0]*transform(-sx+ox, sy);
        p3 = p+MVP[0]*transform( sx+ox, sy);
        p4 = p+MVP[0]*transform( sx+ox,-sy);
    } else {
        float a = OSGViewportSize.y/OSGViewportSize.x;
        p1 = p+transform(-sx*a+ox*a,-sy);
        p2 = p+transform(-sx*a+ox*a, sy);
        p3 = p+transform( sx*a+ox*a, sy);
        p4 = p+transform( sx*a+ox*a,-sy);
        v1 = vertex[0]+transform(-sx*a+ox*a,-sy);
        v2 = vertex[0]+transform(-sx*a+ox*a, sy);
        v3 = vertex[0]+transform( sx*a+ox*a, sy);
        v4 = vertex[0]+transform( sx*a+ox*a,-sy);
    }

    emitVertex(p1, vec2(tc[0], tc[2]), v1);
    emitVertex(p2, vec2(tc[0], tc[3]), v2);
    emitVertex(p3, vec2(tc[1], tc[3]), v3);
    EndPrimitive();
    emitVertex(p1, vec2(tc[0], tc[2]), v1);
    emitVertex(p3, vec2(tc[1], tc[3]), v3);
    emitVertex(p4, vec2(tc[1], tc[2]), v4);
    EndPrimitive();
}

void emitChar(in int d, in float p) {
    float padding = texPadding; // 0.001 texture padding
    float f = charTexSize; // 0.00832 character texture size
    d -= 1; // offset
    if (d >= 0) emitQuad(p, vec4(padding+d*f, padding+(d+1)*f, 0, 1));
}

void emitString(in float str, in float offset) {
    int stri = int(str);
    int c0 = stri;
    int c1 = c0/256;
    int c2 = c1/256;
    c0 = c0%256;
    c1 = c1%256;
    c2 = c2%256;
    if (c0 > 0) emitChar(c0, 3*offset);
    if (c1 > 0) emitChar(c1, 3*offset + 1);
    if (c2 > 0) emitChar(c2, 3*offset + 2);
}

void main() {
    orientationX = cross(orientationDir, orientationUp);
    float str = normal[0][0];
    float offset = normal[0][2];
    if (offset >= 0) emitString(str, offset);
}
);








