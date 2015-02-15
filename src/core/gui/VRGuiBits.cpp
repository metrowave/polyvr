#include "VRGuiBits.h"

#include <gtkmm/combobox.h>
#include <gtkmm/liststore.h>
#include <gtkmm/paned.h>
#include <gtkmm/aboutdialog.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/frame.h>
#include <gtkmm/notebook.h>
#include <gtkmm/separator.h>
#include <gtkmm/table.h>
#include <gtkmm/toggletoolbutton.h>
#include <gtkmm/toolbar.h>
#include <vte-0.0/vte/vte.h>
#include <iostream>
//#include <boost/locale.hpp>
#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/windows/VRView.h"
#include "core/utils/VRInternalMonitor.h"
#include "core/utils/VRVisualLayer.h"
#include "core/scene/VRSceneLoader.h"
#include "VRGuiUtils.h"
#include "VRGuiSignals.h"
#include "VRGuiFile.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"
#include "core/scene/VRScene.h"
#include "PolyVR.h"
#include "core/objects/VRCamera.h"
#include "core/tools/VRRecorder.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiSetup_ViewOptsColumns : public Gtk::TreeModelColumnRecord {
    public:
        VRGuiSetup_ViewOptsColumns() { add(option); add(state); }

        Gtk::TreeModelColumn<Glib::ustring> option;
        Gtk::TreeModelColumn<gint> state;
};


// --------------------------
// ---------SIGNALS----------
// --------------------------

void VRGuiBits::on_view_option_toggle(VRVisualLayer* l, Gtk::ToggleToolButton* tb) {
    l->setVisibility( tb->get_active() );
}

void VRGuiBits_on_camera_changed(GtkComboBox* cb, gpointer data) {
    int i = gtk_combo_box_get_active(cb);
    VRScene* scene = VRSceneManager::getCurrent();
    scene->setActiveCamera(i);

    VRGuiSignals::get()->getSignal("camera_changed")->trigger();
}

void VRGuiBits_on_navigation_changed(GtkComboBox* cb, gpointer data) {
    VRScene* scene = VRSceneManager::getCurrent();
    if (scene == 0) return;

    char* c = gtk_combo_box_get_active_text(cb);
    if (c == 0) return;
    string name = string(c);
    scene->setActiveNavigation(name);
}

void VRGuiBits_on_new_cancel_clicked(GtkButton* cb, gpointer data) {
    Gtk::Window* dialog;
    VRGuiBuilder()->get_widget("NewProject", dialog);
    dialog->hide();
}

void VRGuiBits_on_save_clicked(GtkButton* cb, gpointer data) {
    saveScene();
}

void VRGuiBits_on_quit_clicked(GtkButton* cb, gpointer data) {
    exitPolyVR();
}

void VRGuiBits_on_about_clicked(GtkButton* cb, gpointer data) {
    Gtk::AboutDialog* diag;
    VRGuiBuilder()->get_widget("aboutdialog1", diag);
    diag->run();
}

void VRGuiBits_on_internal_clicked(GtkButton* cb, gpointer data) {
    Gtk::Dialog* diag;
    VRGuiBuilder()->get_widget("dialog2", diag);
    cout << "\nPRINT NAME MAP" << flush;
    VRName::printNameDict();
    diag->run();
}

void VRGuiBits_on_internal_close_clicked(GtkButton* cb, gpointer data) {
    Gtk::Dialog* diag;
    VRGuiBuilder()->get_widget("dialog2", diag);
    diag->hide();
}

void VRGuiBits_on_internal_update(int i) {
    VRInternalMonitor* mnr = VRInternalMonitor::get();
    Glib::RefPtr<Gtk::ListStore> store = Glib::RefPtr<Gtk::ListStore>::cast_static(VRGuiBuilder()->get_object("liststore4"));
    store->clear();

    map<string, string> vars = mnr->getVariables();
    map<string, string>::iterator itr;
    for (itr = vars.begin(); itr != vars.end(); itr++) {
        Gtk::ListStore::Row row = *store->append();
        gtk_list_store_set (store->gobj(), row.gobj(), 0, itr->first.c_str(), -1);
        gtk_list_store_set (store->gobj(), row.gobj(), 1, itr->second.c_str(), -1);
    }
}

// --------------------------
// ---------Main-------------
// --------------------------

VteTerminal* terminal;

void VRGuiBits::write_to_terminal(string s) {
    for (int i=s.size(); i>=0; i--)
        if (s[i] == '\n') s.insert(i, "\r");

    vte_terminal_feed(terminal, s.c_str(), s.size());
}

void VRGuiBits::hideAbout(int i) {
    Gtk::AboutDialog* diag;
    VRGuiBuilder()->get_widget("aboutdialog1", diag);
    diag->hide();
}

bool VRGuiBits::toggleWidgets(GdkEventKey* k) {
    if (k->keyval != 65481) return false;
    static bool fs = false;
    fs = !fs;

    Gtk::Window* win; VRGuiBuilder()->get_widget("window1", win);
    Gtk::Separator* hs1; VRGuiBuilder()->get_widget("hseparator1", hs1);
    Gtk::Table* tab; VRGuiBuilder()->get_widget("table20", tab);
    Gtk::Notebook* nb1; VRGuiBuilder()->get_widget("notebook1", nb1);
    Gtk::Box* hb1; VRGuiBuilder()->get_widget("hbox1", hb1);

    if (fs) {
        nb1->hide();
        hb1->hide();
        tab->hide();
        hs1->hide();
        gtk_widget_hide(term_box);
    } else win->show_all();
    return true;
}

bool VRGuiBits::toggleFullscreen(GdkEventKey* k) {
    if (k->keyval != 65480) return false;
    static bool fs = false;
    fs = !fs;

    Gtk::Window* win; VRGuiBuilder()->get_widget("window1", win);
    if (fs) win->fullscreen();
    else win->unfullscreen();
    return true;
}

bool VRGuiBits::toggleStereo(GdkEventKey* k) {
    if (k->keyval != 65479) return false;
    VRView* v = VRSetupManager::getCurrent()->getView(0);
    if (v == 0) return false;

    bool b = v->isStereo();
    v->setStereo(!b);
    return true;
}

void VRGuiBits::toggleDock() {
    Gtk::ToggleToolButton* tbut;
    VRGuiBuilder()->get_widget("togglebutton1", tbut);
    bool a = tbut->get_active();

    static Gtk::Window* win = 0;
    Gtk::VBox* box;
    Gtk::VPaned* pan;
    VRGuiBuilder()->get_widget("vbox5", box);
    VRGuiBuilder()->get_widget("vpaned1", pan);

    if(a) {
        win = new Gtk::Window();
        win->set_title("PolyVR 3D View");
        win->set_default_size(400, 400);
        box->reparent(*win);
        win->show_all();
    } else if(win) {
        box->reparent(*pan);
        pan->show_all();
        delete win;
    }

    //TODO: reset changelist to redraw everything!
}

VRGuiBits::VRGuiBits() {
    setComboboxCallback("combobox4", VRGuiBits_on_camera_changed);
    setComboboxCallback("combobox9", VRGuiBits_on_navigation_changed);

    setToolButtonCallback("toolbutton4", VRGuiBits_on_save_clicked);
    setToolButtonCallback("toolbutton3", VRGuiBits_on_quit_clicked);
    setToolButtonCallback("toolbutton17", VRGuiBits_on_about_clicked);
    setToolButtonCallback("toolbutton18", VRGuiBits_on_internal_clicked);

    setButtonCallback("button14", VRGuiBits_on_new_cancel_clicked);
    setButtonCallback("button21", VRGuiBits_on_internal_close_clicked);

    setToolButtonCallback("togglebutton1", sigc::mem_fun(*this, &VRGuiBits::toggleDock) );

    setLabel("label24", "Project: None");

    // recorder
    recorder = new VRRecorder();
    recorder->setView(0);
    recorder_visual_layer = new VRVisualLayer("recorder", "recorder.png");
    recorder_visual_layer->setCallback( recorder->getToggleCallback() );

    // About Dialog
    Gtk::AboutDialog* diag;
    VRGuiBuilder()->get_widget("aboutdialog1", diag);
    diag->signal_response().connect( sigc::mem_fun(*this, &VRGuiBits::hideAbout) );
    ifstream f("ressources/gui/authors");
    vector<string> authors;
    for (string line; getline(f, line); ) authors.push_back(line);
    f.close();
    diag->set_authors(authors);

    // window fullscreen
    Gtk::Window* win;
    VRGuiBuilder()->get_widget("window1", win);
    win->signal_key_press_event().connect( sigc::mem_fun(*this, &VRGuiBits::toggleStereo) );
    win->signal_key_press_event().connect( sigc::mem_fun(*this, &VRGuiBits::toggleFullscreen) );
    win->signal_key_press_event().connect( sigc::mem_fun(*this, &VRGuiBits::toggleWidgets) );

    // VTE

    GtkWidget* vte = vte_terminal_new();
    terminal = VTE_TERMINAL (vte);

    vte_terminal_set_background_transparent(terminal, FALSE);
    vte_terminal_set_scrollback_lines(terminal, -1);
    vte_terminal_set_size(terminal, 80, 20);

    char** argv=NULL;
    g_shell_parse_argv("/bin/bash", NULL, &argv, NULL);
    vte_terminal_fork_command_full(terminal, VTE_PTY_DEFAULT, NULL, argv, NULL, GSpawnFlags(0), NULL, NULL, NULL, NULL);

    vte_terminal_set_scroll_on_keystroke(terminal, TRUE);
    gtk_widget_set_size_request(vte, -1, 100);

    GtkWidget* scrollbar = gtk_vscrollbar_new(vte_terminal_get_adjustment(terminal));
    term_box = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(term_box), vte, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(term_box), scrollbar, FALSE, FALSE, 0);

    Gtk::VPaned* paned;
    VRGuiBuilder()->get_widget("vpaned1", paned);
    gtk_paned_pack2(GTK_PANED (paned->gobj()), term_box, FALSE, FALSE);

    vte_terminal_get_emulation(VTE_TERMINAL (vte));

    gtk_widget_show (term_box);
    gtk_widget_show (vte);

    //int pos = paned->property_max_position () - 100;
    //paned->set_position(pos);

    VRFunction<int>* fkt = new VRFunction<int>( "IntMonitor_guiUpdate", VRGuiBits_on_internal_update );
    VRSceneManager::get()->addUpdateFkt(fkt);

    // view options
    //setComboboxCallback("combobox20", VRGuiBits_on_viewoption_changed);
    updateVisualLayer();
}

void VRGuiBits::updateVisualLayer() {
    Gtk::Toolbar* bar;
    VRGuiBuilder()->get_widget("toolbar6", bar);
    for (auto c : bar->get_children()) bar->remove(*c);

    for (auto l : VRVisualLayer::getLayers()) {
        VRVisualLayer* ly = VRVisualLayer::getLayer(l);
        Gtk::ToggleToolButton* tb = Gtk::manage( new Gtk::ToggleToolButton() );
        Gtk::Image* icon = Gtk::manage( new Gtk::Image() );

        sigc::slot<void> slot = sigc::bind<VRVisualLayer*, Gtk::ToggleToolButton*>( sigc::mem_fun(*this, &VRGuiBits::on_view_option_toggle), ly, tb);
        bar->append(*tb, slot);

        string icon_path = VRSceneManager::get()->getOriginalWorkdir() + "/ressources/gui/" + ly->getIconName();
        icon->set(icon_path);
        Glib::RefPtr<Gdk::Pixbuf> pbuf = icon->get_pixbuf();
        if (pbuf) {
            pbuf = pbuf->scale_simple(24, 24, Gdk::INTERP_BILINEAR);
            icon->set(pbuf);
            tb->set_icon_widget(*icon);
        }
    }

    bar->show_all();
}

void VRGuiBits::update() { // scene changed
    VRScene* scene = VRSceneManager::getCurrent();
    setLabel("label24", "Project: None");
    if (scene == 0) return;

    fillStringListstore("cameras", scene->getCameraNames());
    fillStringListstore("nav_presets", scene->getNavigationNames());

    setCombobox("combobox4", scene->getActiveCameraIndex());
    setCombobox("combobox9", getListStorePos( "nav_presets", scene->getActiveNavigation() ) );

    // update setup and project label
    cout << " now running: " << scene->getName() << endl;
    setLabel("label24", "Project: " + scene->getName());

    updateVisualLayer();
}

OSG_END_NAMESPACE;
