#include "VRKeyboard.h"

//#include <GL/glut.h>
//#include <OpenSG/OSGBaseInitFunctions.h>
#ifndef WITHOUT_GTK
#include <gdk/gdkevents.h>
#endif

using namespace OSG;

void VRKeyboard::keyboard(unsigned int k, bool pressed, int x, int y) { change_button(k,pressed); }
void VRKeyboard::keyboard_special(int k, bool pressed, int x, int y) { change_button(k+100,pressed); }

VRKeyboard::VRKeyboard() : VRDevice("keyboard") {
    addBeacon();
}

VRKeyboard::~VRKeyboard() {;}

VRKeyboardPtr VRKeyboard::create() {
    cout << " VRKeyboard::create" << endl;
    auto d = VRKeyboardPtr(new VRKeyboard());
    d->initIntersect(d);
    return d;
}

VRKeyboardPtr VRKeyboard::ptr() { return static_pointer_cast<VRKeyboard>( shared_from_this() ); }

#ifndef WITHOUT_GTK
void VRKeyboard::setGtkEvent(_GdkEventKey* event) { gdk_event = event; }
_GdkEventKey* VRKeyboard::getGtkEvent() { return gdk_event; }
#endif

