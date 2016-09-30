#include "VRPyRealWorld.h"
#include "../../core/scripting/VRPyTransform.h"
#include "../../core/scripting/VRPyBaseT.h"

template<> PyTypeObject VRPyBaseT<RealWorld>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.RealWorld",             /*tp_name*/
    sizeof(VRPyRealWorld),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "VRRealWorld binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyRealWorld::methods,             /* tp_methods */
    VRPyRealWorld::members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New_toZero,                 /* tp_new */
};

PyMemberDef VRPyRealWorld::members[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyRealWorld::methods[] = {
    {"init", (PyCFunction)VRPyRealWorld::initWorld, METH_VARARGS, "Init real obj - init(root)" },
    {"update", (PyCFunction)VRPyRealWorld::update, METH_VARARGS, "Update real obj - update([x,y,z])" },
    {"enableModule", (PyCFunction)VRPyRealWorld::enableModule, METH_VARARGS, "Enable a module - enableModule(str, bool threaded, bool physicalized)" },
    {"disableModule", (PyCFunction)VRPyRealWorld::disableModule, METH_VARARGS, "Disable a module - disableModule(str)" },
    {"configure", (PyCFunction)VRPyRealWorld::configure, METH_VARARGS, "Configure a variable - configure( str var, str value )"
            "\n\tpossible variables: [ 'CHUNKS_PATH' ]"},
    {NULL}  /* Sentinel */
};

PyObject* VRPyRealWorld::initWorld(VRPyRealWorld* self, PyObject* args) {
    VRPyObject* child = 0;
    PyObject* origin = 0;
    if (! PyArg_ParseTuple(args, "OO", &child, &origin)) return NULL;
    if (self->obj == 0) self->obj = new RealWorld( child->objPtr, parseVec2fList(origin) );
    Py_RETURN_TRUE;
}

PyObject* VRPyRealWorld::configure(VRPyRealWorld* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyRealWorld::configure, obj is invalid"); return NULL; }
    const char* var = 0;
    const char* val = 0;
    if (! PyArg_ParseTuple(args, "ss", &var, &val)) return NULL;
    self->obj->configure( var, val );
    Py_RETURN_TRUE;
}

PyObject* VRPyRealWorld::update(VRPyRealWorld* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyRealWorld::physicalize, obj is invalid"); return NULL; }
    self->obj->update( parseVec3f(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyRealWorld::enableModule(VRPyRealWorld* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyRealWorld::enableModule, obj is invalid"); return NULL; }
    const char* name;
    int t, p;
    if (! PyArg_ParseTuple(args, "sii", (char*)&name, &t, &p)) return NULL;
    self->obj->enableModule( name, true, t, p );
    Py_RETURN_TRUE;
}

PyObject* VRPyRealWorld::disableModule(VRPyRealWorld* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "VRPyRealWorld::disableModule, obj is invalid"); return NULL; }
    self->obj->enableModule( parseString(args), false, false, false );
    Py_RETURN_TRUE;
}
