#include <OpenSG/OSGGL.h>
#include <OpenSG/OSGGLUT.h>
#include <OpenSG/OSGGLUTWindow.h>
#include <OpenSG/OSGChangeList.h>

#include "VRWindowManager.h"
#include "core/setup/devices/VRMouse.h"
#include "core/setup/devices/VRKeyboard.h"
#include "core/utils/VROptions.h"
#include "core/utils/VRTimer.h"
#include "core/objects/object/VRObject.h"
#include "core/utils/VRRate.h"
#include "core/scene/VRScene.h"
#include "core/setup/VRSetup.h"
#include "core/scene/rendering/VRRenderStudio.h"
#include "core/gui/VRGuiManager.h"
#include "core/utils/VRFunction.h"
#include "core/utils/VRGlobals.h"
#include "core/utils/VRProfiler.h"
#include "core/utils/xml.h"

#include "VRView.h"
#include "VRGlutWindow.h"
#ifndef WASM
#include "VRMultiWindow.h"
#endif

#ifndef WITHOUT_GTK
#include "core/gui/VRGuiUtils.h"
#include <gtkmm/builder.h>
#include <gtkmm/main.h>
#include <gtkmm/window.h>
#include <gtkmm/drawingarea.h>
#include "VRGtkWindow.h"
#include "core/gui/VRGuiManager.h"
#include "core/gui/VRGuiConsole.h"
#define WARN(x) \
VRGuiManager::get()->getConsole( "Errors" )->write( x+"\n" );
#else
#define WARN(x) \
cout << x << endl;
#endif


OSG_BEGIN_NAMESPACE;
using namespace std;

VRWindowManager::VRWindowManager() : changeListStats("manager"){
    cout << "Init VRWindowManager\n";
    ract = RenderAction::create();
}

VRWindowManager::~VRWindowManager() {}

bool VRWindowManager::checkWin(string name) {
    if (windows.count(name) == 1) return true;
    else {
        cout << "\nError! request for not existing window " << name << "\n";
        return false;
    }
}

RenderActionRefPtr VRWindowManager::getRenderAction() { return ract; }

void setMultisampling(bool on) {
    bool res = false;

#if !defined(GL_MULTISAMPLE_SGIS) && !defined(GL_MULTISAMPLE_ARB)
#ifndef _WIN32
#warning "No Multisampling support detected, disabling"
#endif
#else

  if (on) {
#ifdef GL_MULTISAMPLE_SGIS
    res = true;
    glEnable(GL_MULTISAMPLE_SGIS);
#endif //GL_MULTISAMPLE_SGIS
#ifdef GL_MULTISAMPLE_ARB
    res = true;
    glEnable(GL_MULTISAMPLE_ARB);
#endif //GL_MULTISAMPLE_ARB
  } else {
#ifdef GL_MULTISAMPLE_SGIS
    glDisable(GL_MULTISAMPLE_SGIS);
#endif //GL_MULTISAMPLE_SGIS
#ifdef GL_MULTISAMPLE_ARB
    glDisable(GL_MULTISAMPLE_ARB);
#endif //GL_MULTISAMPLE_ARB
  }
#endif //!defined(GL_MULTISAMPLE_SGIS) && !defined(GL_MULTISAMPLE_ARB)
    //cout << "\nSET AA: " << res;
    if (res) {;};//__GL_FXAA_MODE 	 = 7;//find out how to set vie application
}

VRWindowPtr VRWindowManager::addGlutWindow(string name) {
    VRGlutWindowPtr win = VRGlutWindow::create();
    win->setName(name);
    win->setAction(ract);
    windows[win->getName()] = win;
    return win;
}

VRWindowPtr VRWindowManager::addMultiWindow(string name) {
#ifndef WASM
    VRMultiWindowPtr win = VRMultiWindow::create();
    win->setName(name);
    win->setAction(ract);
    windows[win->getName()] = win;
    return win;
#else
    return 0;
#endif
}

VRWindowPtr VRWindowManager::addGtkWindow(string name, string glarea) {
#ifndef WITHOUT_GTK
    cout << " add Gtk window " << name << endl;
    //gdk_error_trap_push();
    //if (gdk_error_trap_pop()) cout << "    ---- AAA1 ------ " << endl;

    Gtk::DrawingArea* drawArea = 0;
    VRGuiBuilder()->get_widget(glarea, drawArea); // TODO: create new glarea, add flag to editor area window!
    VRGtkWindowPtr win = VRGtkWindow::create(drawArea);

    editorWindow = win;
    win->setName(name);
    win->setAction(ract);
    windows[win->getName()] = win;
    return win;
#else
    return 0;
#endif
}

VRGtkWindowPtr VRWindowManager::getEditorWindow() { return editorWindow; }

void VRWindowManager::pauseRendering(bool b) { rendering_paused = b; }

void VRWindowManager::getWindowSize(string name, int& width, int& height) {
    if (!checkWin(name)) return;

    WindowMTRecPtr win = windows[name]->getOSGWindow();
    width = win->getWidth();
    height = win->getHeight();
}

void VRWindowManager::stopWindows() {
    cout << "VRWindowManager::stopWindows" << endl;
#ifndef WASM
    BarrierRefPtr barrier = Barrier::get("PVR_rendering", true);
    while (barrier->getNumWaiting() < VRWindow::active_window_count) usleep(1);
    for (auto w : getWindows() ) w.second->stop();
    barrier->enter(VRWindow::active_window_count+1);
#endif
}

bool VRWindowManager::doRenderSync = false;

void VRWindowManager::updateWindows() {
    if (rendering_paused) return;
    auto scene = VRScene::getCurrent();
    if (scene) scene->allowScriptThreads();

    ract->setResetStatistics(false);
    StatCollector* sc = ract->getStatCollector();
    if (sc) {
        sc->reset();
        sc->getElem(VRGlobals::FRAME_RATE.statFPS)->add(VRGlobals::FRAME_RATE.fps);
        sc->getElem(VRGlobals::UPDATE_LOOP1.statFPS)->add(VRGlobals::UPDATE_LOOP1.fps);
        sc->getElem(VRGlobals::UPDATE_LOOP2.statFPS)->add(VRGlobals::UPDATE_LOOP2.fps);
        sc->getElem(VRGlobals::UPDATE_LOOP3.statFPS)->add(VRGlobals::UPDATE_LOOP3.fps);
        sc->getElem(VRGlobals::UPDATE_LOOP4.statFPS)->add(VRGlobals::UPDATE_LOOP4.fps);
        sc->getElem(VRGlobals::UPDATE_LOOP5.statFPS)->add(VRGlobals::UPDATE_LOOP5.fps);
        sc->getElem(VRGlobals::UPDATE_LOOP6.statFPS)->add(VRGlobals::UPDATE_LOOP6.fps);
        sc->getElem(VRGlobals::UPDATE_LOOP7.statFPS)->add(VRGlobals::UPDATE_LOOP7.fps);
        sc->getElem(VRGlobals::RENDER_FRAME_RATE.statFPS)->add(VRGlobals::RENDER_FRAME_RATE.fps);
        sc->getElem(VRGlobals::SLEEP_FRAME_RATE.statFPS)->add(VRGlobals::SLEEP_FRAME_RATE.fps);
        sc->getElem(VRGlobals::SWAPB_FRAME_RATE.statFPS)->add(VRGlobals::SWAPB_FRAME_RATE.fps);
        sc->getElem(VRGlobals::WINDOWS_FRAME_RATE.statFPS)->add(VRGlobals::WINDOWS_FRAME_RATE.fps);
        sc->getElem(VRGlobals::SCRIPTS_FRAME_RATE.statFPS)->add(VRGlobals::SCRIPTS_FRAME_RATE.fps);
        sc->getElem(VRGlobals::PHYSICS_FRAME_RATE.statFPS)->add(VRGlobals::PHYSICS_FRAME_RATE.fps);
        sc->getElem(VRGlobals::GTK1_FRAME_RATE.statFPS)->add(VRGlobals::GTK1_FRAME_RATE.fps);
        sc->getElem(VRGlobals::GTK2_FRAME_RATE.statFPS)->add(VRGlobals::GTK2_FRAME_RATE.fps);
        sc->getElem(VRGlobals::SMCALLBACKS_FRAME_RATE.statFPS)->add(VRGlobals::SMCALLBACKS_FRAME_RATE.fps);
        sc->getElem(VRGlobals::SETUP_FRAME_RATE.statFPS)->add(VRGlobals::SETUP_FRAME_RATE.fps);
        sc->getElem(VRGlobals::SCRIPTS_FRAME_RATE.statFPS)->add(VRGlobals::SCRIPTS_FRAME_RATE.fps);
    }

    //TODO: use barrier->getnumwaiting to make a state machine, allways ensure all are waiting!!

#ifndef WASM
    BarrierRefPtr barrier = Barrier::get("PVR_rendering", true);
#endif

    auto updateSceneLinks = [&]() {
        for (auto view : VRSetup::getCurrent()->getViews()) {
            if (auto r = view->getRenderingL()) r->updateSceneLink();
            if (auto r = view->getRenderingR()) r->updateSceneLink();
        }
    };

#ifndef WASM
    auto wait = [&](int timeout = -1) {
        int pID = VRProfiler::get()->regStart("window manager barrier");

        if (timeout > 0) {
            size_t tEnter = time(0);
            while (barrier->getNumWaiting() < VRWindow::active_window_count) {
                usleep(1);
                size_t tNow = time(0);
                int delta = tNow - tEnter;
                if (delta >= timeout) {
                    cout << "WARNING! skipping barrier!" << endl;
                    return false;
                }
            }
        }

        barrier->enter(VRWindow::active_window_count+1);
        VRProfiler::get()->regStop(pID);
        return true;
    };
#endif

    auto tryRender = [&]() {
#ifndef WASM
        if (barrier->getNumWaiting() != VRWindow::active_window_count) return true;
        if (!wait()) return false;
        for (auto w : getWindows() ) if (auto win = dynamic_pointer_cast<VRMultiWindow>(w.second)) if (win->getState() == VRMultiWindow::INITIALIZING) win->initialize();
        commitChanges();

        auto clist = Thread::getCurrentChangeList();
        VRGlobals::NCHANGED = clist->getNumChanged();
        VRGlobals::NCREATED = clist->getNumCreated();
        //changeListStats.update();
        if (!wait()) return false;
        /** let the windows merge the change lists, sync and clear **/
        if (!wait()) return false;
#ifndef WITHOUT_GTK
        for (auto w : getWindows() ) if (auto win = dynamic_pointer_cast<VRGtkWindow>(w.second)) win->render();
#endif
#else
        commitChanges();

        auto clist = Thread::getCurrentChangeList();
        VRGlobals::NCHANGED = clist->getNumChanged();
        VRGlobals::NCREATED = clist->getNumCreated();
        for (auto w : getWindows() ) if (auto win = dynamic_pointer_cast<VRGlutWindow>(w.second)) win->render();
#endif
        clist->clear();
        return true;
    };

    updateSceneLinks();

    if (!tryRender()) {
#ifndef WASM
        cout << "WARNING! a remote window hangs or something!\n";
        for (auto w : getWindows() ) {
            auto win = dynamic_pointer_cast<VRMultiWindow>(w.second);
            if (!win) continue;
            if (win->isWaiting()) continue;
            cout << "WARNING!  window " << win->getName() << " is hanging, state: " << win->getStateString() << endl;
            WARN("WARNING! Lost connection with " + win->getName());
            win->reset();
        }
#endif
    }

    if (scene) scene->blockScriptThreads();
}

void VRWindowManager::setWindowView(string name, VRViewPtr view) {
    if (!checkWin(name)) return;
    VRWindowPtr win = windows[name];
    win->addView(view);
}

void VRWindowManager::addWindowServer(string name, string server) {
#ifndef WASM
    if (!checkWin(name)) return;
    VRMultiWindowPtr win = dynamic_pointer_cast<VRMultiWindow>( windows[name] );
    win->addServer(server);
#endif
}

void VRWindowManager::removeWindow(string name) { windows.erase(name); }

void VRWindowManager::changeWindowName(string& name, string new_name) {
    map<string, VRWindowPtr>::iterator i = windows.find(name);
    if (i == windows.end()) return;

    VRWindowPtr win = i->second;
    windows.erase(i);
    win->setName(new_name);
    windows[win->getName()] = win;
    name = win->getName();
}

map<string, VRWindowPtr> VRWindowManager::getWindows() { return windows; }

VRWindowPtr VRWindowManager::getWindow(string name) {
    if (windows.count(name) == 0) return 0;
    else return windows[name];
}

void VRWindowManager::save(XMLElementPtr node) {
    for (auto window : windows) {
        XMLElementPtr wn = node->addChild("Window");
        window.second->save(wn);
    }
}

void VRWindowManager::load(XMLElementPtr node) {
    cout << "start loading windows\n";
    for (auto el : node->getChildren()) {
        if (!el) continue;

        string type = el->getAttribute("type");
        string name = el->getAttribute("name");
        cout << " VRWindowManager::load '" << type << "'  '" << name << "'" << endl;
        //el->print();

        if (type == "0") {
            VRWindowPtr win = addMultiWindow(name);
            win->load(el);
        }

        if (type == "1") {
            VRWindowPtr win = addGlutWindow(name);
            win->load(el);
        }

        if (type == "2") {
            VRWindowPtr win = addGtkWindow(name);
            win->load(el);
        }
    }
}

OSG_END_NAMESPACE;
