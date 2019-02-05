#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//TODO: Maybe don't use stl as much. Just faster to setup for now.
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>

#define arrayCount(x) ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

typedef unsigned int uint;

#include "shader.cpp"
#include "model_loading.cpp"

static const uint SCREEN_WIDTH = 640;
static const uint SCREEN_HEIGHT = 480;

int main() {
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: could not start GLFW3\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "opengl_foobar", NULL, NULL);
    if (!window) {
        fprintf(stderr, "ERROR: could not open window with GLFW3\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    glewInit();

    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported %s\n", version);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    float points[] = {
        0.0f,  0.5f,  0.0f,
        0.5f, -0.5f,  0.0f,
        -0.5f, -0.5f,  0.0f
    };

    Shader shader = compileShader("basic.vs", "basic.fs");

    printf("START model loading\n");
    Model model = loadModel("data/LowpolyForestPack/Trees/OakTree1.fbx");
    printf("END model loading\n");

    bool running = true;
    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // also clear the depth buffer now!

        use(shader);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view       = glm::lookAt(glm::vec3(0,0,-10), glm::vec3(0), glm::vec3(0,1,0));
        setMat4(shader, "projection", projection);
        setMat4(shader, "view", view);

        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, glm::vec3(0.0f, -2.f, 0.0f));
        modelMat = glm::scale(modelMat, glm::vec3(0.3f, 0.3f, 0.3f));
        modelMat = glm::rotate(modelMat, -90.0f, glm::vec3(1,0,0));
        setMat4(shader, "model", modelMat);
        drawModel(&model, shader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
