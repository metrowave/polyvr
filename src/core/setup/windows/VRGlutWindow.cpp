#include <OpenSG/OSGGLUT.h>

#include "VRGlutWindow.h"

#include "../devices/VRMouse.h"
#include "../devices/VRKeyboard.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

map<int, VRGlutWindow*> glutWindows;

VRGlutWindow* getCurrentWindow() {
#ifndef WASM
    return glutWindows[glutGetWindow()];
#else
    return glutWindows.begin()->second;
#endif
}

void glutResize(int w, int h) { getCurrentWindow()->resize(w, h); }
/*void glutMouse(int b, int s, int x, int y) { VRMouse::get()->mouse(b ,s ,x ,y); } // TODO: pass getCurrentWindow() as first parameter
void glutMotion(int x, int y) { VRMouse::get()->motion(x, y); }
void glutKeyboard(unsigned char k, int x, int y) { VRKeyboard::get()->keyboard(k, x, y); }
void glutKeyboard2(int k, int x, int y) { VRKeyboard::get()->keyboard_special(k, x, y); }*/

VRGlutWindow::VRGlutWindow() {
    cout << "Glut: New Window" << endl;
    type = 1;

    glutInitWindowSize(width, height);
    winID = glutCreateWindow("PolyVR");

    GLUTWindowMTRecPtr win = GLUTWindow::create();
    _win = win;
    win->setGlutId(winID);
    win->setSize(width, height);
    win->init();

    glutWindows[winID] = this;

    glutReshapeFunc( glutResize );
    /*glutKeyboardFunc(glutKeyboard);
    glutSpecialFunc(glutKeyboard2);
    glutMotionFunc(glutMotion);
    glutMouseFunc(glutMouse);*/
}

VRGlutWindow::~VRGlutWindow() {
    glutDestroyWindow(winID);
    win = NULL;
}

VRGlutWindowPtr VRGlutWindow::ptr() { return static_pointer_cast<VRGlutWindow>( shared_from_this() ); }
VRGlutWindowPtr VRGlutWindow::create() { return VRGlutWindowPtr(new VRGlutWindow() ); }

void VRGlutWindow::save(XMLElementPtr node) { VRWindow::save(node); }
void VRGlutWindow::load(XMLElementPtr node) { VRWindow::load(node); }

OSG_END_NAMESPACE;




