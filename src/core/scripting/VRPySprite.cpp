#include "VRPySprite.h"
#include "VRPyTransform.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Sprite, New_VRObjects_ptr);

PyMethodDef VRPySprite::methods[] = {
    {"getText", PyWrap(Sprite, getLabel, "Get label text from sprite.", string) },
    {"getSize", PyWrap(Sprite, getSize, "Get size of sprite.", Vec2d) },
    {"setText", PyWrapOpt(Sprite, setText, "Create text texture, (text, resolution, fg, bg, font)", "1|0 0 0 1|0 0 0 0|SANS 20", VRTexturePtr, string, float, Color4f, Color4f, string ) },
    {"setSize", PyWrap(Sprite, setSize, "Set sprite size.", void, float, float ) },
    {"setTexture", PyWrap(Sprite, setTexture, "Set sprite texture", void, string) },
    {"webOpen", PyWrap(Sprite, webOpen, "Open and display a website - webOpen(str uri, int width, flt ratio)", void, string, int, float) },
    {"showResizeTool", PyWrapOpt(Sprite, showResizeTool, "Show handles to resize the sprite in 3D", "0.1|1", void, bool, float, bool) },
    {"convertToCloth", PyWrap(Sprite, convertToCloth, "convert this Sprite to cloth (softbody)", void) },
    {NULL}  /* Sentinel */
};

