#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "imgui/imgui_demo.cpp"

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
#include "camera.cpp"

struct RenderContext {
    GLFWwindow* window;
    uint width;
    uint height;
};

static RenderContext g_renderContext;

static inline void
resizeView(RenderContext* renderContext, uint width, uint height) {
    renderContext->width = width;
    renderContext->height = height;
    glViewport(0, 0, width, height);
}

static void
windowSizeCallback(GLFWwindow* window, int width, int height) {
    resizeView(&g_renderContext, (uint)width, (uint)height);
}

static float lastX;
static float lastY;
static bool firstMouse = true;

static Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

static bool cameraMousePressed = false;

static void
mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if(!cameraMousePressed) return;
    camera.processMouseMovement(xoffset, yoffset);
}

static void
mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
        cameraMousePressed = true;
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
        cameraMousePressed = false;
}

static void
scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.processMouseScroll(yoffset);
}

struct Entity {
    glm::vec3 position;
    glm::vec3 rotation;
    float scale;

    Model* model;
    Shader shader;
};

int main() {
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: could not start GLFW3\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    g_renderContext.width = 1280;
    g_renderContext.height = 720;

    GLFWwindow* window = glfwCreateWindow(g_renderContext.width, g_renderContext.height, "opengl_foobar", NULL, NULL);
    if (!window) {
        fprintf(stderr, "ERROR: could not open window with GLFW3\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    g_renderContext.window = window;

    glfwSetWindowSizeCallback(window, windowSizeCallback);

    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);

    glewExperimental = GL_TRUE;
    glewInit();

    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported %s\n", version);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    Shader basicShader = compileShader("basic.vs", "basic.fs");
    Model nanosuitModel = loadModel("data/nanosuit/nanosuit.obj");

    std::vector<Entity> entities;
    {
        Entity entity = {};
        entity.scale = 0.3f;
        entity.model = &nanosuitModel;
        entity.shader = basicShader;
        entities.push_back(entity);
    }

    glm::vec3 clearColor = glm::vec3(0.2f, 0.3f, 0.3f);

    float deltaTime = 0.f;
    float lastFrame = glfwGetTime();

    bool running = true;
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.processKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.processKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.processKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.processKeyboard(RIGHT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            camera.processKeyboard(UP, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            camera.processKeyboard(DOWN, deltaTime);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Entities");
        for(int i = 0; i < entities.size(); i++) {
            std::string id = "Entity_" + i;
            ImGui::PushID(id.c_str());

            auto* entity = &entities[i];
            ImGui::Text("Entity: %i", i);

            ImGui::DragFloat3("position", (float*)&entity->position, 0.1f);
            ImGui::DragFloat3("rotation", (float*)&entity->rotation, 0.1f);
            ImGui::DragFloat("scale", &entity->scale, 0.01f);

            ImGui::Spacing();

            ImGui::PopID();
        }
        ImGui::Separator();
        if(ImGui::Button("Add entity")) {
            Entity entity = {};
            entity.scale = 0.3f;
            entity.model = &nanosuitModel;
            entity.shader = basicShader;
            entities.push_back(entity);
        }
        ImGui::End();

        glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // also clear the depth buffer now!

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)g_renderContext.width / (float)g_renderContext.height, 0.1f, 100.0f);
        glm::mat4 view = camera.getViewMatrix();

        for(int i = 0; i < entities.size(); i++) {
            auto* entity = &entities[i];
            use(entity->shader);
            setMat4(entity->shader, "projection", projection);
            setMat4(entity->shader, "view", view);

            glm::mat4 modelMat = glm::mat4(1.0f);
            modelMat = glm::translate(modelMat, entity->position);
            modelMat = glm::scale(modelMat, glm::vec3(entity->scale, entity->scale, entity->scale));
            modelMat = modelMat * glm::yawPitchRoll(entity->rotation.x, entity->rotation.y, entity->rotation.z);

            setMat4(entity->shader, "model", modelMat);

            drawModel(entity->model, entity->shader);
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
