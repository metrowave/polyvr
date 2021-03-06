#ifndef CEF_H_INCLUDED
#define CEF_H_INCLUDED

#include <OpenSG/OSGConfig.h>

#include "include/cef_app.h"
#include "include/cef_client.h"
#include "include/cef_render_handler.h"

#include "core/utils/VRFunctionFwd.h"
#include "core/objects/VRObjectFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRDevice;

class CEF_handler : public CefRenderHandler {
    private:
        VRTexturePtr image = 0;
        int width = 1024;
        int height = 1024;

    public:
        CEF_handler();
        ~CEF_handler();

        bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect);
        void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects, const void* buffer, int width, int height);
        VRTexturePtr getImage();
        void resize(int resolution, float aspect);

        IMPLEMENT_REFCOUNTING(CEF_handler);
};

class CEF_client : public CefClient {
    private:
        CefRefPtr<CEF_handler> handler;

    public:
        CEF_client();
        ~CEF_client();

        CefRefPtr<CEF_handler> getHandler();
        CefRefPtr<CefRenderHandler> GetRenderHandler();

        IMPLEMENT_REFCOUNTING(CEF_client);
};

class CEF {
    private:
        CefRefPtr<CefBrowser> browser;
        CefRefPtr<CEF_client> client;

        string site;
        VRMaterialWeakPtr mat;
        VRObjectWeakPtr obj;
        int resolution = 1024;
        float aspect = 1;
        bool init = false;
        bool focus = false;

        VRUpdateCbPtr update_callback;
        map<VRDevice*, VRDeviceCbPtr> mouse_dev_callback;
        map<VRDevice*, VRUpdateCbPtr> mouse_move_callback;
        //VRDeviceCbPtr mouse_dev_callback;
        VRDeviceCbPtr keyboard_dev_callback;

        void global_initiate();
        void initiate();
        void update();

        void mouse(int lb, int rb, int wu, int wd, VRDeviceWeakPtr dev);
        void mouse_move(VRDeviceWeakPtr dev);
        void keyboard(VRDeviceWeakPtr dev);

        CEF();
    public:
        ~CEF();
        static shared_ptr<CEF> create();

        void setResolution(float a);
        void setAspectRatio(float a);

        void setMaterial(VRMaterialPtr mat);
        void addMouse(VRDevicePtr dev, VRObjectPtr obj, int lb, int rb, int wu, int wd);
        void addKeyboard(VRDevicePtr dev);

        void open(string site);
        void reload();
        string getSite();
        void resize();

        static void reloadScripts(string path);
        static void shutdown();
};

typedef shared_ptr<CEF> CEFPtr;

OSG_END_NAMESPACE;

#endif // CAVEKEEPER_H_INCLUDED
