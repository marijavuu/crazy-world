#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>1

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

void renderQuad();//hdr bloom

//skajboks
unsigned int loadTexture(char const * path);
unsigned int loadCubemap(vector<std::string> faces);

// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 800;

bool hdr = true;
bool hdrKeyPressed = false;
bool bloom = false;
bool bloomKeyPressed = false;
int increaseSpeed = 1.0f;
float exposure = 1.0f;
glm::vec3 spiderPosition = glm::vec3(-15.0f,2.0f,50.0f);

// camera
Camera camera(glm::vec3(-10.0f, 5.0f, 20.0f));
bool CameraMouseMovementUpdateEnabled = true;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 backpackPosition = glm::vec3(0.0f);
    float backpackScale = 1.0f;
    // PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}
    void SaveToFile(std::string filename);
    void LoadFromFile(std::string filename);
};
void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}
void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}
ProgramState *programState;
void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);//p

    // build and compile shaders
    // -------------------------
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader tanjirShader("resources/shaders/tanjir.vs","resources/shaders/tanjir.fs");
    Shader hdrShader("resources/shaders/hdr.vs","resources/shaders/hdr.fs");
    Shader bloomShader("resources/shaders/bloom.vs","resources/shaders/bloom.fs");

    // tanjir
    float vertices[] = {          //naopacke !!!!!!
            // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  0.0f
    };

    //vao i vbo za tanjir
    unsigned int tanjirVAO, tanjirVBO;
    glGenVertexArrays(1, &tanjirVAO);
    glGenBuffers(1, &tanjirVBO);
    glBindVertexArray(tanjirVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tanjirVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);



    glm::vec3 tanjiric2 = glm::vec3(-26.0f,-5.0f,-3.0f);

    // load models
    // -----------
    Model hamburgeri("resources/objects/model16/hamburgeres.obj");
    hamburgeri.SetShaderTextureNamePrefix("material.");

    Model keksici("resources/objects/model22/Biscuit.obj");
    keksici.SetShaderTextureNamePrefix("material.");

    Model cheezespider("resources/objects/model24/cheezespider.obj");
    cheezespider.SetShaderTextureNamePrefix("material.");

    Model ananas("resources/objects/model25/10200_Pineapple_v1-L2.obj");
    ananas.SetShaderTextureNamePrefix("material.");

    //nesto me ova koca zeza ,nema ispunjenu casu ,nzm sto,a volim ovaj model
    Model cocacola1("resources/objects/model27/cup OBJ.obj");
    cocacola1.SetShaderTextureNamePrefix("material.");

    // everything for bloom and hdr
    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // color attachments we'll use for rendering
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ping-pong-framebuffer for blurring
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }


    //***********************************************************************************


    // load textures
    unsigned int  tanjiric2tex= loadTexture(FileSystem::getPath("resources/textures/tanjir5.png").c_str());


    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };


    // skybox VAO, VBO, and loading textures
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);


    vector<std::string> faces {



            FileSystem::getPath("resources/textures/skybox/2.jpg"),
            FileSystem::getPath("resources/textures/skybox/5.jpg"),
            FileSystem::getPath("resources/textures/skybox/12.jpg"),
            FileSystem::getPath("resources/textures/skybox/12.jpg"),
            FileSystem::getPath("resources/textures/skybox/12.jpg"),
            FileSystem::getPath("resources/textures/skybox/12.jpg")



    };

    //stbi_set_flip_vertically_on_load(true);

//***************************************************************

    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


    //Shader activation
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    bloomShader.use();
    bloomShader.setInt("image", 0);

    hdrShader.use();
    hdrShader.setInt("hdrBuffer", 0);
    hdrShader.setInt("bloomBlur", 1);



    unsigned int cubemapTexture = loadCubemap(faces);

    float lin = 0.14f;
    float kvad = 0.02f;

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // render
        // ------
        //glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);//izmeni
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);


        // don't forget to enable shader before setting uniforms
        ourShader.use();


        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        //Directional Lignt
        ourShader.setVec3("dirLight.direction", 20.0f, 20.0f, 0.0f);
        ourShader.setVec3("dirLight.ambient", 0.3, 0.3, 0.3);
        ourShader.setVec3("dirLight.diffuse",  0.6f,0.2f,0.2);
        ourShader.setVec3("dirLight.specular", 0.1, 0.1, 0.1);

        // Pointlight's
        //1
        ourShader.setVec3("pointLight[0].position", glm::vec3(1.05f,5.4f,8.7f));
        ourShader.setVec3("pointLight[0].ambient", glm::vec3(0.15, 0.15, 0.15));
        ourShader.setVec3("pointLight[0].diffuse", glm::vec3(1.5f,1.5f,1.1f));
        ourShader.setVec3("pointLight[0].specular", glm::vec3(0.15, 0.15, 0.15));
        ourShader.setFloat("pointLight[0].constant", 1.0f);
        ourShader.setFloat("pointLight[0].linear", lin);
        ourShader.setFloat("pointLight[0].quadratic", kvad);
        //2
        ourShader.setVec3("pointLight[1].position", glm::vec3(-1.70f,(2.4f + sin(glfwGetTime())/6),11.1f));
        ourShader.setVec3("pointLight[1].ambient", glm::vec3(0.15, 0.15, 0.15));
        ourShader.setVec3("pointLight[1].diffuse", glm::vec3(1.5f,1.5f,1.1f));
        ourShader.setVec3("pointLight[1].specular", glm::vec3(0.15, 0.15, 0.15));
        ourShader.setFloat("pointLight[1].constant", 1.0f);
        ourShader.setFloat("pointLight[1].linear", lin);
        ourShader.setFloat("pointLight[1].quadratic", kvad);
        //3
        ourShader.setVec3("pointLight[2].position", glm::vec3(-5.75f,(-4.85f + sin(glfwGetTime())/6),1.95f));
        ourShader.setVec3("pointLight[2].ambient", glm::vec3(0.15, 0.15, 0.15));
        ourShader.setVec3("pointLight[2].diffuse", glm::vec3(1.5f,1.5f,1.1f));
        ourShader.setVec3("pointLight[2].specular", glm::vec3(0.15, 0.15, 0.15));
        ourShader.setFloat("pointLight[2].constant", 1.0f);
        ourShader.setFloat("pointLight[2].linear", lin);
        ourShader.setFloat("pointLight[2].quadratic", kvad);
        //4
        ourShader.setVec3("pointLight[3].position", glm::vec3(7.7f,(0.4f + sin(glfwGetTime())/6),8.75f));
        ourShader.setVec3("pointLight[3].ambient", glm::vec3(0.15, 0.15, 0.15));
        ourShader.setVec3("pointLight[3].diffuse", glm::vec3(1.5f,1.5f,1.1f));
        ourShader.setVec3("pointLight[3].specular", glm::vec3(0.15, 0.15, 0.15));
        ourShader.setFloat("pointLight[3].constant", 1.0f);
        ourShader.setFloat("pointLight[3].linear", lin);
        ourShader.setFloat("pointLight[3].quadratic", kvad);
        //5
        ourShader.setVec3("pointLight[3].position", glm::vec3(-18.7f,(1.4f + sin(glfwGetTime())/6),45.75f));
        ourShader.setVec3("pointLight[3].ambient", glm::vec3(0.15, 0.15, 0.15));
        ourShader.setVec3("pointLight[3].diffuse", glm::vec3(1.5f,1.5f,1.1f));
        ourShader.setVec3("pointLight[3].specular", glm::vec3(0.15, 0.15, 0.15));
        ourShader.setFloat("pointLight[3].constant", 1.0f);
        ourShader.setFloat("pointLight[3].linear", lin);
        ourShader.setFloat("pointLight[3].quadratic", kvad);


        ourShader.setVec3("viewPosition", camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        //Enabling back face culling
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        // render the loaded model

        //hamburgeri

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3(-1.0f,(-3.0f+ sin(glfwGetTime())/6),0.0f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(0.7f,0.7f,0.7f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        hamburgeri.Draw(ourShader);

        //keksici
        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(-16.0f,(-3.0f+ sin(glfwGetTime())/6),0.0f)
        ); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(1.2f,1.5f,1.2f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        keksici.Draw(ourShader);


        //cheezespider
        model = glm::mat4(1.0f);
        model = glm::translate(model,spiderPosition); // translate it down so it's at the center of the scene
        model = glm::rotate(model, (float)-90, glm::vec3(0.0, 1.0, 0.0));
        model = glm::rotate(model, (float) sin(glfwGetTime()), glm::vec3(0.0, 1.0, 0.0));
        model = glm::scale(model, glm::vec3(0.1f,0.1f,0.1f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        cheezespider.Draw(ourShader);


        //ananas
        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(-9.0f,(-3.0f+ sin(glfwGetTime())/6),0.0f)
        ); // translate it down so it's at the center of the scene
        model = glm::rotate(model, (float)-90, glm::vec3(0.0, 1.0, 0.0));
        model = glm::scale(model, glm::vec3(0.2f,0.2f,0.2f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        ananas.Draw(ourShader);


        //cocacola1
        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3(-6.0f,(-3.0f+ sin(glfwGetTime())/6),0.0f) ); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(0.2f,0.2f,0.2f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        cocacola1.Draw(ourShader);



        glDisable(GL_CULL_FACE);

        //tanjir2
        tanjirShader.use();
        tanjirShader.setMat4("projection", projection);
        tanjirShader.setMat4("view", view);

        glBindVertexArray(tanjirVAO);
        glBindTexture(GL_TEXTURE_2D, tanjiric2tex);
        model = glm::mat4(1.0f);
        model = glm::translate(model,tanjiric2);
        model = glm::scale(model, glm::vec3(35.0f,25.0f,35.0f));
        tanjirShader.setMat4("model",model);
        glDrawArrays(GL_TRIANGLES,0,6);


        //*************************************************************************
        // draw skybox as last
        // change depth function so depth test passes when values are equal to depth buffer's content
        //glDepthMask(GL_FALSE);//!!!!!!

        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        //projection=glm::perspective(glm::radians(programState->camera.Zoom),(float )SCR_WIDTH/(float )SCR_HEIGHT , 0.1f ,100.0f);
        view = glm::mat4(glm::mat3(camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", glm::mat4(glm::mat3 (view)));
        skyboxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //*********************************************
        //load pingpong
        bool horizontal = true, first_iteration = true;
        unsigned int amount = 10;
        bloomShader.use();
        for (unsigned int i = 0; i < amount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            bloomShader.setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);

            renderQuad();

            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // **********************************************
        // load hdr and bloom
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        hdrShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
        hdrShader.setBool("hdr", hdr);
        hdrShader.setBool("bloom", bloom);
        hdrShader.setFloat("exposure", exposure);
        renderQuad();
        // view = glm::mat4(glm::mat3(camera.GetViewMatrix()));

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    //deleting arrays and buffers
    glDeleteVertexArrays(1, &skyboxVAO);//~~~~~~~~~~~~~~~~~~
    glDeleteBuffers(1, &skyboxVAO);//~~~~~~~~~~~~~~~~~~~~~~~~
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// Spider movement
void moveSpider(Camera_Movement direction,int increaseSpeed)
{
    float velocity = 2.5f * deltaTime * increaseSpeed;
    glm::vec3 yLock(1.0f, 0.0f, 1.0f);
    glm::vec3 yMove(0.0f, 5.0f, 0.0f);

    if (direction == FORWARD)
        spiderPosition += camera.Front * velocity * yLock;
    if (direction == BACKWARD)
        spiderPosition -= camera.Front * velocity * yLock;
    if (direction == LEFT)
        spiderPosition -= camera.Right * velocity * yLock;
    if (direction == RIGHT)
        spiderPosition += camera.Right * velocity * yLock;

    if (direction == UP)
        spiderPosition += velocity * yMove;
    if (direction == DOWN)
        spiderPosition -= velocity * yMove;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if(glfwGetKey(window,GLFW_KEY_O) == GLFW_PRESS){
        increaseSpeed = 3.0f;
    }else
        increaseSpeed = 1.0f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime * increaseSpeed);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime * increaseSpeed);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime * increaseSpeed);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime* increaseSpeed);

    if(glfwGetKey(window,GLFW_KEY_I) == GLFW_PRESS){
        increaseSpeed = 5.0f;
    }else
        increaseSpeed = 1.0f;


    // spider movement
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        moveSpider(FORWARD,increaseSpeed);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        moveSpider(BACKWARD,increaseSpeed);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        moveSpider(LEFT,increaseSpeed);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        moveSpider(RIGHT,increaseSpeed);

    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
        moveSpider(UP,increaseSpeed);
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
        moveSpider(DOWN,increaseSpeed);


    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS && !hdrKeyPressed)
    {
        hdr = !hdr;
        hdrKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_RELEASE)
    {
        hdrKeyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !bloomKeyPressed)
    {
        bloom = !bloom;
        bloomKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE)
    {
        bloomKeyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        if (exposure > 0.0f)
            exposure -= 0.005f;
        else
            exposure = 0.0f;
    }
    else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        exposure += 0.005f;
    }


}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (CameraMouseMovementUpdateEnabled)//bolje bez ovoga
        camera.ProcessMouseMovement(xoffset, yoffset);//izmeni

}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}

//Cubemap loading function
//------------------------------------------------------------------------
unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}
//Texture loading function
//-----------------------------------------------------------

unsigned int loadTexture(const char *path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}



void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}