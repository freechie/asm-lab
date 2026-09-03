#define PY_SSIZE_T_CLEAN
#include <Python.h>

/* Tiny C extension. Same a+b, no interpreter in the way. */

static PyObject *addmod_add(PyObject *self, PyObject *args) {
    long a, b;
    (void)self;
    if (!PyArg_ParseTuple(args, "ll", &a, &b)) return NULL;
    return PyLong_FromLong(a + b);
}

static PyMethodDef methods[] = {
    {"add", addmod_add, METH_VARARGS, "add two ints in C"},
    {NULL, NULL, 0, NULL},
};

static struct PyModuleDef module = {
    PyModuleDef_HEAD_INIT,
    "addmod",
    NULL,
    -1,
    methods,
};

PyMODINIT_FUNC PyInit_addmod(void) { return PyModule_Create(&module); }
