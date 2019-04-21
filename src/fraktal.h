// Developed by Simen Haugo.
// See LICENSE.txt for copyright and licensing details (standard MIT License).

/*

Index of this file:
(A decent code editor should let you jump quickly to a section or function)

§1 Types and forward declarations
....fEnum
§2 Arrays
....fraktal_create_array
....fraktal_destroy_array
....fraktal_zero_array
....fraktal_to_cpu
....fraktal_array_format
....fraktal_array_size
....fraktal_array_channels
....fraktal_is_valid_array
....fraktal_get_gl_handle
§3 Kernels
....fraktal_create_link
....fraktal_destroy_link
....fraktal_add_link_data
....fraktal_link_kernel
....fraktal_destroy_kernel
....fraktal_load_kernel
....fraktal_use_kernel
....fraktal_run_kernel
§4 Parameters
....fraktal_get_param_offset
....fraktal_param_...
§5 Context management
....fraktal_create_context
....fraktal_share_context
....fraktal_destroy_context
....fraktal_make_context_current
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// FRAKTALAPI is used to declare functions as visible when
// linking against a DLL / shared library / dynamic library.
#if defined(_WIN32) && defined(FRAKTAL_BUILD_DLL)
// We are building fraktal as a Win32 DLL
#define FRAKTALAPI __declspec(dllexport)
#elif defined(__GNUC__) && defined(FRAKTAL_BUILD_DLL)
// We are building fraktal as a shared / dynamic library
#define FRAKTALAPI __attribute__((visibility("default")))
#else
// We are building or calling fraktal as a static library
#define FRAKTALAPI
#endif

//-----------------------------------------------------------------------------
// §1 Types and forward declarations
//-----------------------------------------------------------------------------

typedef int fEnum;
enum fEnum_
{
    // Array access modes
    FRAKTAL_READ_ONLY,
    FRAKTAL_READ_WRITE,

    // Array formats
    FRAKTAL_FLOAT,
    FRAKTAL_UINT8,

    // Texture wrap modes
    FRAKTAL_CLAMP_TO_EDGE,
    FRAKTAL_REPEAT,

    // Texture filter modes
    FRAKTAL_LINEAR,
    FRAKTAL_NEAREST,
};

struct fArray;
struct fKernel;
struct fLinkState;
struct fContext;

//-----------------------------------------------------------------------------
// §2 Arrays
//-----------------------------------------------------------------------------

/*
    Creates a 1D or 2D GPU array of packed float or uint8 vector values
    of the specified dimensions.

    'data'    : An optional pointer to a region in CPU memory used
                to initialize the array. The CPU memory must be a
                contiguous array of packed float or uint8 vector
                values matching the given channels and dimensions.
    'width'   : The number of array values along x.
    'height'  : The number of array values along y. If 0, the array
                is a 1D array, otherwise the array is a 2D array.
    'channels': The number of vector components. Must be 1, 2 or 4.
    'format'  : Must be FRAKTAL_FLOAT or FRAKTAL_UINT8.
    'access'  : Must be FRAKTAL_READ_ONLY or FRAKTAL_READ_WRITE.

    If successful, the function returns a handle to a GPU array that
    can be used as kernel input or an output target (if 'access' is
    not FRAKTAL_READ_ONLY).

    If the 'data' is NULL, the values of the array are uninitialized
    and undefined. The array may be cleared using fraktal_zero_array.
*/
FRAKTALAPI fArray *fraktal_create_array(
    const void *data,
    int width,
    int height,
    int channels,
    fEnum format,
    fEnum access);

/*
    Frees all memory associated with an array.

    If 'a' is NULL the function silently returns.
*/
FRAKTALAPI void fraktal_destroy_array(fArray *a);

/*
    Sets each value in the array to 0. If 'a' has multiple channels,
    each channel is set to the value 0.

    Passing an invalid array or NULL will trigger an assertion.
*/
FRAKTALAPI void fraktal_zero_array(fArray *a);

/*
    Copies the values of a GPU array to a region of memory allocated
    on the CPU. The destination must be of the same size in bytes as
    the GPU array.

    'cpu_memory' must not be NULL and 'a' must be a valid array.
*/
FRAKTALAPI void fraktal_to_cpu(void *cpu_memory, fArray *a);

/*
    These methods return information about an array.
*/
FRAKTALAPI void fraktal_array_size(fArray *a, int *width, int *height);
FRAKTALAPI fEnum fraktal_array_format(fArray *a); // -1 if 'a' is NULL
FRAKTALAPI int fraktal_array_channels(fArray *a); // 0 is 'a' is NULL

/*
    Returns true if the fArray satisfies the following properties:
      * Width is > 0
      * Height is >= 0 (0 means 'a' is a 1D array)
      * Channels is 1, 2 or 4
      * Access mode is among the modes listed in fEnum.
      * Format is among the formats listed in fEnum.
*/
FRAKTALAPI bool fraktal_is_valid_array(fArray *a);

/*
    If the backend uses OpenGL 3.1, the result is a GLuint handle to
    the array's underlying Texture Object, which can be passed to
    glBindTexture. The texture target is either GL_TEXTURE_1D, if
    'a' is a 1D array, or GL_TEXTURE_2D, if 'a' is a 2D array.
*/
FRAKTALAPI unsigned int fraktal_get_gl_handle(fArray *a);

//-----------------------------------------------------------------------------
// §3 Kernels
//-----------------------------------------------------------------------------

/*
    On success, the function returns a handle that should be passed
    to subsequent link_add_file or add_link_data calls. If the call
    is successful, the caller owns the returned fLinkState, which
    should eventually be destroyed with fraktal_destroy_link.
*/
FRAKTALAPI fLinkState *fraktal_create_link();

/*
    Frees memory associated with a linking operation. On return, the
    fLinkState handle is invalidated and should not be used anywhere.

    If 'link' is NULL the function silently returns.
*/
FRAKTALAPI void fraktal_destroy_link(fLinkState *link);

/*
    'link': Obtained from fraktal_create_link.
    'data': A pointer to a buffer containing kernel source. Must
            be NULL-terminated.
    'size': Length of input data in bytes (excluding NULL-terminator).
            0 can be passed if the input is a NULL-terminated string.
    'name': An optional name for this input in log messages.

    No references are kept to 'data' (it can safely be freed afterward).
*/
FRAKTALAPI bool fraktal_add_link_data(
    fLinkState *link,
    const void *data,
    size_t size,
    const char *name);

/*
    'link': Obtained from fraktal_create_link.
    'path': A NULL-terminated path to a file containing kernel source.

    This method is equivalent to calling add_link_data on the contents
    of the file.
*/
FRAKTALAPI bool fraktal_add_link_file(fLinkState *link, const char *path);

/*
    On success, the method returns a fKernel handle required in all
    kernel-specific operations, such as execution, setting parameters,
    or obtaining information on active parameters.

    If the call is successful, the caller owns the returned fKernel,
    which should eventually be destroyed with fraktal_destroy_kernel.
*/
FRAKTALAPI fKernel *fraktal_link_kernel(fLinkState *link);

/*
    Frees memory associated with a kernel. On return, the fKernel handle
    is invalidated and should not be used anywhere.

    If NULL is passed the method silently returns.
*/
FRAKTALAPI void fraktal_destroy_kernel(fKernel *f);

/*
    'path': A NULL-terminated path to a file containing kernel source.

    This method is equivalent to calling
        fLinkState *link = fraktal_link_create();
        fraktal_link_add_file(link, path);
        fKernel *result = fraktal_link_kernel(link);
*/
FRAKTALAPI fKernel *fraktal_load_kernel(const char *path);

/*
    Calling this function modifies the GPU state of the current context
    as required by fraktal_run_kernel and fraktal_param* functions. The
    state should be restored by calling this function with NULL.

    A kernel can be run multiple times while in use. Switching kernels
    does not require the state to be restored, for example:
      fraktal_use_kernel(a);
      fraktal_run_kernel(a);
      fraktal_use_kernel(b);
      fraktal_run_kernel(b);
      fraktal_use_kernel(NULL);
    runs two kernels 'a' and 'b' and properly restores the GPU state.
*/
FRAKTALAPI void fraktal_use_kernel(fKernel *f);

/*
    Launches a number of concurrent GPU threads each running the current
    kernel (set by fraktal_use_kernel) and adds the results to 'out'.

    The number of threads is determined from the dimensions of 'out':
    * A 1D array of width w launches a sequence of threads with indices
      [0, 1, ..., w-1].

    * A 2D array of dimensions (w,h) launches a 2D grid of threads with
      indices [0, w-1] x [0, h-1].

    The 1D or 2D index of the executing thread can be used to control
    the kernel function on a per-thread basis, e.g. loading values from
    different locations in memory or running different instructions. A
    common application is to process values in an array, for example:

        #param(array2D, float, data)
        #out(float, y)

        void main() {
            float x = loadArray1D(data, globalIdx.x);
            y = x*x;
        }

    The index is accessible using the built-in keyword 'globalIdx.xy'.

    Results are **added** to the values in 'out'. The array may be
    cleared to zero using fraktal_zero_array(out).
*/
FRAKTALAPI void fraktal_run_kernel(fArray *out);

//-----------------------------------------------------------------------------
// §4 Parameters
//-----------------------------------------------------------------------------

/*
    The value -1 is returned if 'name' refers to a non-existent
    or unused parameter.
*/
FRAKTALAPI int fraktal_get_param_offset(fKernel *f, const char *name);

FRAKTALAPI void fraktal_param_1f(int offset, float x);
FRAKTALAPI void fraktal_param_2f(int offset, float x, float y);
FRAKTALAPI void fraktal_param_3f(int offset, float x, float y, float z);
FRAKTALAPI void fraktal_param_4f(int offset, float x, float y, float z, float w);
FRAKTALAPI void fraktal_param_1i(int offset, int x);
FRAKTALAPI void fraktal_param_2i(int offset, int x, int y);
FRAKTALAPI void fraktal_param_3i(int offset, int x, int y, int z);
FRAKTALAPI void fraktal_param_4i(int offset, int x, int y, int z, int w);
FRAKTALAPI void fraktal_param_matrix4f(int offset, float m[4*4]);
FRAKTALAPI void fraktal_param_transpose_matrix4f(int offset, float m[4*4]);
FRAKTALAPI void fraktal_param_array(int offset, int tex_unit, fArray *a);

//-----------------------------------------------------------------------------
// §5 Context management
//-----------------------------------------------------------------------------

/*
    fraktal requires a GPU context to be bound to the calling thread
    in order to access GPU resources. By default, fraktal will auto-
    matically create a context for you. However, if you are using
    fraktal together with other libraries that also use the GPU,
    you have two options:

      1. Share contexts
      2. Use a separate context for fraktal and make_context_current
         to attach or detach the fraktal context.

  1. Share contexts:

    Sharing contexts makes GPU memory used in fraktal visible to
    outside libraries and vice versa. This can be useful if you are
    passing results from fraktal to a library, or data from a library
    to fraktal, as it avoids an unecessary copy from GPU to CPU memory
    and back; the memory can remain on the GPU at all times.

    For example:

        - You are writing a GUI application and using GLFW or SDL to
          create an OpenGL context for your application, and want to
          share this context with fraktal.

        - You are training a deep neural network in PyTorch or Tensorflow
          and want to share data between them.

      Calling fraktal_share_context before any other functions will tell
    fraktal that its context is managed externally (i.e. by you). You
    are thus responsible for ensuring the GPU context is current on the
    thread whenever a fraktal function is called.

  2. Use a separate context for fraktal:

    If you want fraktal to use its own context you need to ensure that
    it is always 'current' on the thread on which fraktal functions are
    called. For example, if you juggle between two GPU libraries on the
    same thread, it's possible that one library makes its context current
    and the other never reattaches its context, resulting in an error
    or undefined behavior when it subsequently attempts to use the GPU.

      Note that if fraktal is the only library that accesses the GPU on
    a given thread, you do not need to worry about making the context
    current and you can rely on the default behavior described at the
    top of this section.

      Otherwise you need to explicitly create a context before calling
    any other fraktal functions, and make this context current when
    calling fraktal functions, and detach it to relinquish control to
    a different library (we automatically make the previous context
    current, thus being compatible with libraries that do not auto-
    matically try to make their context current).
*/

FRAKTALAPI fContext *fraktal_create_context();
FRAKTALAPI void fraktal_share_context();
FRAKTALAPI void fraktal_destroy_context(fContext *ctx);
FRAKTALAPI void fraktal_make_context_current(fContext *ctx);

#ifdef __cplusplus
}
#endif
