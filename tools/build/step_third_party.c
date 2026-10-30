/* Vendored libraries: Monocypher, miniaudio (sound routing) and a trimmed raylib (no audio, no 3D models). */
#include "bld.h"

#define RL "third_party/raylib/src"

/* Optimisation, target defines and (for our own code) warnings. */
void base_flags(strlist *f, bool warnings)
{
    if (B.debug) {
        sl_pushn(f, "-O0", "-g", (char *)NULL);
    } else {
        sl_pushn(f, "-O2", (char *)NULL);
    }
    if (B.os == OS_WINDOWS) {
        sl_pushn(f, "-D__USE_MINGW_ANSI_STDIO=1", "-DWIN32_LEAN_AND_MEAN", "-D_WIN32_WINNT=0x0A00", (char *)NULL);
    }
    if (!warnings) {
        sl_push(f, "-w");
    } else {
        sl_pushn(f, "-Wall", "-Wextra", (char *)NULL);
        if (B.werror) {
            sl_push(f, "-Werror");
        }
    }
}

/* Include paths and defines that code using raylib/raygui needs. */
void raylib_public_flags(strlist *f)
{
    sl_pushn(f, "-I" RL, "-Ithird_party/raygui", "-DPLATFORM_DESKTOP", "-DGRAPHICS_API_OPENGL_33", (char *)NULL);
}

static bool monocypher(strlist *objs)
{
    unit u = {.name = "monocypher"};
    base_flags(&u.cflags, false);
    sl_pushn(&u.srcs, "third_party/monocypher/monocypher.c", "third_party/monocypher/monocypher-ed25519.c",
             (char *)NULL);
    bool ok = unit_compile(&u);
    sl_extend(objs, &u.objs);
    return ok;
}

/* Only devices: no decoders, encoders, resource manager or node graph. */
void miniaudio_flags(strlist *f)
{
    sl_pushn(f, "-Ithird_party/miniaudio", "-DMA_NO_DECODING", "-DMA_NO_ENCODING", "-DMA_NO_GENERATION",
             "-DMA_NO_RESOURCE_MANAGER", "-DMA_NO_NODE_GRAPH", "-DMA_NO_ENGINE", (char *)NULL);
}

static bool miniaudio(strlist *objs)
{
    unit u = {.name = "miniaudio"};
    base_flags(&u.cflags, false);
    miniaudio_flags(&u.cflags);
    if (B.os == OS_LINUX) {
        sl_push(&u.cflags, "-D_GNU_SOURCE");
    }
    sl_push(&u.srcs, "third_party/miniaudio/miniaudio.c");
    bool ok = unit_compile(&u);
    sl_extend(objs, &u.objs);
    return ok;
}

static void raylib_flags(strlist *f)
{
    base_flags(f, false);
    raylib_public_flags(f);
    sl_pushn(f, "-std=gnu99", "-I" RL "/external/glfw/include", (char *)NULL);
    if (B.os == OS_LINUX) {
        sl_pushn(f, "-D_GLFW_X11", "-D_GNU_SOURCE", (char *)NULL);
    }
}

static bool raylib(strlist *objs)
{
    unit u = {.name = "raylib"}, glfw = {.name = "raylib-glfw"};
    raylib_flags(&u.cflags);
    raylib_flags(&glfw.cflags);
    sl_pushn(&u.srcs, RL "/rcore.c", RL "/rshapes.c", RL "/rtextures.c", RL "/rtext.c", RL "/utils.c",
             (char *)NULL);
    if (B.os == OS_MACOS) {
        sl_pushn(&glfw.cflags, "-x", "objective-c", (char *)NULL); /* GLFW's Cocoa backend */
    }
    sl_push(&glfw.srcs, RL "/rglfw.c");
    bool ok = unit_compile(&u) && unit_compile(&glfw);
    sl_extend(objs, &u.objs);
    sl_extend(objs, &glfw.objs);
    return ok;
}

bool step_third_party(strlist *objs, bool with_raylib)
{
    return monocypher(objs) && miniaudio(objs) && (!with_raylib || raylib(objs));
}

/* System libraries for raylib/GLFW on the target OS. */
void raylib_libs(strlist *f)
{
    if (B.os == OS_WINDOWS) {
        sl_pushn(f, "-lopengl32", "-lgdi32", "-lwinmm", "-lshell32", (char *)NULL);
    } else if (B.os == OS_MACOS) {
        sl_pushn(f, "-framework", "Cocoa", "-framework", "IOKit", "-framework", "CoreVideo", "-framework",
                 "OpenGL", (char *)NULL);
    } else {
        sl_pushn(f, "-lX11", "-lXrandr", "-lXi", "-lXinerama", "-lXcursor", "-lGL", "-ldl", "-lrt",
                 (char *)NULL);
    }
}
