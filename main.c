#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#ifndef __WIN32
#include <unistd.h>
#endif


#define WIDTH 500
#define HEIGHT 500
#define WINDOW_SCALE 4
#define SLEEP_TIME 0

void sleep_us(unsigned long long us) {
#ifndef __WIN32
    usleep(us);
#endif
}

unsigned int ceil_div(unsigned int a, unsigned int d) {
    return (a + d - 1) / d;
}

void GLAPIENTRY
MessageCallback( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam )
{
    (void) source;
    (void) id;
    (void) length;
    (void) userParam;
    fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
        (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
        type, severity, message);
}



int rect_prog() {
    static int prg = -1;
    if(prg == -1) {
        const char* v_source = "#version 430 core\n"
            "layout(location = 0) in vec2 position;\n"
            "layout(location = 1) in vec2 uv;\n"
            "out vec2 f_uv;\n"
            "void main() {\n"
            "   gl_Position = vec4(position, 0.0f, 1.0f);\n"
            "   f_uv = uv;\n"
            "}";
        const char* f_source = "#version 430 core\n"
            "in vec2 f_uv;\n"
            "out vec4 color;\n"
            "uniform isampler2D sampler;\n"
            "void main() {\n"
            "   int v = texture(sampler, f_uv).x;\n"
            //"   color = vec4(float(v), 0.0f, 0.0f, 1.0f);\n"
            "   color = vec4(1.0f, 0.0f, 1.0f, 1.0f);\n"
            "   if(v == 1) color = vec4(1.0, 0.0f, 0.0f, 1.0f);\n"
            "   if(v == 0) color = vec4(0.2f, 0.2f, 0.1f, 1.0f);\n"
            "}";

        const GLint v_len = strlen(v_source);
        const GLint f_len = strlen(f_source);
        prg = glCreateProgram();
        unsigned int v_shader = glCreateShader(GL_VERTEX_SHADER);
        unsigned int f_shader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(v_shader, 1, (const GLchar**) &v_source, &v_len);
        glShaderSource(f_shader, 1, (const GLchar**) &f_source, &f_len);

        glCompileShader(v_shader);
        glCompileShader(f_shader);

        GLint status;
        glGetShaderiv(v_shader, GL_COMPILE_STATUS, &status);
        if(status != GL_TRUE) {
            char log[1024];
            GLsizei length;
            glGetShaderInfoLog(v_shader, sizeof(log) - 1, &length, log);
            fprintf(stderr, "vertex shader: %.*s\n", (int) length, log);
        }

        glGetShaderiv(f_shader, GL_COMPILE_STATUS, &status);
        if(status != GL_TRUE) {
            char log[1024];
            GLsizei length;
            glGetShaderInfoLog(f_shader, sizeof(log) - 1, &length, log);
            fprintf(stderr, "fragment shader: %.*s\n", (int) length, log);
        }



        glAttachShader(prg, v_shader);
        glAttachShader(prg, f_shader);
        glLinkProgram(prg);
        glDeleteShader(v_shader);
        glDeleteShader(f_shader);

        glGetProgramiv(prg, GL_LINK_STATUS, &status);
        if(status != GL_TRUE) {
            char log[1024];
            GLsizei length;
            glGetProgramInfoLog(prg, sizeof(log) - 1, &length, log);
            fprintf(stderr, "program: %.*s\n", (int) length, log);
        }

    }
    return prg;
}



char* slurp_file(const char* filepath) {
    FILE* f = fopen(filepath, "r");
    if(f == NULL) {
        perror("fopen");
        return NULL;
    }
    fseek(f, 0L, SEEK_END);
    unsigned long size = ftell(f);
    rewind(f);
    char* data = malloc(size + 1);
    if(data == NULL) {
        fclose(f);
        return NULL;
    }
    size_t read = fread(data, 1, size, f);
    if(read != size) {
        fprintf(stderr, "fread: %d\n", ferror(f));
        return NULL;
    }
    data[size] = '\0';
    fclose(f);
    return data;
}

int main() {
    glfwInit();

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* wnd = glfwCreateWindow(WIDTH * WINDOW_SCALE, HEIGHT * WINDOW_SCALE, "Compute Test", NULL, NULL);
    glfwMakeContextCurrent(wnd);
    glfwSwapInterval(0);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // During init, enable debug output
    // glEnable(GL_DEBUG_OUTPUT);
    // glDebugMessageCallback(MessageCallback, 0);

    unsigned int prog = glCreateProgram();
    unsigned int shader = glCreateShader(GL_COMPUTE_SHADER);

    char* source = slurp_file("./shader.glsl");
    if(source == NULL) {
        exit(1);
    }
    GLint len = strlen(source);
    glShaderSource(shader, 1, (const GLchar**) &source, &len);
    free(source);
    int status;
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(status != GL_TRUE) {
        char log[1024];
        GLsizei length;
        glGetShaderInfoLog(shader, sizeof(log) - 1, &length, log);
        fprintf(stderr, "compute shader: %.*s", (int) length, log);
    }
    glAttachShader(prog, shader);
    glLinkProgram(prog);
    glDeleteShader(shader);

    
    unsigned int vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    float vdata[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,

        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 0.0f, 1.0f,
    };
    unsigned int vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vdata), vdata, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (const void*) (0));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (const void*) (sizeof(float) * 2));

#define SPAWN_GLIDERS 0
    int DATA[WIDTH * HEIGHT];
    for(int i = 0; i < WIDTH; ++i) {
        for(int j = 0; j < HEIGHT; ++j) {
#if SPAWN_GLIDERS
            if(i < 450 && j > HEIGHT - 450)
                continue;
#endif
            int idx = i + WIDTH * j;
            DATA[idx] = (rand() % 100) < 20;
        }
    }

#if SPAWN_GLIDERS
    for(int i = 0; i < 10; i++) {
        DATA[16 + (HEIGHT - i * 30 - 1 - 18) * WIDTH] = 1;
        DATA[16 + (HEIGHT - i * 30 - 1 - 19) * WIDTH] = 1;
        DATA[17 + (HEIGHT - i * 30 - 1 - 18) * WIDTH] = 1;
        DATA[17 + (HEIGHT - i * 30 - 1 - 19) * WIDTH] = 1;
        DATA[26 + (HEIGHT - i * 30 - 1 - 18) * WIDTH] = 1;
        DATA[26 + (HEIGHT - i * 30 - 1 - 19) * WIDTH] = 1;
        DATA[26 + (HEIGHT - i * 30 - 1 - 20) * WIDTH] = 1;
        DATA[27 + (HEIGHT - i * 30 - 1 - 17) * WIDTH] = 1;
        DATA[28 + (HEIGHT - i * 30 - 1 - 16) * WIDTH] = 1;
        DATA[29 + (HEIGHT - i * 30 - 1 - 16) * WIDTH] = 1;
        DATA[27 + (HEIGHT - i * 30 - 1 - 21) * WIDTH] = 1;
        DATA[28 + (HEIGHT - i * 30 - 1 - 22) * WIDTH] = 1;
        DATA[29 + (HEIGHT - i * 30 - 1 - 22) * WIDTH] = 1;
        DATA[30 + (HEIGHT - i * 30 - 1 - 19) * WIDTH] = 1;
        DATA[32 + (HEIGHT - i * 30 - 1 - 19) * WIDTH] = 1;
        DATA[32 + (HEIGHT - i * 30 - 1 - 18) * WIDTH] = 1;
        DATA[32 + (HEIGHT - i * 30 - 1 - 20) * WIDTH] = 1;
        DATA[33 + (HEIGHT - i * 30 - 1 - 19) * WIDTH] = 1;
        DATA[31 + (HEIGHT - i * 30 - 1 - 21) * WIDTH] = 1;
        DATA[31 + (HEIGHT - i * 30 - 1 - 17) * WIDTH] = 1;
        DATA[36 + (HEIGHT - i * 30 - 1 - 18) * WIDTH] = 1;
        DATA[37 + (HEIGHT - i * 30 - 1 - 18) * WIDTH] = 1;
        DATA[36 + (HEIGHT - i * 30 - 1 - 17) * WIDTH] = 1;
        DATA[37 + (HEIGHT - i * 30 - 1 - 17) * WIDTH] = 1;
        DATA[36 + (HEIGHT - i * 30 - 1 - 16) * WIDTH] = 1;
        DATA[37 + (HEIGHT - i * 30 - 1 - 16) * WIDTH] = 1;
        DATA[38 + (HEIGHT - i * 30 - 1 - 19) * WIDTH] = 1;
        DATA[38 + (HEIGHT - i * 30 - 1 - 15) * WIDTH] = 1;
        DATA[40 + (HEIGHT - i * 30 - 1 - 15) * WIDTH] = 1;
        DATA[40 + (HEIGHT - i * 30 - 1 - 14) * WIDTH] = 1;
        DATA[40 + (HEIGHT - i * 30 - 1 - 19) * WIDTH] = 1;
        DATA[40 + (HEIGHT - i * 30 - 1 - 20) * WIDTH] = 1;
        DATA[50 + (HEIGHT - i * 30 - 1 - 16) * WIDTH] = 1;
        DATA[51 + (HEIGHT - i * 30 - 1 - 16) * WIDTH] = 1;
        DATA[51 + (HEIGHT - i * 30 - 1 - 17) * WIDTH] = 1;
        DATA[50 + (HEIGHT - i * 30 - 1 - 17) * WIDTH] = 1;
    }
#endif


    unsigned int tex[2];
    glGenTextures(2, tex);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, WIDTH, HEIGHT, 0, GL_RED_INTEGER, 
             GL_INT, DATA);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, WIDTH, HEIGHT, 0, GL_RED_INTEGER, 
             GL_INT, DATA);




    int input = 0;
    while(!glfwWindowShouldClose(wnd)) {
        if(glfwGetKey(wnd, GLFW_KEY_SPACE) == GLFW_PRESS) {
            printf("start simulating\n");
            break;
        }
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex[input]);

        if(glfwGetMouseButton(wnd, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            // place pixel
            

            double x, y;
            glfwGetCursorPos(wnd, &x, &y);
            const int px = (int) x / WINDOW_SCALE;
            const int py = (int) y / WINDOW_SCALE;

            if(px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT) {
                if(DATA[px + (HEIGHT - 1 - py) * WIDTH] != 1)
                    printf("\tDATA[%d + (HEIGHT - 1 - %d) * WIDTH] = 1;\n", px, py);
                DATA[px + (HEIGHT - 1 - py) * WIDTH] = 1;
                glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, WIDTH, HEIGHT, 0, GL_RED_INTEGER, 
                     GL_INT, DATA);
            }
        }

        glClear(GL_COLOR_BUFFER_BIT);
        const int p = rect_prog();
        glUseProgram(p);
        int loc = glGetUniformLocation(p, "sampler");
        glUniform1i(loc, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(wnd);
        glfwPollEvents();
    }

    while(!glfwWindowShouldClose(wnd)) {
        sleep_us(SLEEP_TIME);
        const int output = 1 - input;

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex[output]);
        glBindImageTexture(0, tex[output], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32I);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tex[input]);
        glBindImageTexture(1, tex[input], 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32I);

        const int TILE_SIZE = 30;
        glUseProgram(prog);
        glDispatchCompute(ceil_div(WIDTH, TILE_SIZE), ceil_div(HEIGHT, TILE_SIZE), 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        glClear(GL_COLOR_BUFFER_BIT);
        const int p = rect_prog();
        glUseProgram(p);
        int loc = glGetUniformLocation(p, "sampler");
        glUniform1i(loc, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex[output]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glfwSwapBuffers(wnd);
        glfwPollEvents();

        input = output;
    }

    glDeleteTextures(2, tex);

    glDeleteProgram(rect_prog());
    glDeleteProgram(prog);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    glfwDestroyWindow(wnd);
    glfwTerminate();
}
