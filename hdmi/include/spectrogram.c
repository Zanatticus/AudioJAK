#include "spectrogram.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#if( __has_include("python3.10/Python.h"))
#include <python3.10/Python.h>
#else
#include <python3.11/Python.h>
#endif
//#include <png.h>

PyObject *sgram;
PyObject *plotfxn;


int set_python_module_path(const char* module_path) {
    // Get a reference to the sys.path list
    PyObject* sys_path = PySys_GetObject("path");
    if (sys_path == NULL) {
        fprintf(stderr, "Failed to get sys.path\n");
        return 1;
    }

    // Append the specified module path to sys.path
    if (PyList_Append(sys_path, PyUnicode_FromString(module_path)) != 0) {
        fprintf(stderr, "Failed to append directory to sys.path\n");
        return 1;
    }

    //PyObject_Print(sys_path, stdout, Py_PRINT_RAW);
    //printf("\n");

    return 0;
}

void checkPyError(PyObject* pObject) {
    if (pObject == NULL) {
        PyErr_Print();
    }
}

/* Initialize the spectrogram */
int initSpectrograph(int len)
{
    printf("Initializing\n");
    Py_Initialize();
    set_python_module_path(".");
    set_python_module_path("./include");
    set_python_module_path("./modules");
    sgram = PyImport_ImportModule("spectrogram"); //Set to NULL to not use spectrogram
    if(sgram != NULL)
    {
        printf("Using spectrogam\n");
        return 0;
    }
    else
    {
        FILE *original_stderr = stderr;
        freopen("/dev/null", "w", stderr);
        Py_Finalize();
        freopen(NULL, "w", original_stderr);
        printf("Not using spectrogram\n");
        return -1;
    }
}

/* Stops spectrogram */
void stopSpectrogram()
{
    Py_Finalize();
}

void read_integers_from_file(const char* filepath, uint32_t** integers, size_t* count) {
    FILE* file = fopen(filepath, "rb");
    if (file == NULL) {
        // Handle file open error
        return;
    }
    
    // Get the file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Calculate the number of integers in the file
    *count = file_size / sizeof(uint32_t);

    if (*integers == NULL) {
        // Handle memory allocation error
        fclose(file);
        return;
    }

    // Read the integers from the file
    fread(*integers, sizeof(uint32_t), *count, file);

    // Close the file
    fclose(file);
}

/* Get the pixels of the spectrogram */
//void getSpectrogram(uint32_t *samples, int sampleRate, int len, uint32_t *output, int w, int h, uint32_t backgroundColor)
void getSpectrogram(char* filename, uint32_t *output, int w, int h, uint32_t backgroundColor)
{

    
    //set_python_module_path("/home/jared/.local/lib/python3.11/site-packages");

    sgram = PyImport_ImportModule("spectrogram");
    checkPyError(sgram);
    plotfxn = PyObject_GetAttrString(sgram, "plot_spectrogram_filename");
    checkPyError(plotfxn);

    PyObject *args = PyTuple_New(4);
    PyObject *fn = PyUnicode_FromString(filename);
    /*
    PyObject *sampleList = PyList_New(len);

    for(Py_ssize_t i = 0; i < len; i++)
    {
        //printf("%ld, \n", i);
        uint32_t samp = samples[i];
        PyObject *pItem = PyLong_FromUnsignedLong(samp);
        //PyList_Append(sampleList, pItem);
        if(PyList_SetItem(sampleList, i, pItem) != 0)
            PyErr_Print();
    }
    
    printf("finished creating list\n");

    PyObject *ww = PyLong_FromLong(w);
    PyObject *hh = PyLong_FromLong(h);
    PyObject *sr = PyLong_FromLong(sampleRate);
    PyObject *bgc = PyLong_FromUnsignedLong(backgroundColor);
    */
    //PyObject *fxn_out = PyList_New(w * h);
    PyObject *ww = PyLong_FromLong(w);
    PyObject *hh = PyLong_FromLong(h);
    //PyObject *sr = PyLong_FromLong(sampleRate);
    PyObject *bgc = PyLong_FromUnsignedLong(backgroundColor);

    PyTuple_SetItem(args, 0, fn);
    //PyTuple_SetItem(args, 0, sampleList);
    //PyTuple_SetItem(args, 1, sr);
    PyTuple_SetItem(args, 1, hh);
    PyTuple_SetItem(args, 2, ww);
    PyTuple_SetItem(args, 3, bgc);

    //printf("Calling function\n");

    PyObject *result = PyObject_CallObject(plotfxn, args);
    checkPyError(result);
    /*
    printf("Getting results\n");
    
    int cnt = 0;
    for(int j = 0; j < h; j++)
    {
        for(int i = 0; i < w; i++)
        {
            PyObject *num = PyList_GetItem(result, (Py_ssize_t)(j * w + i));
            checkPyError(num);
            uint32_t c = (uint32_t)PyLong_AsUnsignedLong(num);
            Py_DECREF(num);
            output[j * w + i] = c;
            cnt++;
        }
    }
    */


    //png_uint_32 ow=w, oh=h;
    size_t numPixels = w * h;
    //read_png_file("spec.png", &ow, &oh, output);
    read_integers_from_file("spec.data", &output, &numPixels);

    //printf("w: %d/%d, h: %d/%d\n", w, ow, h, oh);

    

    //printf("Decrefing\n");

    //Py_DECREF(ww);
    //Py_DECREF(hh);
    //Py_DECREF(sr);
    //Py_DECREF(bgc);
    Py_DECREF(args);
    //Py_DECREF(sampleList);
    //Py_DECREF(fxn_out);
    //Py_DECREF(result);
    //printf("Done Decrefing\n");
    
    //printf("Done Decrefing2\n");
}