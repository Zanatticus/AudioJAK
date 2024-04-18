#include "spectrogram.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <python3.11/Python.h>
#include <png.h>

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
void initSpectrograph(int len)
{
    Py_Initialize();
    set_python_module_path(".");
    set_python_module_path("./include");
    /*
    Py_Initialize();
    set_python_module_path(".");
    set_python_module_path("./include");
    //set_python_module_path("/home/jared/.local/lib/python3.11/site-packages");

    sgram = PyImport_ImportModule("spectrogram");
    checkPyError(sgram);
    plotfxn = PyObject_GetAttrString(sgram, "plot_spectrogram");
    checkPyError(plotfxn);
*/
    //args = PyTuple_New(5);
    //sampleList = PyList_New(len);
}

/* Stops spectrogram */
void stopSpectrogram()
{
    //Py_DECREF(sgram);
    //Py_DECREF(plotfxn);
    Py_Finalize();
}

void read_png_file(const char* filename, png_uint_32* width, png_uint_32* height, uint32_t *pixels) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Error: File %s could not be opened for reading\n", filename);
        //return NULL;
    }

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(fp);
        fprintf(stderr, "Error: png_create_read_struct failed\n");
        //return NULL;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(fp);
        fprintf(stderr, "Error: png_create_info_struct failed\n");
        return;// NULL;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        fprintf(stderr, "Error: Error during png reading\n");
        return;// NULL;
    }

    png_init_io(png_ptr, fp);
    png_read_info(png_ptr, info_ptr);

    int bit_depth, color_type;
    png_get_IHDR(png_ptr, info_ptr, width, height, &bit_depth, &color_type, NULL, NULL, NULL);

    if (color_type != PNG_COLOR_TYPE_RGBA) {
        fprintf(stderr, "Error: Only PNG files with RGBA color type are supported\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return;// NULL;
    }

    png_bytep* row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * (*height));
    for (int y = 0; y < *height; y++) {
        row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png_ptr, info_ptr));
    }

    png_read_image(png_ptr, row_pointers);

    //uint32_t* pixels = (uint32_t*)malloc(sizeof(uint32_t) * (*width) * (*height));

    for (int y = 0; y < *height; y++) {
        png_bytep row = row_pointers[y];
        for (int x = 0; x < *width; x++) {
            png_bytep px = &(row[x * 4]);
            uint32_t pixel = ((uint32_t)px[0] << 24) | ((uint32_t)px[1] << 16) | ((uint32_t)px[2] << 8) | (uint32_t)px[3];
            pixels[y * (*width) + x] = pixel;
        }
    }

    for (int y = 0; y < *height; y++) {
        free(row_pointers[y]);
    }
    free(row_pointers);

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);

    //return pixels;
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


    png_uint_32 ow=w, oh=h;
    read_png_file("spec.png", &ow, &oh, output);

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