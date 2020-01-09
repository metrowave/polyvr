#include "GLTF.h"
#include "core/utils/VRProgress.h"
#include "core/utils/toString.h"
#include "core/math/pose.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/sprite/VRSprite.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTexture.h"
#include "core/objects/VRLight.h"
#include "core/objects/VRLightBeacon.h"
#include "core/objects/VRCamera.h"

#include "core/scene/VRScene.h"

#include <string>
#include <iostream>
#include <fstream>
#include <stack>

#include <OpenSG/OSGColor.h>
#include <OpenSG/OSGQuaternion.h>

// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "tiny_gltf.h"

using namespace OSG;

struct GLTFSchema {
    int version = 1;

    struct FieldRef {
        string type;
        string def;
    };

    struct NodeRef {
        map<string, FieldRef> fieldRefs;
    };

    map<string, NodeRef> nodeRefs;

    void addNodeRef(string node, vector<string> fields, vector<string> types, vector<string> defaults) {
        nodeRefs[node] = NodeRef();
        for (uint i=0; i<fields.size(); i++) {
            nodeRefs[node].fieldRefs[fields[i]] = FieldRef();
            nodeRefs[node].fieldRefs[fields[i]].type = types[i];
            nodeRefs[node].fieldRefs[fields[i]].def  = defaults[i];
        }
    }

    GLTFSchema(int version = 1) : version(version) {
        if (version == 1) { // TODO
        }

        if (version == 2) {
            addNodeRef("extensionsUsed", {"array"}, {"string"}, {""});
            addNodeRef("extensionsRequired", {"array"}, {"string"}, {""});
            addNodeRef("accessors", {"array"}, {"assessor"}, {"NULL"});
            addNodeRef("animations", {"array"}, {"animation"}, {"NULL"});
            addNodeRef("asset", {"array"}, {"asset"}, {"NULL"});
            addNodeRef("buffers", {"array"}, {"buffer"}, {"NULL"});
            addNodeRef("bufferViews", {"array"}, {"bufferView"}, {"NULL"});
            addNodeRef("cameras", {"array"}, {"camera"}, {"NULL"});
            addNodeRef("images", {"array"}, {"image"}, {"NULL"});
            addNodeRef("materials", {"array"}, {"material"}, {"NULL"});
            addNodeRef("meshes", {"array"}, {"mesh"}, {"NULL"});
            addNodeRef("nodes", {"array"}, {"node"}, {"NULL"});
            addNodeRef("samplers", {"array"}, {"sampler"}, {"NULL"});
            addNodeRef("scene", {"glTFid"}, {"int"}, {"NULL"});
            addNodeRef("scenes", {"array"}, {"scene"}, {"NULL"});
            addNodeRef("skins", {"array"}, {"skin"}, {"NULL"});
            addNodeRef("textures", {"array"}, {"texture"}, {"NULL"});

            addNodeRef("Material", {"name","extensions","extras","pbrMetallicRoughness","normalTexture","emissiveTexture", "emissiveFactor", "alphaMode", "alphaCutoff", "doubleSided" }, {"string","","","","","", "MFnumber", "SFstring", "SFnumber", "SFboolean" }, {"","","","","","", "[ 0.0, 0.0, 0.0 ]", "OPAQUE", "0.5", "false"});
            //alphaMode: OPAQUE, MASK, BLEND
        }
    }

    bool isNode(string& token) { return nodeRefs.count(token); }

    bool isFieldOf(string& node, string& token) {
        if (!isNode(node)) return false;
        return nodeRefs[node].fieldRefs.count(token);
    }

    FieldRef& getField(string& node, string& field) {
        return nodeRefs[node].fieldRefs[field];
    }
};

GLTFSchema gltfschema;


struct GLTFUtils {
    int version = 1;

    bool isGroupNode(string node) {
        if (version == 1) {
            if (node == "Group") return true;
            if (node == "Separator") return true;
            if (node == "Switch") return true;
            if (node == "TransformSeparator") return true;
        }

        if (version == 2) {
            //if (node == "children") return true;
        }

        return false;
    }

    bool isGeometryNode(string node) {
        if (node == "Mesh") return true;

        if (node == "IndexedFaceSet") return true;
        if (node == "IndexedLineSet") return true;
        if (node == "PointSet") return true;
        if (node == "Cone") return true;
        if (node == "Sphere") return true;
        if (node == "Cylinder") return true;

        if (version == 1) {
            if (node == "Cube") return true;
            if (node == "AsciiText") return true;
        }

        if (version == 2) {
            if (node == "Box") return true;
            if (node == "ElevationGrid") return true;
            if (node == "Extrusion") return true;
            if (node == "Text") return true;
        }

        return false;
    }

    bool isPropertyNode(string node) {
        if (node == "DirectionalLight") return true;
        if (node == "PointLight") return true;
        if (node == "SpotLight") return true;
        if (node == "Normal") return true;
        if (node == "FontStyle") return true;
        if (node == "Material") return true;
        if (node == "LOD") return true;
        if (node == "Transform") return true;

        if (version == 1) {
            if (node == "Coordinate3") return true;
            if (node == "Info") return true;
            if (node == "MaterialBinding") return true;
            if (node == "NormalBinding") return true;
            if (node == "Texture2") return true;
            if (node == "Texture2Transform") return true;
            if (node == "TextureCoordinate2") return true;
            if (node == "ShapeHints") return true;
            if (node == "MatrixTransform") return true;
            if (node == "Rotation") return true;
            if (node == "Scale") return true;
            if (node == "Translation") return true;
            if (node == "OrthographicCamera") return true;
            if (node == "PerspectiveCamera") return true;
        }

        if (version == 2) {
            if (node == "Group") return true;
            if (node == "Anchor") return true;
            if (node == "Billboard") return true;
            if (node == "Collision") return true;
            if (node == "Inline") return true;
            if (node == "Switch") return true;
            if (node == "AudioClip") return true;
            if (node == "Script") return true;
            if (node == "Shape") return true;
            if (node == "Sound") return true;
            if (node == "WorldInfo") return true;
            if (node == "Color") return true;
            if (node == "Coordinate") return true;
            if (node == "TextureCoordinate") return true;
            if (node == "Appearance") return true;
            if (node == "ImageTexture") return true;
            if (node == "MovieTexture") return true;
            if (node == "PixelTexture") return true;
            if (node == "TextureTransform") return true;
        }

        return false;
    }

    bool isOtherNode(string node) {
        if (version == 2) {
            if (node == "CylinderSensor") return true;
            if (node == "PlaneSensor") return true;
            if (node == "ProximitySensor") return true;
            if (node == "SphereSensor") return true;
            if (node == "TimeSensor") return true;
            if (node == "TouchSensor") return true;
            if (node == "VisibilitySensor") return true;
            if (node == "ColorInterpolator") return true;
            if (node == "CoordinateInterpolator") return true;
            if (node == "NormalInterpolator") return true;
            if (node == "OrientationInterpolator") return true;
            if (node == "PositionInterpolator") return true;
            if (node == "ScalarInterpolator") return true;
            if (node == "Background") return true;
            if (node == "Fog") return true;
            if (node == "NavigationInfo") return true;
            if (node == "Viewpoint") return true;
        }
        return false;
    }

    bool isNode(string node) {
        if (isGroupNode(node)) return true;
        if (isGeometryNode(node)) return true;
        if (isPropertyNode(node)) return true;
        if (isOtherNode(node)) return true;
        return false;
    }

    bool isTransformationNode(string node) {
        if (node == "MatrixTransform") return true;
        if (node == "Rotation") return true;
        if (node == "Scale") return true;
        if (node == "Transform") return true;
        if (node == "Translation") return true;

        return false;
    }

    bool isTransformationResetNode(string node) {
        if (version == 1) {
            if (node == "Separator") return true;
            if (node == "TransformSeparator") return true;
        }

        if (version == 2) {}

        return false;
    }

};

struct GLTFNode : GLTFUtils {
    string name;
    string type;
    GLTFNode* parent = 0;
    vector<GLTFNode*> children;
    vector<Pnt3d> positions;
    vector<Vec3d> normals;
    vector<Color3f> colors;
    vector<Vec2d> texCoords;
    vector<int> coordIndex;
    vector<int> normalIndex;
    vector<int> colorIndex;
    vector<int> texCoordIndex;
    VRObjectPtr obj;
    VRMaterialPtr material;
    Matrix4d pose;
    VRGeoData geoData;
    int matID = -1;

    Vec3d translation = Vec3d(0,0,0);
    Vec4d rotation = Vec4d(0,0,1,0);
    Vec3d scale = Vec3d(1,1,1);
    Matrix4d matTransform  = Matrix4d();


    GLTFNode(string t, string n = "Unnamed") : name(n), type(t) {}
    virtual ~GLTFNode() {
        for (auto c : children) delete c;
    }

    void addChild(GLTFNode* c) {
        c->parent = this;
        children.push_back(c);
    }

    virtual GLTFNode* newChild(string t, string n) = 0;

    vector<GLTFNode*> getSiblings() {
        vector<GLTFNode*> res;
        if (!parent) return res;
        for (auto c : parent->children) {
            if (c != this) res.push_back(c);
        }
        return res;
    }

    void print(string padding = "") {
        cout << padding << "Node '" << name << "' of type " << type << endl;
        for (auto c : children) c->print(padding+" ");
    }


    // build OSG ---------------------
    VRObjectPtr makeObject() {
        //cout << "make object '" << name << "' of type " << type << endl;
        if (isGroupNode(type)) return VRObject::create(name);
        if (isGeometryNode(type)) {
            if (type == "AsciiText") return VRSprite::create(name);
            return VRGeometry::create(name);
        }
        if (isPropertyNode(type)) {
            if (type == "PointLight") return VRLight::create(name);
            if (type == "DirectionalLight") return VRLight::create(name);
            if (type == "SpotLight") return VRLight::create(name);
            if (type == "PerspectiveCamera") return VRCamera::create(name, false);
        }

        if (isTransformationNode(type)) return VRTransform::create(name);

        if (type == "Coordinate") return 0;
        if (type == "Normal") return 0;
        if (type == "Color") return 0;
        if (type == "Material") return 0;
        if (type == "Appearance") return 0;

        return VRObject::create(name);
    }

    void applyPose() {
        VRTransformPtr t = dynamic_pointer_cast<VRTransform>(obj);
        t->setMatrix(pose);
    }

    void applyMaterial() {
        VRGeometryPtr g = dynamic_pointer_cast<VRGeometry>(obj);
        if (g && material) g->setMaterial(material);
    }

    // transformation data
    void handleTranslation() {
        pose.setTranslate( translation );
    }

    void handleRotation() {
        //Vec4d rotation = getSFRotation(data, "rotation", Vec4d(0,0,1,0));
        pose.setRotate( Quaterniond( Vec3d(rotation[0], rotation[1], rotation[2]), rotation[3] ) );
    }

    void handleScale() {
        pose.setScale( scale );
    }

    void handleTransform() {
        Vec3d center = Vec3d(0,0,0);
        Vec4d scaleOrientation = Vec4d(0,0,1,0);
        Matrix4d m1; m1.setTranslate(translation+center); m1.setRotate( Quaterniond( Vec3d(rotation[0], rotation[1], rotation[2]), rotation[3] ) );
        Matrix4d m2; m2.setRotate( Quaterniond( Vec3d(scaleOrientation[0], scaleOrientation[1], scaleOrientation[2]), scaleOrientation[3] ) );
        Matrix4d m3; m3.setScale(scale);
        Matrix4d m4; m4.setTranslate(-center); m4.setRotate( Quaterniond( Vec3d(scaleOrientation[0], scaleOrientation[1], scaleOrientation[2]), -scaleOrientation[3] ) );
        Matrix4d M = m1; M.mult(m2); M.mult(m3); M.mult(m4);
        pose = M;
    }

    void buildOSG() {
        if (!obj) {
            obj = makeObject();
            if (obj) {
                if (parent && parent->obj) parent->obj->addChild(obj);
                else cout << "WARNING in GLTFNode::buildOSG, cannot append object to parent!" << endl;
            }
            //applyProperties();
        }

        for (auto c : children) c->buildOSG();
    }

    virtual Matrix4d applyTransformations(Matrix4d m = Matrix4d()) = 0;
    virtual VRMaterialPtr applyMaterials() = 0;
    virtual VRGeoData applyGeometries() = 0;

    void resolveLinks(map<string, GLTFNode*>& references) {
        if (type == "Link") obj->getParent()->addChild(references[name]->obj->duplicate());
        for (auto c : children) c->resolveLinks(references);
    }
};

struct GLTFNNode : GLTFNode{
    GLTFNNode(string type, string name = "Unnamed") : GLTFNode(type, name) { version = 2; }
    ~GLTFNNode() {}

    GLTFNNode* newChild(string t, string n) {
        //cout << "GLTF2Node::newChild '" << n << "' of type " << t << endl;
        auto c = new GLTFNNode(t,n);
        addChild(c);
        return c;
    }

    Matrix4d applyTransformations(Matrix4d m = Matrix4d()) {
        if (isTransformationNode(type)) {
            VRTransformPtr t = dynamic_pointer_cast<VRTransform>(obj); ///AGRAJAG
            if (t) t->setMatrix(pose);
        }

        for (auto c : children) c->applyTransformations(m);
        return m;
    }

    VRMaterialPtr applyMaterials() {
        if (isGeometryNode(type)) {
            //applyMaterial();
            VRGeometryPtr g = dynamic_pointer_cast<VRGeometry>(obj);
            if (g) {
                if (material) {
                    g->setMaterial(material);
                    cout << "mat set " << matID << " on: " << name << endl;
                }
            }
        }

        for (auto c : children) c->applyMaterials();
        return material;
    }

    VRGeoData applyGeometries() {
        if (type == "Mesh") {
            VRGeometryPtr g = dynamic_pointer_cast<VRGeometry>(obj);
            if (g) {
                geoData.apply(g);
            }
        }

        for (auto c : children) c->applyGeometries();
        return geoData;
    }
};

class GLTFLoader : public GLTFUtils {
    private:
        string path;
        VRTransformPtr res;
        VRProgressPtr progress;
        bool threaded = false;
        GLTFNode* tree = 0;
        map<string, GLTFNode*> references;
        tinygltf::Model model;
        map<int, GLTFNode*> nodes;
        map<int, int> nodeToMesh;
        map<int, int> meshToNode;
        map<int, vector<int>> childrenPerNode;
        map<int, VRMaterialPtr> materials;
        map<int, VRTexturePtr> textures;
        size_t sceneID = -1;
        size_t nodeID = -1;
        size_t meshID = -1;
        size_t matID = -1;
        size_t texID = -1;

        static string GetFilePathExtension(const string &FileName) {
            if (FileName.find_last_of(".") != std::string::npos)
                return FileName.substr(FileName.find_last_of(".") + 1);
            return "";
        }

        bool openFile(ifstream& file) {
            file.open(path);
            if (!file.is_open()) { cout << "ERROR: file '" << path << "' not found!" << endl; return false; }
            return true;
        }

        size_t getFileSize(ifstream& file) {
            file.seekg(0, ios_base::end);
            size_t fileSize = file.tellg();
            file.seekg(0, ios_base::beg);
            return fileSize;
        }

        void handleScene(const tinygltf::Scene &gltfScene){
            sceneID++;
            string res = "";

            res += to_string(sceneID) + " " + gltfScene.name + " nodes: ";
            for (auto each : gltfScene.nodes) res += " " + each;

            //cout << res << endl;
        }

        void handleNode(const tinygltf::Node &gltfNode){
            nodeID++;
            string res = "";
            string type = "Untyped";
            string name = "Unnamed";
            Matrix4d mat4;
            mat4.setTranslate(Vec3d(0,0,0));
            Vec3d translation = Vec3d(0,0,0);
            Vec4d rotation = Vec4d(0,0,1,0);
            Vec3d scale = Vec3d(1,1,1);

            res += to_string(nodeID) + " " + gltfNode.name;
            name = gltfNode.name;

            if (gltfNode.mesh > -1) {
                //res += " mesh: " + to_string(gltfNode.mesh);
                nodeToMesh[nodeID] = gltfNode.mesh;
                meshToNode[gltfNode.mesh] = nodeID;
                //res += " -------------------";
                type = "Mesh";
            }

            if (gltfNode.rotation.size() == 4) {
                auto v = gltfNode.rotation;
                rotation = Vec4d( v[0], v[1], v[2], v[3] );
                res += " rotation: found";
                type = "Rotation";
            }

            if (gltfNode.scale.size() == 3) {
                auto v = gltfNode.scale;
                scale = Vec3d( v[0], v[1], v[2] );
                res += " scale: found";
                type = "Scale";
            }

            if (gltfNode.translation.size() == 3) {
                auto v = gltfNode.translation;
                translation = Vec3d( v[0], v[1], v[2] );
                res += " scale: found";
                type = "Translation";
            }

            if (gltfNode.matrix.size() == 16) {
                auto v = gltfNode.matrix;
                mat4 = Matrix4d( v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10], v[11], v[12], v[13], v[14], v[15] );
                res += " matrix:";
                for (auto each: gltfNode.matrix) res += " " + to_string(each);
                res += " found";
                type = "Transform";
            }

            if (name == "") name = "NN";
            GLTFNode* thisNode = new GLTFNNode(type,name);
            nodes[nodeID] = thisNode;

            if (gltfNode.children.size() > 0) {
                res += " children:";
                vector<int> children;
                for (auto each : gltfNode.children) { children.push_back(each); res += " " + to_string(each); }
                childrenPerNode[nodeID] = children;
            }

            if (isTransformationNode(type)) {
                thisNode->rotation = rotation;
                thisNode->translation = translation;
                thisNode->scale = scale;
                if (type == "Transform") thisNode->pose = mat4;
                else thisNode->handleTransform();
                //cout << res << endl;
            }

            //if (type == "Untyped") cout << res << " " << type << endl;
            //if (type == "Mesh") cout << res << " " << type << endl;
        }

        void handleMaterial(const tinygltf::Material &gltfMaterial){
            matID++;
            //cout << "Emmissive Tex: " << gltfMaterial.emissiveTexture.index << endl;
            //cout << "Oclusion Tex: " << gltfMaterial.occlusionTexture.index << endl;
            VRMaterialPtr mat = VRMaterial::create(gltfMaterial.name);
            cout << matID <<  " " << gltfMaterial.name << endl;
            bool bsF = false;
            bool mtF = false;
            bool rfF = false;
            bool emF = false;
            for (const auto &content : gltfMaterial.values) {
                cout << " " << content.first;
                if (content.first == "baseColorTexture") {
                    int tID = gltfMaterial.pbrMetallicRoughness.baseColorTexture.index;
                    //cout << matID << " " << gltfMaterial.name << " BaseColor Tex: " << tID << endl;
                    mat->setTexture(textures[tID]);
                    //cout << "  " << matID << " " << gltfMaterial.name << " baseColor: " << tID << " - " << textures[tID]->getSize() << endl;
                }

                if (content.first == "metallicRoughnessTexture") {
                    int tID = gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;
                    //cout << "  " << matID << " " << gltfMaterial.name << " metallicRoughnessTexture: " << tID << " found but not used"<<endl;
                }
                if (content.first == "baseColorFactor") bsF = true;
                if (content.first == "metallicFactor") mtF = true;
                if (content.first == "roughnessFactor") rfF = true;
                if (content.first == "emissiveFactor") emF = true;
            }
            cout << endl;
            cout << " - ";
            for (const auto &content : gltfMaterial.additionalValues) {
                cout << " " << content.first;
                if (content.first == "normalTexture") {
                    int tID = gltfMaterial.normalTexture.index;
                    //cout << "  " << matID << " " << gltfMaterial.name << " Normal Tex: " << gltfMaterial.normalTexture.index << " found but not used" << endl;
                }

                if (content.first == "emissiveTexture") {
                    int tID = gltfMaterial.emissiveTexture.index;
                    //cout << "  " << matID << " " << gltfMaterial.name << " Emmissive Tex: " << gltfMaterial.emissiveTexture.index << " found but not used" << endl;
                }
            }
            cout << endl;

            if (bsF && mtF && rfF) {
                if (gltfMaterial.pbrMetallicRoughness.baseColorFactor.size() == 3) {
                    Color3f baseColor = Color3f(gltfMaterial.pbrMetallicRoughness.baseColorFactor[0],gltfMaterial.pbrMetallicRoughness.baseColorFactor[1],gltfMaterial.pbrMetallicRoughness.baseColorFactor[2]);
                    double metallicFactor = gltfMaterial.pbrMetallicRoughness.metallicFactor;
                    double roughnessFactor = gltfMaterial.pbrMetallicRoughness.roughnessFactor;

                    Color3f spec = Color3f(0.04,0.04,0.04)*(1.0-metallicFactor) + baseColor*metallicFactor;
                    Color3f diff = baseColor * (1.0 - metallicFactor);
                    float shiny = 1.0 - roughnessFactor;
                    mat->setSpecular(spec);
                    mat->setAmbient(diff);
                    mat->setShininess(shiny);
                    mat->ignoreMeshColors(true);
                    cout << " newMAT - 3 " << baseColor << endl;
                    if (emF) {
                        //gltfMaterial.pbrMetallicRoughness.
                        //mat->setEmission(Color3f c);
                    }
                }
                if (gltfMaterial.pbrMetallicRoughness.baseColorFactor.size() == 4) {
                    Color4f baseColor = Color4f(gltfMaterial.pbrMetallicRoughness.baseColorFactor[0],gltfMaterial.pbrMetallicRoughness.baseColorFactor[1],gltfMaterial.pbrMetallicRoughness.baseColorFactor[2],gltfMaterial.pbrMetallicRoughness.baseColorFactor[3]);
                    double metallicFactor = gltfMaterial.pbrMetallicRoughness.metallicFactor;
                    double roughnessFactor = gltfMaterial.pbrMetallicRoughness.roughnessFactor;

                    Color4f spec = Color4f(0.04,0.04,0.04,1)*(1.0-metallicFactor) + baseColor*metallicFactor;
                    Color4f diff = baseColor * (1.0 - metallicFactor);
                    float shiny = 1.0 - roughnessFactor;
                    mat->setSpecular( Color3f(spec[0],spec[1],spec[2]) );
                    mat->setAmbient( Color3f(diff[0],diff[1],diff[2]) );
                    mat->setShininess(shiny);
                    mat->ignoreMeshColors(true);
                    cout << " newMAT 4 - doh " << baseColor << " spec " << spec << endl;
                }
            }
            materials[matID] = mat;
        }

        void handleTexture(const tinygltf::Texture &gltfTexture){
            texID++;
            VRTexturePtr img = VRTexture::create();
            //Get texture data layout information
            const auto &image = model.images[gltfTexture.source];
            int components = image.component;
            int width = image.width;
            int height = image.height;
            int bits = image.bits;
            cout << texID << " " << gltfTexture.source << " components " << components << " width " << width << " height " << height << " bits " << bits  << endl;

            const auto size = components * width * height * bits; //sizeof(unsigned char);
            char* data = new char[size];
            memcpy(data, image.image.data(), size);
            Vec3i dims = Vec3i(width, height, 1);

            int pf = Image::OSG_RGB_PF;
            if (components == 4) pf = Image::OSG_RGBA_PF;
            int f = Image::OSG_UINT8_IMAGEDATA;
            if (bits == 32) f = Image::OSG_FLOAT32_IMAGEDATA;
            img->getImage()->set( pf, dims[0], dims[1], dims[2], 1, 1, 0, (const UInt8*)data, f);
            //if (components == 4) setTexture(img, true);
            //if (components == 3) setTexture(img, false);

            textures[texID] = img;
        }

        void handleMesh(const tinygltf::Mesh &gltfMesh){
            meshID++;
            string res = "";

            res += to_string(meshID) + " " + gltfMesh.name;

            if (gltfMesh.primitives.size() == 1) {
                res += " ONE Primitive";
                //cout << res << endl;

                auto nodeID = meshToNode[meshID];
                auto& node = nodes[nodeID];
                tinygltf::Primitive primitive = gltfMesh.primitives[0];
                long n = 0;
                VRGeoData gdata = VRGeoData();

                string atts = "";
                const tinygltf::Accessor& accessorP = model.accessors[primitive.attributes["POSITION"]];
                const tinygltf::Accessor& accessorN = model.accessors[primitive.attributes["NORMAL"]];
                const tinygltf::Accessor& accessorColor = model.accessors[primitive.attributes["COLOR_0"]];
                const tinygltf::Accessor& accessorTexUV = model.accessors[primitive.attributes["TEXCOORD_0"]];
                const tinygltf::Accessor& accessorTexUV1 = model.accessors[primitive.attributes["TEXCOORD_1"]];
                for (auto att : primitive.attributes) atts += att.first + " ";
                const tinygltf::BufferView& bufferViewP = model.bufferViews[accessorP.bufferView];
                const tinygltf::BufferView& bufferViewN = model.bufferViews[accessorN.bufferView];
                const tinygltf::BufferView& bufferViewCO = model.bufferViews[accessorColor.bufferView];
                const tinygltf::BufferView& bufferViewUV = model.bufferViews[accessorTexUV.bufferView];

                // cast to float type read only. Use accessor and bufview byte offsets to determine where position data
                // is located in the buffer.
                const tinygltf::Buffer& bufferP = model.buffers[bufferViewP.buffer];
                const tinygltf::Buffer& bufferN = model.buffers[bufferViewN.buffer];
                const tinygltf::Buffer& bufferCO = model.buffers[bufferViewCO.buffer];
                const tinygltf::Buffer& bufferUV = model.buffers[bufferViewUV.buffer];
                // bufferView byteoffset + accessor byteoffset tells you where the actual position data is within the buffer. From there
                // you should already know how the data needs to be interpreted.
                const float* positions = reinterpret_cast<const float*>(&bufferP.data[bufferViewP.byteOffset + accessorP.byteOffset]);
                const float* normals   = reinterpret_cast<const float*>(&bufferN.data[bufferViewN.byteOffset + accessorN.byteOffset]);
                const float* colors   = reinterpret_cast<const float*>(&bufferCO.data[bufferViewCO.byteOffset + accessorColor.byteOffset]);
                const float* UVs   = reinterpret_cast<const float*>(&bufferUV.data[bufferViewUV.byteOffset + accessorTexUV.byteOffset]);
                // From here, you choose what you wish to do with this position data. In this case, we  will display it out.

                cout << atts << endl;
                cout << "PositBufferLength " << accessorP.count << " Type  " << accessorP.type << " " << bufferViewP.byteStride << endl;
                cout << "NormaBufferLength " << accessorN.count << " Type  " << accessorN.type << " " << bufferViewN.byteStride << endl;
                cout << "ColorBufferLength " << accessorColor.count << " Type  " << accessorColor.type << " " << bufferViewCO.byteStride << endl;
                cout << "TexUVBufferLength " << accessorTexUV.count << " Type  " << accessorTexUV.type << " " << bufferViewUV.byteStride << endl;
                cout << "PrimitiveIndecesC " << model.accessors[primitive.indices].count << " MODE: " << primitive.mode << " TRIS: " << model.accessors[primitive.indices].count/3 << endl;
                for (size_t i = 0; i < accessorP.count; ++i) {
                    // Positions are Vec3 components, so for each vec3 stride, offset for x, y, and z.
                    Vec3d pos = Vec3d( positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2] );
                    gdata.pushVert(pos);
                    //gdata.pushIndex(i);
                    n ++;
                }
                for (size_t i = 0; i < accessorN.count; ++i) {
                    Vec3d nor = Vec3d( normals  [i * 3 + 0], normals  [i * 3 + 1], normals  [i * 3 + 2] );
                    gdata.pushNorm(nor);
                    //gdata.pushNormalIndex(i);
                }
                if (accessorColor.count == accessorP.count){
                    for (size_t i = 0; i < accessorColor.count; ++i) {
                        if (accessorColor.type == 3){ auto cl = Color3f( colors[i * 3 + 0], colors[i * 3 + 1], colors[i * 3 + 2] ); gdata.pushColor(cl); /*gdata.pushColorIndex(i);*/ }
                        if (accessorColor.type == 4){ auto cl = Color4f( colors[i * 4 + 0], colors[i * 4 + 1], colors[i * 4 + 2], colors[i * 4 + 2] ); gdata.pushColor(cl); /*gdata.pushColorIndex(i);*/ }
                        //if (accessorColor.type == 4){ auto cl = Color4f( 212.0/255.0,175.0/255.0,55.0/255.0, 1 ); gdata.pushColor(cl); }
                    }
                }
                if (accessorTexUV.count == accessorP.count) {
                    for (size_t i = 0; i < accessorTexUV.count; ++i) {
                        Vec2d UV = Vec2d( UVs[i*2 + 0], UVs[i*2 + 1] );
                        gdata.pushTexCoord(UV);
                        //gdata.pushTexCoordIndex(i);
                    }
                }
                if (primitive.mode == 4) {
                    //DEFAULT TRIS
                    const tinygltf::Accessor& accessorIndices = model.accessors[primitive.indices];
                    const tinygltf::BufferView& bufferViewIndices = model.bufferViews[accessorIndices.bufferView];
                    const tinygltf::Buffer& bufferInd = model.buffers[bufferViewIndices.buffer];
                    const int* indices   = reinterpret_cast<const int*>(&bufferInd.data[bufferViewIndices.byteOffset + accessorIndices.byteOffset]);
                    for (size_t i = 0; i < accessorIndices.count/3; ++i) {
                        gdata.pushTri(indices[i*3+0],indices[i*3+1],indices[i*3+2]);
                    }
                }
                node->geoData = gdata;
                node->matID = primitive.material;
                node->material = materials[matID];
                cout << "prim with v " << n << " : " << primitive.mode <<  endl;
            }

            if (gltfMesh.primitives.size() > 1) {
                res += " primitives: " + gltfMesh.primitives.size();
            }
            //for (auto each : gltfMesh.primitives) cout << " " << each.;

            //cout << res << endl;
        }

        void connectTree(){
            for (auto eachPair : nodes){
                auto ID = eachPair.first;
                auto& node = eachPair.second;
                for (auto childID : childrenPerNode[ID]){
                    node->addChild(nodes[childID]);
                }
            }
        }

        bool parsetinygltf() {
            for (auto each: model.scenes) handleScene(each);
            for (auto each: model.nodes) handleNode(each);
            for (auto each: model.textures) handleTexture(each);
            for (auto each: model.materials) handleMaterial(each);
            for (auto each: model.meshes) handleMesh(each);
            connectTree();
            return true;
        }

    public:
        GLTFLoader(string p, VRTransformPtr r, VRProgressPtr pr, bool t) : path(p), res(r), progress(pr), threaded(t) {}

        void load() {
            ifstream file;
            if (!openFile(file)) return;
            auto fileSize = getFileSize(file);

            if (progress == 0) progress = VRProgress::create();
            progress->setup("load GLTF " + path, fileSize);
            progress->reset();

            //using namespace tinygltf;
            tinygltf::TinyGLTF loader;
            std::string err;
            std::string warn;

            string ext = GetFilePathExtension(path);
            bool ret = false;

            if ( ext.compare("glb") == 0 ) {
                cout << "try loading bin glb file at " << path << endl;
                ret = loader.LoadBinaryFromFile(&model, &err, &warn, path.c_str()); // for binary glTF(.glb)
            }

            if ( ext.compare("gltf") == 0 ) {
                cout << "try loading ASCII gltf file at " << path << endl;
                ret = loader.LoadASCIIFromFile(&model, &err, &warn, path.c_str());
            }

            if (!warn.empty()) {
                printf("Warn: %s\n", warn.c_str());
            }

            if (!err.empty()) {
                printf("Err: %s\n", err.c_str());
            }

            if (!ret) {
                printf("Failed to parse glTF\n");
                //return -1;
            }
            debugDump(&model);
            parsetinygltf();

            version = 2;

            gltfschema = GLTFSchema(version);
            tree = new GLTFNNode("Root", "Root");
            tree->addChild(nodes[0]);
            tree->obj = res;
            //tree->print();
            tree->buildOSG();
            tree->applyTransformations();
            tree->applyMaterials();
            tree->applyGeometries();
            progress->finish();
            return;
            tree->resolveLinks(references);

            progress->finish();
            delete tree;
        }

        void debugDump(tinygltf::Model *model){
            cout << "This glTF model has:\n"
                << model->accessors.size() << " accessors\n"
                << model->animations.size() << " animations\n"
                << model->buffers.size() << " buffers\n"
                << model->bufferViews.size() << " bufferViews\n"
                << model->materials.size() << " materials\n"
                << model->meshes.size() << " meshes\n"
                << model->nodes.size() << " nodes\n"
                << model->textures.size() << " textures\n"
                << model->images.size() << " images\n"
                << model->skins.size() << " skins\n"
                << model->samplers.size() << " samplers\n"
                << model->cameras.size() << " cameras\n"
                << model->scenes.size() << " scenes\n"
                << model->lights.size() << " lights\n"
                << endl;
        }
};

void OSG::loadGLTF(string path, VRTransformPtr res, VRProgressPtr p, bool thread) {
    GLTFLoader gltf(path, res, p, thread);
    gltf.load();
}

void constructGLTF(tinygltf::Model& model, VRObjectPtr obj, int pID = -1) {
    tinygltf::Scene& scene = model.scenes.back();

    // new node
    int nID = model.nodes.size();
    model.nodes.push_back(tinygltf::Node());
    tinygltf::Node& node = model.nodes.back();

    // from object
    node.name = obj->getName();

    // from transform
    auto trans = dynamic_pointer_cast<VRTransform>(obj);
    if (trans) {
        Matrix4d mat = trans->getMatrix();
        double* data = mat.getValues();
        for (int i=0; i<16; i++) node.matrix.push_back(data[i]);
    }

    // scene graph structure
    if (pID >= 0) {
        auto& parent = model.nodes[pID];
        parent.children.push_back(nID);
    } else scene.nodes.push_back(nID);

    // from geometry
    auto geo = dynamic_pointer_cast<VRGeometry>(trans);
    if (geo) {
        int mID = model.meshes.size();
        model.meshes.push_back(tinygltf::Mesh());
        tinygltf::Mesh& mesh = model.meshes.back();
        mesh.name = geo->getName() + "_mesh";
        node.mesh = mID;

        VRGeoData data(geo);

        int Ntypes = data.getDataSize(0);
        int Nlengths = data.getDataSize(1);
        int Nindices = data.getDataSize(2);
        int Npositions = data.getDataSize(3);
        int Nnormals = data.getDataSize(4);
        int Ncolors3 = data.getDataSize(5);
        int Ncolors4 = data.getDataSize(6);
        int Ntexcoords = data.getDataSize(7);

        // buffer
        int indicesBufID = model.buffers.size();
        model.buffers.push_back(tinygltf::Buffer());
        tinygltf::Buffer& indicesBuffer = model.buffers.back();
        vector<int> indicesVec;
        for (int i = 0; i<Nindices; i++) indicesVec.push_back(data.getIndex(i));
        indicesBuffer.name = "indicesBuffer";
        unsigned char* dInds = (unsigned char*)&indicesVec[0];
        indicesBuffer.data = vector<unsigned char>( dInds, dInds + sizeof(int)*indicesVec.size() );

        int positionsBufID = model.buffers.size();
        model.buffers.push_back(tinygltf::Buffer());
        tinygltf::Buffer& positionsBuffer = model.buffers.back();
        vector<Vec3f> positionsVec;
        for (int i = 0; i<Npositions; i++) positionsVec.push_back(Vec3f(data.getPosition(i)));
        positionsBuffer.name = "positionsBuffer";
        unsigned char* dPos = (unsigned char*)&positionsVec[0];
        positionsBuffer.data = vector<unsigned char>( dPos, dPos + sizeof(Vec3f)*positionsVec.size() );

        int normalsBufID = model.buffers.size();
        model.buffers.push_back(tinygltf::Buffer());
        tinygltf::Buffer& normalsBuffer = model.buffers.back();
        vector<Vec3f> normalsVec;
        for (int i = 0; i<Nnormals; i++) normalsVec.push_back(Vec3f(data.getNormal(i)));
        normalsBuffer.name = "normalsBuffer";
        unsigned char* dNorms = (unsigned char*)&normalsVec[0];
        normalsBuffer.data = vector<unsigned char>( dNorms, dNorms + sizeof(Vec3f)*normalsVec.size() );

        // buffer views
        int indicesViewID = model.bufferViews.size();
        model.bufferViews.push_back(tinygltf::BufferView());
        tinygltf::BufferView& indicesView = model.bufferViews.back();
        indicesView.buffer = indicesBufID;
        indicesView.byteOffset = 0;
        indicesView.byteLength = sizeof(int)*3;
        indicesView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;

        int positionsViewID = model.bufferViews.size();
        model.bufferViews.push_back(tinygltf::BufferView());
        tinygltf::BufferView& positionsView = model.bufferViews.back();
        positionsView.buffer = positionsBufID;
        positionsView.byteOffset = 0;
        positionsView.byteLength = sizeof(Vec3f)*3;
        positionsView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

        int normalsViewID = model.bufferViews.size();
        model.bufferViews.push_back(tinygltf::BufferView());
        tinygltf::BufferView& normalsView = model.bufferViews.back();
        normalsView.buffer = normalsBufID;
        normalsView.byteOffset = 0;
        normalsView.byteLength = sizeof(Vec3f)*3;
        normalsView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

        // accessors
        int indicesAccID = model.accessors.size();
        model.accessors.push_back(tinygltf::Accessor());
        tinygltf::Accessor& indices = model.accessors.back();
        indices.name = "indices";
        indices.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
        indices.type = TINYGLTF_TYPE_SCALAR;
        indices.count = Nindices;
        indices.bufferView = indicesViewID;
        indices.byteOffset = 0;

        int positionsAccID = model.accessors.size();
        model.accessors.push_back(tinygltf::Accessor());
        tinygltf::Accessor& positions = model.accessors.back();
        positions.name = "positions";
        positions.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        positions.type = TINYGLTF_TYPE_VEC3;
        positions.count = Npositions;
        positions.bufferView = positionsViewID;
        positions.byteOffset = 0;

        int normalsAccID = model.accessors.size();
        model.accessors.push_back(tinygltf::Accessor());
        tinygltf::Accessor& normals = model.accessors.back();
        normals.name = "normals";
        normals.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
        normals.type = TINYGLTF_TYPE_VEC3;
        normals.count = Nnormals;
        normals.bufferView = normalsViewID;
        normals.byteOffset = 0;

        // add types
        for (int iType = 0; iType < Ntypes; iType++) {
            int type = data.getType(iType);
            int length = data.getLength(iType);

            mesh.primitives.push_back(tinygltf::Primitive());
            tinygltf::Primitive& primitive = mesh.primitives.back();
            primitive.mode = TINYGLTF_MODE_TRIANGLES;
            primitive.indices = indicesAccID;
            primitive.attributes["POSITION"] = positionsAccID;
            primitive.attributes["NORMAL"] = normalsAccID;
        }

        /*
        #define TINYGLTF_MODE_POINTS (0)
        #define TINYGLTF_MODE_LINE (1)
        #define TINYGLTF_MODE_LINE_LOOP (2)
        #define TINYGLTF_MODE_LINE_STRIP (3)
        #define TINYGLTF_MODE_TRIANGLES (4)
        #define TINYGLTF_MODE_TRIANGLE_STRIP (5)
        #define TINYGLTF_MODE_TRIANGLE_FAN (6);
        */
    }


    for (auto child : obj->getChildren()) constructGLTF(model, child, nID);
}

void OSG::writeGLTF(VRObjectPtr obj, string path) {
    tinygltf::Model model;
    model.scenes.push_back(tinygltf::Scene());
    model.defaultScene = 0;
    model.asset.version = "2.0";

    constructGLTF(model, obj);
    tinygltf::TinyGLTF writer;
    writer.WriteGltfSceneToFile(&model, path, true, true, true, false); // last flag is binary
}









