#include <android/asset_manager.h>
#include <android/input.h>
#include <android/log.h>
#include <android/native_activity.h>
#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

#include <complex.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "polynomial.h"

#define LOG_TAG "CoefficientRootDance"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static const char *VERTEX_SHADER =
    "#version 300 es\n"
    "precision highp float;\n"
    "layout(location = 0) in vec2 a_position;\n"
    "void main() {\n"
    "    gl_Position = vec4(a_position, 0.0, 1.0);\n"
    "}\n";

enum selection_kind {
    SELECTION_NONE = 0,
    SELECTION_COEFFICIENT = 1,
    SELECTION_ROOT = 2
};

struct engine {
    struct android_app *app;

    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int32_t width;
    int32_t height;

    GLuint program;
    GLuint vao;
    GLuint vbo;
    GLint resolution_location;
    GLint half_height_location;
    GLint coefficients_location;
    GLint roots_location;
    GLint active_kind_location;
    GLint active_index_location;

    float half_height;
    float complex coefficients[QUADRATIC_COEFFICIENT_COUNT];
    float complex roots[QUADRATIC_ROOT_COUNT];

    enum selection_kind active_kind;
    int active_index;

    bool dirty;
};

static void reset_polynomial(struct engine *engine) {
    engine->half_height = 3.2f;
    engine->roots[0] = -1.0f + 0.6f * I;
    engine->roots[1] =  1.0f - 0.4f * I;
    roots_to_coefficients(engine->roots, engine->coefficients);
    engine->active_kind = SELECTION_NONE;
    engine->active_index = -1;
    engine->dirty = true;
}

static char *load_asset_text(AAssetManager *manager, const char *name) {
    AAsset *asset = AAssetManager_open(manager, name, AASSET_MODE_BUFFER);
    if (asset == NULL) {
        LOGE("could not open asset %s", name);
        return NULL;
    }

    off64_t length = AAsset_getLength64(asset);
    char *text = malloc((size_t)length + 1u);
    if (text == NULL) {
        AAsset_close(asset);
        return NULL;
    }

    off64_t offset = 0;
    while (offset < length) {
        int amount = AAsset_read(asset, text + offset, (size_t)(length - offset));
        if (amount <= 0) {
            free(text);
            AAsset_close(asset);
            LOGE("could not read asset %s", name);
            return NULL;
        }
        offset += amount;
    }

    text[length] = '\0';
    AAsset_close(asset);
    return text;
}

static GLuint compile_shader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) {
        return shader;
    }

    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    char *log = length > 0 ? malloc((size_t)length) : NULL;
    if (log != NULL) {
        glGetShaderInfoLog(shader, length, NULL, log);
        LOGE("shader compilation failed: %s", log);
        free(log);
    } else {
        LOGE("shader compilation failed");
    }

    glDeleteShader(shader);
    return 0;
}

static GLuint link_program(GLuint vertex_shader, GLuint fragment_shader) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_TRUE) {
        return program;
    }

    GLint length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    char *log = length > 0 ? malloc((size_t)length) : NULL;
    if (log != NULL) {
        glGetProgramInfoLog(program, length, NULL, log);
        LOGE("program link failed: %s", log);
        free(log);
    } else {
        LOGE("program link failed");
    }

    glDeleteProgram(program);
    return 0;
}

static bool create_renderer(struct engine *engine) {
    static const GLfloat fullscreen_triangle[] = {
        -1.0f, -1.0f,
         3.0f, -1.0f,
        -1.0f,  3.0f
    };

    char *fragment_source =
        load_asset_text(engine->app->activity->assetManager, "dance.frag");
    if (fragment_source == NULL) {
        return false;
    }

    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, VERTEX_SHADER);
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_source);
    free(fragment_source);

    if (vertex_shader == 0 || fragment_shader == 0) {
        if (vertex_shader != 0) {
            glDeleteShader(vertex_shader);
        }
        if (fragment_shader != 0) {
            glDeleteShader(fragment_shader);
        }
        return false;
    }

    engine->program = link_program(vertex_shader, fragment_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    if (engine->program == 0) {
        return false;
    }

    engine->resolution_location =
        glGetUniformLocation(engine->program, "u_resolution");
    engine->half_height_location =
        glGetUniformLocation(engine->program, "u_half_height");
    engine->coefficients_location =
        glGetUniformLocation(engine->program, "u_coefficients[0]");
    engine->roots_location =
        glGetUniformLocation(engine->program, "u_roots[0]");
    engine->active_kind_location =
        glGetUniformLocation(engine->program, "u_active_kind");
    engine->active_index_location =
        glGetUniformLocation(engine->program, "u_active_index");

    glGenVertexArrays(1, &engine->vao);
    glBindVertexArray(engine->vao);

    glGenBuffers(1, &engine->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, engine->vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(fullscreen_triangle),
        fullscreen_triangle,
        GL_STATIC_DRAW
    );
    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        2 * (GLsizei)sizeof(GLfloat),
        (const void *)0
    );
    glEnableVertexAttribArray(0);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);

    LOGI("renderer ready: %s / %s", glGetString(GL_VERSION), glGetString(GL_RENDERER));
    return true;
}

static bool initialize_display(struct engine *engine) {
    if (engine->app->window == NULL) {
        return false;
    }

    const EGLint config_attributes[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };
    const EGLint context_attributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY || !eglInitialize(display, NULL, NULL)) {
        LOGE("eglInitialize failed: 0x%x", eglGetError());
        return false;
    }

    EGLConfig config = NULL;
    EGLint config_count = 0;
    if (!eglChooseConfig(
            display,
            config_attributes,
            &config,
            1,
            &config_count
        ) || config_count != 1) {
        LOGE("could not choose GLES3 EGL config: 0x%x", eglGetError());
        eglTerminate(display);
        return false;
    }

    EGLint format = 0;
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

    EGLSurface surface =
        eglCreateWindowSurface(display, config, engine->app->window, NULL);
    EGLContext context =
        eglCreateContext(display, config, EGL_NO_CONTEXT, context_attributes);
    if (surface == EGL_NO_SURFACE || context == EGL_NO_CONTEXT) {
        LOGE("could not create EGL surface/context: 0x%x", eglGetError());
        if (surface != EGL_NO_SURFACE) {
            eglDestroySurface(display, surface);
        }
        if (context != EGL_NO_CONTEXT) {
            eglDestroyContext(display, context);
        }
        eglTerminate(display);
        return false;
    }

    if (!eglMakeCurrent(display, surface, surface, context)) {
        LOGE("eglMakeCurrent failed: 0x%x", eglGetError());
        eglDestroyContext(display, context);
        eglDestroySurface(display, surface);
        eglTerminate(display);
        return false;
    }

    engine->display = display;
    engine->surface = surface;
    engine->context = context;
    eglQuerySurface(display, surface, EGL_WIDTH, &engine->width);
    eglQuerySurface(display, surface, EGL_HEIGHT, &engine->height);

    if (!create_renderer(engine)) {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(display, context);
        eglDestroySurface(display, surface);
        eglTerminate(display);
        engine->display = EGL_NO_DISPLAY;
        engine->surface = EGL_NO_SURFACE;
        engine->context = EGL_NO_CONTEXT;
        return false;
    }

    glViewport(0, 0, engine->width, engine->height);
    engine->dirty = true;
    return true;
}

static void terminate_display(struct engine *engine) {
    if (engine->display == EGL_NO_DISPLAY) {
        return;
    }

    if (engine->vbo != 0) {
        glDeleteBuffers(1, &engine->vbo);
        engine->vbo = 0;
    }
    if (engine->vao != 0) {
        glDeleteVertexArrays(1, &engine->vao);
        engine->vao = 0;
    }
    if (engine->program != 0) {
        glDeleteProgram(engine->program);
        engine->program = 0;
    }

    eglMakeCurrent(
        engine->display,
        EGL_NO_SURFACE,
        EGL_NO_SURFACE,
        EGL_NO_CONTEXT
    );
    if (engine->context != EGL_NO_CONTEXT) {
        eglDestroyContext(engine->display, engine->context);
    }
    if (engine->surface != EGL_NO_SURFACE) {
        eglDestroySurface(engine->display, engine->surface);
    }
    eglTerminate(engine->display);

    engine->display = EGL_NO_DISPLAY;
    engine->surface = EGL_NO_SURFACE;
    engine->context = EGL_NO_CONTEXT;
}

static void update_surface_size(struct engine *engine) {
    if (engine->display == EGL_NO_DISPLAY || engine->surface == EGL_NO_SURFACE) {
        return;
    }

    eglQuerySurface(engine->display, engine->surface, EGL_WIDTH, &engine->width);
    eglQuerySurface(engine->display, engine->surface, EGL_HEIGHT, &engine->height);
    glViewport(0, 0, engine->width, engine->height);
    engine->dirty = true;
}

static void draw_frame(struct engine *engine) {
    if (engine->display == EGL_NO_DISPLAY ||
        engine->program == 0 ||
        engine->width <= 0 ||
        engine->height <= 0) {
        return;
    }

    GLfloat coefficient_pairs[QUADRATIC_COEFFICIENT_COUNT * 2];
    GLfloat root_pairs[QUADRATIC_ROOT_COUNT * 2];

    for (int index = 0; index < QUADRATIC_COEFFICIENT_COUNT; ++index) {
        coefficient_pairs[2 * index] = crealf(engine->coefficients[index]);
        coefficient_pairs[2 * index + 1] = cimagf(engine->coefficients[index]);
    }
    for (int index = 0; index < QUADRATIC_ROOT_COUNT; ++index) {
        root_pairs[2 * index] = crealf(engine->roots[index]);
        root_pairs[2 * index + 1] = cimagf(engine->roots[index]);
    }

    glUseProgram(engine->program);
    glUniform2f(
        engine->resolution_location,
        (float)engine->width,
        (float)engine->height
    );
    glUniform1f(engine->half_height_location, engine->half_height);
    glUniform2fv(
        engine->coefficients_location,
        QUADRATIC_COEFFICIENT_COUNT,
        coefficient_pairs
    );
    glUniform2fv(
        engine->roots_location,
        QUADRATIC_ROOT_COUNT,
        root_pairs
    );
    glUniform1i(engine->active_kind_location, (int)engine->active_kind);
    glUniform1i(engine->active_index_location, engine->active_index);

    glBindVertexArray(engine->vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    if (!eglSwapBuffers(engine->display, engine->surface)) {
        LOGE("eglSwapBuffers failed: 0x%x", eglGetError());
    }
    engine->dirty = false;
}

static float complex screen_to_complex(
    const struct engine *engine,
    float x,
    float y,
    bool right_side
) {
    float side_width = 0.5f * (float)engine->width;
    float side_start = right_side ? side_width : 0.0f;
    float local_x = x - side_start;
    float aspect = side_width / (float)engine->height;

    float real_part =
        (2.0f * local_x / side_width - 1.0f) *
        engine->half_height *
        aspect;
    float imaginary_part =
        (1.0f - 2.0f * y / (float)engine->height) *
        engine->half_height;

    return real_part + imaginary_part * I;
}

static void complex_to_screen(
    const struct engine *engine,
    float complex value,
    bool right_side,
    float *x,
    float *y
) {
    float side_width = 0.5f * (float)engine->width;
    float side_start = right_side ? side_width : 0.0f;
    float aspect = side_width / (float)engine->height;

    *x =
        side_start +
        0.5f *
        (crealf(value) / (engine->half_height * aspect) + 1.0f) *
        side_width;
    *y =
        0.5f *
        (1.0f - cimagf(value) / engine->half_height) *
        (float)engine->height;
}

static int nearest_handle(
    const struct engine *engine,
    float x,
    float y,
    enum selection_kind kind
) {
    const float hit_radius = 44.0f;
    float best_distance = hit_radius * hit_radius;
    int best_index = -1;
    bool right_side = kind == SELECTION_ROOT;

    int count =
        kind == SELECTION_ROOT ?
        QUADRATIC_ROOT_COUNT :
        QUADRATIC_COEFFICIENT_COUNT;

    for (int index = 0; index < count; ++index) {
        float complex value =
            kind == SELECTION_ROOT ?
            engine->roots[index] :
            engine->coefficients[index];

        float handle_x = 0.0f;
        float handle_y = 0.0f;
        complex_to_screen(
            engine,
            value,
            right_side,
            &handle_x,
            &handle_y
        );

        float delta_x = x - handle_x;
        float delta_y = y - handle_y;
        float distance = delta_x * delta_x + delta_y * delta_y;
        if (distance < best_distance) {
            best_distance = distance;
            best_index = index;
        }
    }

    return best_index;
}

static void move_active_handle(
    struct engine *engine,
    float x,
    float y
) {
    if (engine->active_kind == SELECTION_COEFFICIENT) {
        float complex value = screen_to_complex(engine, x, y, false);
        engine->coefficients[engine->active_index] = value;
        coefficients_to_roots(engine->coefficients, engine->roots);
        engine->dirty = true;
    } else if (engine->active_kind == SELECTION_ROOT) {
        float complex value = screen_to_complex(engine, x, y, true);
        engine->roots[engine->active_index] = value;
        roots_to_coefficients(engine->roots, engine->coefficients);
        engine->dirty = true;
    }
}

static int32_t handle_input(struct android_app *app, AInputEvent *event) {
    struct engine *engine = app->userData;
    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) {
        return 0;
    }

    int32_t masked_action =
        AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
    float x = AMotionEvent_getX(event, 0);
    float y = AMotionEvent_getY(event, 0);

    switch (masked_action) {
        case AMOTION_EVENT_ACTION_DOWN: {
            enum selection_kind kind =
                x < 0.5f * (float)engine->width ?
                SELECTION_COEFFICIENT :
                SELECTION_ROOT;
            int index = nearest_handle(engine, x, y, kind);

            if (index < 0) {
                engine->active_kind = SELECTION_NONE;
                engine->active_index = -1;
                return 1;
            }

            engine->active_kind = kind;
            engine->active_index = index;
            engine->dirty = true;
            LOGI("drag start kind=%d index=%d", (int)kind, index);
            return 1;
        }

        case AMOTION_EVENT_ACTION_MOVE:
            if (engine->active_kind != SELECTION_NONE) {
                move_active_handle(engine, x, y);
            }
            return 1;

        case AMOTION_EVENT_ACTION_UP:
            if (engine->active_kind != SELECTION_NONE) {
                move_active_handle(engine, x, y);
                LOGI(
                    "drag end kind=%d index=%d",
                    (int)engine->active_kind,
                    engine->active_index
                );
            }
            engine->active_kind = SELECTION_NONE;
            engine->active_index = -1;
            engine->dirty = true;
            return 1;

        case AMOTION_EVENT_ACTION_CANCEL:
            engine->active_kind = SELECTION_NONE;
            engine->active_index = -1;
            engine->dirty = true;
            return 1;

        default:
            return 1;
    }
}

static void handle_command(struct android_app *app, int32_t command) {
    struct engine *engine = app->userData;

    switch (command) {
        case APP_CMD_INIT_WINDOW:
            if (app->window != NULL && engine->display == EGL_NO_DISPLAY) {
                initialize_display(engine);
            }
            break;

        case APP_CMD_TERM_WINDOW:
            terminate_display(engine);
            break;

        case APP_CMD_WINDOW_RESIZED:
        case APP_CMD_CONTENT_RECT_CHANGED:
        case APP_CMD_CONFIG_CHANGED:
            update_surface_size(engine);
            break;

        case APP_CMD_GAINED_FOCUS:
            engine->dirty = true;
            break;

        default:
            break;
    }
}

void android_main(struct android_app *app) {
    struct engine engine = {
        .app = app,
        .display = EGL_NO_DISPLAY,
        .surface = EGL_NO_SURFACE,
        .context = EGL_NO_CONTEXT,
        .active_kind = SELECTION_NONE,
        .active_index = -1,
        .dirty = true
    };

    reset_polynomial(&engine);

    app->userData = &engine;
    app->onAppCmd = handle_command;
    app->onInputEvent = handle_input;

    while (true) {
        int events = 0;
        struct android_poll_source *source = NULL;
        int timeout =
            engine.dirty && engine.display != EGL_NO_DISPLAY ?
            0 :
            -1;

        int ident =
            ALooper_pollOnce(timeout, NULL, &events, (void **)&source);

        if (ident >= 0 && source != NULL) {
            source->process(app, source);
        }

        if (app->destroyRequested != 0) {
            terminate_display(&engine);
            return;
        }

        if (engine.dirty && engine.display != EGL_NO_DISPLAY) {
            draw_frame(&engine);
        }
    }
}
