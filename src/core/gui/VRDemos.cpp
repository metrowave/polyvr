#include "VRDemos.h"

#include <gtkmm/table.h>
#include <gtkmm/stock.h>
#include <gtkmm/settings.h>
#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/paned.h>
#include <gtkmm/liststore.h>
#include <gtkmm/cellrenderertoggle.h>
#include <gtkmm/treeview.h>
#include <gtkmm/window.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>
#include <gtkmm/builder.h>
#include <gtkmm/frame.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/filechooser.h>
#include <string>
#include <iostream>
#include <boost/bind.hpp>

#include "core/scene/VRSceneLoader.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/scene/import/VRImport.h"
#include "core/objects/VRTransform.h"
#include "core/objects/VRCamera.h"
#include "core/math/boundingbox.h"
#include "core/scene/VRProjectsList.h"
#include "core/setup/devices/VRSignal.h"
#include "core/utils/system/VRSystem.h"
#include "VRGuiUtils.h"
#include "VRGuiSignals.h"
#include "VRGuiFile.h"
#include "VRGuiContextMenu.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

// TODO:
// rename && delete scenes
// switch to a liststore || something!

class scenes_columns : public Gtk::TreeModelColumnRecord {
    public:
        scenes_columns() { add(name); add(favs); add(obj); }

        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<bool> favs;
        Gtk::TreeModelColumn<gpointer> obj;
};

VRAppLauncher::VRAppLauncher(VRAppSectionPtr s) : section(s) {}
VRAppLauncher::~VRAppLauncher() {}

VRAppLauncherPtr VRAppLauncher::create(VRAppSectionPtr s) { return VRAppLauncherPtr( new VRAppLauncher(s) ); }

void VRAppLauncher::updatePixmap() {
    if (imgScene == 0) return;
    if ( !exists( pxm_path ) ) return;
    try {
        Glib::RefPtr<Gdk::Pixbuf> pxb = Gdk::Pixbuf::create_from_file (pxm_path);
        imgScene->set(pxb);
        imgScene->set_size_request(100, 75);
    } catch (...) { cout << "Warning: Caught exception in VRAppManager::updatePixmap, ignoring.."; }
}

void VRAppLauncher::show() { widget->show(); }
void VRAppLauncher::hide() { widget->hide(); }

/** Section **/


VRAppSection::VRAppSection(string name) {
    setNameSpace("__system_apps__");
    setName(name);
}

VRAppSection::~VRAppSection() {}
VRAppSectionPtr VRAppSection::create(string name) { return VRAppSectionPtr( new VRAppSection(name) ); }
VRAppSectionPtr VRAppSection::ptr() { return shared_from_this(); }

VRAppLauncherPtr VRAppSection::addLauncher(string path, string timestamp, VRGuiContextMenu* menu, VRAppManager* mgr, bool write_protected, bool favorite, string table) {
    if (!exists(path)) return 0;
    if (apps.count(path)) return apps[path];
    auto app = VRAppLauncher::create(ptr());
    app->path = path;
    app->lastStarted = timestamp;
    string filename = getFileName(path);
    string foldername = getFolderName(path);
    app->pxm_path = foldername + "/.local_" + filename.substr(0,filename.size()-4) + "/snapshot.png";
    app->write_protected = write_protected;
    app->favorite = favorite;
    app->table = table;
    apps[path] = app;
    setButton(app, menu, mgr);
    return app;
}

void VRAppSection::setButton(VRAppLauncherPtr e, VRGuiContextMenu* menu, VRAppManager* mgr) {
    Gtk::Settings::get_default()->property_gtk_button_images() = true;

    string rpath = VRSceneManager::get()->getOriginalWorkdir();

    // prep icons
    e->imgPlay = Gtk::manage(new Gtk::Image(Gtk::Stock::MEDIA_PLAY, Gtk::ICON_SIZE_BUTTON));
    e->imgOpts = loadGTKIcon(0, rpath+"/ressources/gui/opts20.png", 20, 20);
    e->imgScene = loadGTKIcon(0, rpath+"/ressources/gui/default_scene.png", 100, 75);
    e->imgLock = loadGTKIcon(0, rpath+"/ressources/gui/lock20.png", 20, 20);
    e->imgUnlock = loadGTKIcon(0, rpath+"/ressources/gui/unlock20.png", 20, 20);

    // prep other widgets
    e->widget = Gtk::manage(new Gtk::Frame());
    Gtk::EventBox* ebox = Gtk::manage(new Gtk::EventBox());
    Gtk::HBox* hb = Gtk::manage(new Gtk::HBox(false, 0));
    Gtk::VBox* vb = Gtk::manage(new Gtk::VBox(false, 0));
    Gtk::VBox* vb2 = Gtk::manage(new Gtk::VBox(false, 0));
    e->label = Gtk::manage(new Gtk::Label(e->path, true));
    e->timestamp = Gtk::manage(new Gtk::Label(e->lastStarted, true));
    e->butPlay = Gtk::manage(new Gtk::Button());
    e->butOpts = Gtk::manage(new Gtk::Button());
    e->butLock = Gtk::manage(new Gtk::Button());
    e->label->set_alignment(0.5, 0.5);
    e->timestamp->set_alignment(0.5, 0.5);
    e->label->set_ellipsize(Pango::ELLIPSIZE_START);
    e->label->set_max_width_chars(20);
    e->butPlay->set_tooltip_text("Play/Stop");
    e->butOpts->set_tooltip_text("Options");
    e->butLock->set_tooltip_text("Write protection");
    e->label->set_tooltip_text(e->path);

    // build widget
    vb2->pack_start(*e->butOpts, false, false, 0);
    vb2->pack_end(*e->butLock, false, false, 0);
    hb->pack_start(*e->imgScene, false, false, 10);
    hb->pack_end(*e->butPlay, false, false, 5);
    hb->pack_end(*vb2, false, false, 5);
    vb->pack_start(*e->label, false, false, 5);
    vb->pack_start(*hb, false, false, 5);
    if (e->lastStarted != "") vb->pack_start(*e->timestamp, false, false, 2);
    e->widget->add(*ebox);
    ebox->add(*vb);
    e->butPlay->add(*e->imgPlay);
    e->butOpts->add(*e->imgOpts);
    if (e->write_protected) e->butLock->add(*e->imgLock);
    else e->butLock->add(*e->imgUnlock);

    e->updatePixmap();

    // events
    e->uPixmap = VRDeviceCb::create("GUI_addDemoEntry", boost::bind(&VRAppLauncher::updatePixmap, e) );
    VRGuiSignals::get()->getSignal("onSaveScene")->add( e->uPixmap );

    menu->connectWidget("DemoMenu", ebox);
    ebox->signal_event().connect( sigc::bind<VRAppLauncherPtr>( sigc::mem_fun(*mgr, &VRAppManager::on_any_event), e) );

    e->butPlay->signal_clicked().connect( sigc::bind<VRAppLauncherPtr>( sigc::mem_fun(*mgr, &VRAppManager::toggleDemo), e) );
    e->butOpts->signal_clicked().connect( sigc::bind<VRAppLauncherPtr>( sigc::mem_fun(*mgr, &VRAppManager::on_menu_advanced), e) );
    e->butLock->signal_clicked().connect( sigc::bind<VRAppLauncherPtr>( sigc::mem_fun(*mgr, &VRAppManager::on_lock_toggle), e) );
    e->widget->show_all();
}

int VRAppSection::getSize() { return apps.size(); }

void VRAppSection::fillTable(string t, Gtk::Table* tab, int& i) {
    int x,y;
    Gtk::AttachOptions optsH = Gtk::FILL|Gtk::EXPAND;
    Gtk::AttachOptions optsV = Gtk::SHRINK;
    //Gtk::AttachOptions optsV = Gtk::AttachOptions(0);

    for (auto d : apps) {
        if (d.second->table != t) continue;
        if (d.second->widget == 0) continue;

        Gtk::Widget* w = d.second->widget;
        x = i%2;
        y = i/2;
        tab->attach( *w, x, x+1, y, y+1, optsH, optsV, 10, 10);
        i++;
    }
}

void VRAppSection::clearTable(string t, Gtk::Table* tab) {
    for (auto d : apps) {
        if (d.second->table != t) continue;

        Gtk::Widget* w = d.second->widget;
        if (w == 0) continue;
        tab->remove(*w);
    }
}

void VRAppSection::setGuiState(VRAppLauncherPtr e, bool running, bool noLauncherScene) {
    if (noLauncherScene) running = true; // disables widget
    for (auto i : apps) {
        VRAppLauncherPtr d = i.second;
        if (d->widget) d->widget->set_sensitive(!running);
        if (d->imgPlay) d->imgPlay->set(Gtk::Stock::MEDIA_PLAY, Gtk::ICON_SIZE_BUTTON);
        if (d != e) d->running = false;
    }
}

void VRAppSection::remLauncher(string path) { apps.erase(path); }
VRAppLauncherPtr VRAppSection::getLauncher(string path) { return apps.count(path) ? apps[path] : 0; }

map<string, VRAppLauncherPtr> VRAppSection::getLaunchers() { return apps; }

/** Manager **/

VRAppManager::VRAppManager() {
    initMenu();

    auto examplesSection = addSection("examples");
    auto favoritesSection = addSection("favorites");
    auto recentsSection = addSection("recents");

    auto examples = VRSceneManager::get()->getExamplePaths();
    for (auto p : examples->getPaths() ) examplesSection->addLauncher(p, "", menu, this, true, false, "examples_tab");
    updateTable("examples_tab");

    auto favorites = VRSceneManager::get()->getFavoritePaths();
    //for (string path : favorites) favoritesSection->addLauncher(path, menu, this);
    //updateTable("favorites_tab");


    int i=0;
    for (auto p : favorites->getEntriesByTimestamp()) {
        long ts = p->getTimestamp();
        string t = "";
        if (ts > 0) t = asctime( localtime(&ts) );
        addEntry(p->getPath(), "favorites_tab", false, t, i<2);
        i++;
    }

    if (favorites->size() == 0) setNotebookPage("notebook2", 1);

    updateCb = VRFunction<VRDeviceWeakPtr>::create("GUI_updateDemos", boost::bind(&VRAppManager::update, this) );
    VRGuiSignals::get()->getSignal("scene_changed")->add( updateCb );

    setToolButtonCallback("toolbutton1", sigc::mem_fun(*this, &VRAppManager::on_new_clicked));
    setToolButtonCallback("toolbutton5", sigc::mem_fun(*this, &VRAppManager::on_saveas_clicked));
    setToolButtonCallback("toolbutton28", sigc::mem_fun(*this, &VRAppManager::on_stop_clicked));
    setToolButtonCallback("toolbutton21", sigc::mem_fun(*this, &VRAppManager::on_load_clicked));

    setToolButtonSensitivity("toolbutton4", false); // disable 'save' button on startup
    setToolButtonSensitivity("toolbutton5", false); // disable 'save as' button on startup
    setToolButtonSensitivity("toolbutton28", false); // disable 'stop' button on startup

    setEntryCallback("appSearch", sigc::mem_fun(*this, &VRAppManager::on_search), true); // app search
}

VRAppManager::~VRAppManager() {}

VRAppManagerPtr VRAppManager::create() { return VRAppManagerPtr( new VRAppManager() ); }

VRAppSectionPtr VRAppManager::addSection(string name) {
    auto s = VRAppSection::create(name);
    sections[name] = s;
    return s;
}

bool VRAppManager::on_any_event(GdkEvent* event, VRAppLauncherPtr entry) {
    if (event->type == GDK_BUTTON_PRESS) current_demo = entry;
    return false;
}

void VRAppManager::on_lock_toggle(VRAppLauncherPtr e) {
    e->write_protected = !e->write_protected;
    e->butLock->remove();
    if (e->write_protected) e->butLock->add(*e->imgLock);
    else e->butLock->add(*e->imgUnlock);
    e->butLock->show_all();

    auto scene = VRScene::getCurrent();
    if (scene) scene->setFlag("write_protected", e->write_protected);
}

void VRAppManager::updateTable(string t) {
    Gtk::Table* tab;
    VRGuiBuilder()->get_widget(t, tab);

    int N = 4;
    if (t == "examples_tab") N += sections["examples"]->getSize();
    if (t == "favorites_tab") N += sections["recents"]->getSize() + sections["favorites"]->getSize();
    tab->resize(N*0.5+1, 2);

    int i = 0;
    if (t == "examples_tab") sections["examples"]->fillTable(t, tab, i);
    if (t == "favorites_tab") {
        sections["recents"]->fillTable(t, tab, i);
        sections["favorites"]->fillTable(t, tab, i);
    }

    tab->show();
}

void VRAppManager::clearTable(string t) {
    Gtk::Table* tab;
    VRGuiBuilder()->get_widget(t, tab);
    if (t == "examples_tab") sections["examples"]->clearTable(t, tab);
    if (t == "favorites_tab") {
        sections["recents"]->clearTable(t, tab);
        sections["favorites"]->clearTable(t, tab);
    }
}

void VRAppManager::setGuiState(VRAppLauncherPtr e) {
    bool running = (e == 0) ? noLauncherScene : e->running;
    setVPanedSensitivity("vpaned1", running);
    setNotebookSensitivity("notebook3", running);

    for (auto section : sections) section.second->setGuiState(e, running, noLauncherScene);

    if (e) {
        if (e->widget) e->widget->set_sensitive(true);
        if (running) { if (e->imgPlay) e->imgPlay->set(Gtk::Stock::MEDIA_STOP, Gtk::ICON_SIZE_BUTTON); }
        else { if (e->imgPlay) e->imgPlay->set(Gtk::Stock::MEDIA_PLAY, Gtk::ICON_SIZE_BUTTON); }
    }

    setToolButtonSensitivity("toolbutton4", running); // toggle 'save' button availability
    setToolButtonSensitivity("toolbutton5", running); // toggle 'save as' button availability
    setToolButtonSensitivity("toolbutton28", running); // toggle 'stop' button availability
}

VRAppLauncherPtr VRAppManager::addEntry(string path, string table, bool running, string timestamp, bool recent) {
    if (!exists(path)) return 0;
    clearTable(table);

    VRAppLauncherPtr e = 0;
    if (table == "examples_tab") e = sections["examples"]->addLauncher(path, timestamp, menu, this, true, false, table);
    if (table == "favorites_tab" &&  recent) e =   sections["recents"]->addLauncher(path, timestamp, menu, this, false, true, table);
    if (table == "favorites_tab" && !recent) e = sections["favorites"]->addLauncher(path, timestamp, menu, this, false, true, table);
    if (!e) return 0;

    e->running = running;

    updateTable(table);
    setGuiState(e);
    setNotebookPage("notebook2", 0);
    return e;
}

void VRAppManager::initMenu() {
    menu = new VRGuiContextMenu("DemoMenu");
    menu->appendItem("DemoMenu", "Unpin", sigc::mem_fun(*this, &VRAppManager::on_menu_unpin));
    menu->appendItem("DemoMenu", "Delete", sigc::mem_fun(*this, &VRAppManager::on_menu_delete));
    menu->appendItem("DemoMenu", "Advanced..", sigc::bind<VRAppLauncherPtr>( sigc::mem_fun(*this, &VRAppManager::on_menu_advanced), 0));

    setButtonCallback("button10", sigc::mem_fun(*this, &VRAppManager::on_advanced_cancel));
    setButtonCallback("button26", sigc::mem_fun(*this, &VRAppManager::on_advanced_start));
}

void VRAppManager::on_menu_delete() {
    VRAppLauncherPtr d = current_demo;
    if (!d) return;
    if (d->write_protected == true) return;
    string table = d->table;

    string path = d->path;
    if (!askUser("Delete scene " + path + " (this will remove it completely from disk!)", "Are you sure you want to delete this scene?")) return;
    if (d->running) toggleDemo(d); // close demo if it is running

    clearTable(table);
    if (table == "examples_tab") sections["examples"]->remLauncher(path);
    if (table == "recents_tab") sections["recents"]->remLauncher(path);
    if (table == "favorites_tab") sections["favorites"]->remLauncher(path);

    current_demo.reset();
    remove(path.c_str());
    updateTable(table);
    VRSceneManager::get()->remFavorite(path);
}

void VRAppManager::on_menu_unpin() {
    VRAppLauncherPtr d = current_demo;
    if (!d) return;
    if (d->write_protected == true) return;
    string table = d->table;

    string path = d->path;
    if (!askUser("Forget about " + path + " ?", "")) return;
    if (d->running) toggleDemo(d); // close demo if it is running

    clearTable(table);
    if (table == "examples_tab") sections["examples"]->remLauncher(path);
    if (table == "recents_tab") sections["recents"]->remLauncher(path);
    if (table == "favorites_tab") sections["favorites"]->remLauncher(path);

    current_demo.reset();
    updateTable(table);
    VRSceneManager::get()->remFavorite(path);
}

void VRAppManager::on_menu_advanced(VRAppLauncherPtr e) {
    if (e) current_demo = e;
    setCheckButton("checkbutton34", false);
    setCheckButton("checkbutton36", false);
    showDialog("advanced_start");
}

void VRAppManager::on_advanced_cancel() {
    hideDialog("advanced_start");
}

void VRAppManager::on_advanced_start() {
    bool no_scripts = getCheckButtonState("checkbutton34");
    bool lightweight = getCheckButtonState("checkbutton36");
    hideDialog("advanced_start");
    if (current_demo == 0) return;

    if (lightweight) VRImport::get()->ingoreHeavyRessources(); // just for the next scene

    if (current_demo->running) toggleDemo(current_demo); // close demo if it is running
    toggleDemo(current_demo); // start demo

    auto scene = VRScene::getCurrent();
    if (no_scripts && scene) scene->pauseScripts(true);
}

void VRAppManager::normFileName(string& path) {
    string e = path.substr(path.size()-4, path.size());
    if (e == ".xml" || e == ".pvr") return;
    path += ".pvr";
}

string encryptionKey;
void VRAppManager::on_toggle_encryption(Gtk::CheckButton* b) {
    bool doEncryption = b->get_active();
    encryptionKey = "";
    if (!doEncryption) return;
    encryptionKey = askUserPass("Please enter an encryption key");
}

void VRAppManager::on_diag_save_clicked() { // TODO: check if ending is .pvr
    string path = VRGuiFile::getPath();
    saveScene(path, true, encryptionKey);
    VRSceneManager::get()->addFavorite(path);
    addEntry(path, "favorites_tab", true);
}

void VRAppManager::toggleDemo(VRAppLauncherPtr e) {
    bool run = !e->running;
    VRSceneManager::get()->closeScene();
    if (run) {
        string encryptionKey;
        if (endsWith(e->path, ".pvc")) encryptionKey = askUserPass("Please insert encryption key");
        VRSceneManager::get()->loadScene(e->path, e->write_protected, encryptionKey);
    }
}

void VRAppManager::on_saveas_clicked() {
    auto scene = VRScene::getCurrent();
    if (!scene) return;
    encryptionKey = "";
    VRGuiFile::gotoPath( scene->getWorkdir() );
    VRGuiFile::setCallbacks( sigc::mem_fun(*this, &VRAppManager::on_diag_save_clicked) );
    VRGuiFile::clearFilter();
    VRGuiFile::addFilter("Project", 3, "*.xml", "*.pvr", "*.pvc");
    VRGuiFile::addFilter("All", 1, "*");
    VRGuiFile::open( "Save", Gtk::FILE_CHOOSER_ACTION_SAVE, "Save project as.." );
    VRGuiFile::setSaveasWidget( sigc::mem_fun(*this, &VRAppManager::on_toggle_encryption) );
    VRGuiFile::setFile( scene->getFile() );
}

void VRAppManager::on_stop_clicked() {
    if (noLauncherScene) {
        VRSceneManager::get()->closeScene();
        noLauncherScene = false;
        update();
    }

    if (current_demo && current_demo->running) toggleDemo(current_demo); // close demo if it is running
}

void VRAppManager::on_diag_load_clicked() {
    string path = VRGuiFile::getPath();
    if (!exists(path)) return;
    if (current_demo) if (current_demo->running) toggleDemo(current_demo); // close demo if it is running
    bool loadProject = bool( endsWith(path, ".xml") || endsWith(path, ".pvr") || endsWith(path, ".pvc") );

    if (loadProject) {
        cout << "load project '" << path << "'" << endl;
        auto e = addEntry(path, "favorites_tab", false);
        VRSceneManager::get()->addFavorite(path);
        if (e) toggleDemo(e);
    } else {
        cout << "load model '" << path << "'" << endl;
        VRSceneManager::get()->setWorkdir( getFolderName(path) );
        VRSceneManager::get()->newScene("PolyVR Model Viewer");

        noLauncherScene = true;
        update();

        auto scene = VRScene::getCurrent();
        if (!scene) return;

        scene->setBackground(VRBackground::SKY);

        auto cam = dynamic_pointer_cast<VRCamera>(scene->get("Default"));
        auto light = scene->get("light");
        auto model = VRImport::get()->load(path, light);

        auto bb = model->getBoundingbox();
        if (bb->volume() < 1e-4) return;
        double h = bb->center()[1];
        Vec3d p = Vec3d(0,h,0);
        cam->setTransform(p, Vec3d(0,0,-1), Vec3d(0,1,0));
        cam->focusObject(model);
    }
}

void VRAppManager::on_load_clicked() {
    VRGuiFile::setCallbacks( sigc::mem_fun(*this, &VRAppManager::on_diag_load_clicked) );
    VRGuiFile::gotoPath( g_get_home_dir() );
    VRGuiFile::clearFilter();
    VRGuiFile::addFilter("Project", 3, "*.xml", "*.pvr", "*.pvc");
    VRGuiFile::addFilter("Model", 19, "*.dae", "*.wrl", "*.obj", "*.3ds", "*.3DS", "*.ply", "*.hgt", "*.tif", "*.tiff", "*.pdf", "*.shp", "*.e57", "*.xyz", "*.STEP", "*.STP", "*.step", "*.stp", "*.ifc", "*.dxf");
    VRGuiFile::addFilter("All", 1, "*");
    VRGuiFile::open( "Load", Gtk::FILE_CHOOSER_ACTION_OPEN, "Load project" );
}

void VRAppManager::writeGitignore(string path) {
    ofstream f(path);
    f << ".local_*" << endl;
    f << "core" << endl;
    f << "*.blend1" << endl;
    f << "*~" << endl;
    f << ".treeLods" << endl;
}

void VRAppManager::on_diag_new_clicked() {
    //string path = VRGuiFile::getRelativePath_toOrigin();
    string path = VRGuiFile::getPath();
    if (path == "") return;
    normFileName(path);
    VRSceneManager::get()->newScene(path);
    string gitIgnorePath = getFolderName(path) + "/.gitignore";
    if (!exists(gitIgnorePath)) writeGitignore(gitIgnorePath);
    saveScene(path);
    addEntry(path, "favorites_tab", true);
    VRSceneManager::get()->addFavorite(path);
}

void VRAppManager::on_new_clicked() {
    VRGuiFile::setCallbacks( sigc::mem_fun(*this, &VRAppManager::on_diag_new_clicked) );
    VRGuiFile::gotoPath( g_get_home_dir() );
    VRGuiFile::setFile( "myApp.pvr" );
    VRGuiFile::clearFilter();
    VRGuiFile::open( "Create", Gtk::FILE_CHOOSER_ACTION_SAVE, "Create new project" );
}

void VRAppManager::on_search() {
    string s = getTextEntry("appSearch");
    for (auto section : sections) {
        for (auto launcher : section.second->getLaunchers()) {
            if (s == "") { launcher.second->show(); continue; }
            string name = launcher.first;
            if (contains(name, s, false)) launcher.second->show(); // TODO: extend contains with a case sensitive flag
            else launcher.second->hide();
        }
    }
}

void VRAppManager::update() {
    auto scene = VRScene::getCurrent();
    if (scene == 0) {
        if (current_demo) current_demo->running = false;
        setGuiState(current_demo);
        return;
    }

    string sPath = scene->getPath();
    if (current_demo) {
        if (current_demo->path == sPath) {
            current_demo->running = true;
            setGuiState(current_demo);
            return;
        }
        current_demo->running = false;
        setGuiState(current_demo);
    }

    auto e = sections["recents"]->getLauncher(sPath);
    if (!e) e = sections["examples"]->getLauncher(sPath);
    if (!e) e = sections["favorites"]->getLauncher(sPath);

    if (e) {
        current_demo = e;
        current_demo->running = true;
        setGuiState(current_demo);
    }

    if (noLauncherScene) setGuiState(0);
}

OSG_END_NAMESPACE;


