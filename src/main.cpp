#define GLM_ENABLE_EXPERIMENTAL
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shader.h"
#include "opengl_utils.h"
#include <iostream>
#include <vector>
#include "camera.h"
#include "texture.h"
#include "texture_cube.h"
#include "model.h"
#include "mesh.h"
#include "FreeImage.h"
#include <time.h>
#include "glm/gtx/string_cast.hpp"
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <string>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

bool isWindowed = true;
bool isKeyboardDone[1024] = { 0 };

// setting
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT =800;

const int GUIDE_WIDTH = 10;
const int GUIDE_HEIGHT = 10;

// camera
Camera camera(glm::vec3(0, 18, 0), glm::vec3(0,-1,0), glm::vec3(0.0f, 1.0f, 0.0f), 90.0f);
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

bool mask_mode = false;

bool changeFlagSPACE = false;
bool start = true;


// Save Image to png file. press V key.
// file name : date.png (created in bin folder)
void saveImage(const char* filename) {
    // Make the BYTE array, factor of 3 because it's RBG.
    BYTE* pixels = new BYTE[3 * SCR_WIDTH * SCR_HEIGHT];
    glReadPixels(0, 0, SCR_WIDTH, SCR_HEIGHT, GL_BGR, GL_UNSIGNED_BYTE, pixels);
    ofstream myfile(filename,ios::app);
    if(myfile.is_open()){
        for (int i = 0 ; i< 3*SCR_WIDTH * SCR_HEIGHT-1; i++){
            myfile << int(pixels[i]) << ",";
        }
        myfile << int(pixels[3*SCR_WIDTH*SCR_HEIGHT-1]) << endl;
    }
    myfile.close();

//    // Convert to FreeImage format & save to file
//    FIBITMAP* image = FreeImage_ConvertFromRawBits(pixels, SCR_WIDTH, SCR_HEIGHT, 3 * SCR_WIDTH, 24, 0xFF0000, 0x00FF00, 0x0000FF, false);
//    FreeImage_Save(FIF_PNG, image, filename, 0);
//
//    // Free resources
//    FreeImage_Unload(image);
    delete[] pixels;
}


float random_value(float range_min, float range_max){
    return (float) rand() / ((float)RAND_MAX + 1)*(range_max - range_min)   + range_min;
}

glm::vec3 current_pos;
glm::vec3 ball_speed;
glm::vec3 damp_speed;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    srand((unsigned int)time(NULL));
    ball_speed = glm::vec3(rand()%20-10,0,rand()%20-10);
    current_pos = glm::vec3(0,0.5,0);
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "2020-26234 Bae Jin Seok", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    // glEnable(GL_DEPTH_TEST);

    // build and compile our shader program
    // ------------------------------------
    Shader rayTracingShader("../shaders/shader_ray_tracing.vs", "../shaders/shader_ray_tracing.fs");

    std::vector<float> quad_data({
                                         // positions         // uvs
                                         1.0f, 1.0f, 0.0f,  1.0f, 1.0f,  // top right
                                         1.0f, -1.0f, 0.0f,  1.0f, 0.0f,  // top left
                                         -1.0f,  -1.0f, 0.0f, 0.0f, 0.0f,   // bottom left
                                         -1.0f,  1.0f, 0.0f,  0.0f, 1.0f  // bottom right
                                 });
    std::vector<unsigned int> quad_indices_vec({ 0,1,3,1,2,3 });
    std::vector<unsigned int> attrib_sizes({ 3, 2 });

    VAO* quad = getVAOFromAttribData(quad_data, attrib_sizes, quad_indices_vec);

    std::vector<std::string> faces
            {
//                    "../resources/skybox/right.jpg",
//                    "../resources/skybox/left.jpg",
//                    "../resources/skybox/top.jpg",
//                    "../resources/skybox/bottom.jpg",
//                    "../resources/skybox/front.jpg",
//                    "../resources/skybox/back.jpg"
                    "../resources/skybox/default_skybox.jpg",
                    "../resources/skybox/default_skybox.jpg",
                    "../resources/skybox/default_skybox.jpg",
                    "../resources/skybox/default_skybox.jpg",
                    "../resources/skybox/default_skybox.jpg",
                    "../resources/skybox/default_skybox.jpg"
            };
    CubemapTexture skyboxTexture = CubemapTexture(faces);

//
//    Model myModel("../resources/icosphere.obj");
//    glm::vec3 mesh_vertices_data[500];
//    glm::vec3 mesh_indices_data[500];
//    for (int i = 0; i<myModel.mesh.vertices.size(); i++){
//        mesh_vertices_data[i] = myModel.mesh.vertices[i].Position;
//    }
//    for (int i = 0; i<myModel.mesh.indices.size()/3; i++){
//        glm::vec3 temp;
//        temp.x = myModel.mesh.indices[3*i];
//        temp.y = myModel.mesh.indices[3*i+1];
//        temp.z = myModel.mesh.indices[3*i+2];
//        mesh_indices_data[i] = temp;
//    }
//
//    unsigned int vs_ubo = glGetUniformBlockIndex(rayTracingShader.ID, "mesh_vertices_ubo");
//    glUniformBlockBinding(rayTracingShader.ID, vs_ubo, 0);
//    unsigned int is_ubo = glGetUniformBlockIndex(rayTracingShader.ID, "mesh_tri_indices_ubo");
//    glUniformBlockBinding(rayTracingShader.ID, is_ubo, 1);

    /*
    // use this code for mesh render
    unsigned int uboMeshVertices;
    glGenBuffers(1, &uboMeshVertices);
    glBindBuffer(GL_UNIFORM_BUFFER, uboMeshVertices);
    glBufferData(GL_UNIFORM_BUFFER, 4000 * sizeof(float), mesh_vertices_data, GL_STATIC_DRAW);
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboMeshVertices, 0, 4000 * sizeof(float));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    unsigned int uboMeshIndices;
    glGenBuffers(1, &uboMeshIndices);
    glBindBuffer(GL_UNIFORM_BUFFER, uboMeshIndices);
    glBufferData(GL_UNIFORM_BUFFER, 4000 * sizeof(unsigned int), mesh_indices_data, GL_STATIC_DRAW);
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboMeshIndices, 0, 4000 * sizeof(unsigned int));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
     */

    rayTracingShader.use();
    rayTracingShader.setFloat("H", SCR_HEIGHT);
    rayTracingShader.setFloat("W", SCR_WIDTH);
    rayTracingShader.setFloat("fovY", camera.Zoom);
    rayTracingShader.setFloat("environmentMap", 0);
    rayTracingShader.setInt("meshTriangleNumber", 0);
    rayTracingShader.setVec3("ambientLightColor", glm::vec3(0.02f));


    //  set materials you can change this.
    glm::vec3 ground_color = glm::vec3(0.5 + (rand()%10)/10.0, 0.5 + (rand()%10)/10.0, 0.5 + (rand()%10)/10.0);
    ground_color = glm::normalize(ground_color);
    ground_color *= 0.8;
    ground_color *= (1 - mask_mode);
    rayTracingShader.setVec3("material_ground.Ka", glm::vec3(0));
    rayTracingShader.setVec3("material_ground.Kd", ground_color);
    rayTracingShader.setVec3("material_ground.Ks", ground_color * 0.1f);
    rayTracingShader.setFloat("material_ground.shininess", 12);
    rayTracingShader.setVec3("material_ground.R0", glm::vec3((1-mask_mode)*0.005f));
    rayTracingShader.setInt("material_ground.material_type", 0);

    rayTracingShader.setVec3("material_mirror.Ka", glm::vec3(0));
    rayTracingShader.setVec3("material_mirror.Kd", glm::vec3(0.03f, 0.03f, 0.08f));
    rayTracingShader.setVec3("material_mirror.Ks", glm::vec3(0));
    rayTracingShader.setVec3("material_mirror.R0", glm::vec3(1.0f));
    rayTracingShader.setInt("material_mirror.material_type", 0);

    rayTracingShader.setFloat("material_dielectric_glass.ior", 1.2);
    rayTracingShader.setVec3("material_dielectric_glass.extinction_constant", glm::log(glm::vec3(0.80f, 0.89f, 0.75f)));
    rayTracingShader.setVec3("material_dielectric_glass.shadow_attenuation_constant", glm::vec3(0.4f, 0.7f, 0.4f));
    rayTracingShader.setInt("material_dielectric_glass.scatter_type", 2);
    rayTracingShader.setInt("material_dielectric_glass.material_type", 1);

    glm::vec3 box_color = glm::vec3((rand()%10)/10.0, (rand()%10)/10.0, (rand()%10)/10.0);
    box_color *= (1 - mask_mode);

    rayTracingShader.setVec3("material_box.Ka", glm::vec3(0));
    rayTracingShader.setVec3("material_box.Kd", box_color);
    rayTracingShader.setVec3("material_box.Ks", box_color);
    rayTracingShader.setFloat("material_box.shininess", (1-mask_mode)*20);
    rayTracingShader.setVec3("material_box.R0", glm::vec3((1-mask_mode)*0.1f));
    rayTracingShader.setInt("material_box.material_type", 0);

    rayTracingShader.setVec3("material_lambert.Ka", glm::vec3(0));
    rayTracingShader.setVec3("material_lambert.Kd", glm::vec3(0.8, 0.8, 0.0));
    rayTracingShader.setVec3("material_lambert.Ks", glm::vec3(0));
    rayTracingShader.setVec3("material_lambert.R0", glm::vec3(0));
    rayTracingShader.setInt("material_lambert.material_type", 0);
    glm::vec3 ball_color = glm::vec3((rand()%10)/10.0, (rand()%10)/10.0, (rand()%10)/10.0);
    if (mask_mode) ball_color = glm::vec3(1);

    rayTracingShader.setVec3("material_gold.Ka", glm::vec3(0));
    rayTracingShader.setVec3("material_gold.Kd", ball_color);
    rayTracingShader.setVec3("material_gold.Ks", ball_color);
    rayTracingShader.setFloat("material_gold.shininess", (1-mask_mode)*200);
    rayTracingShader.setVec3("material_gold.R0", glm::vec3(0));
    rayTracingShader.setInt("material_gold.material_type", 0);

    rayTracingShader.setVec3("material_mesh.Ka", glm::vec3(0.2,0.07,0.02));
    rayTracingShader.setVec3("material_mesh.Kd", glm::vec3(0.7, 0.3, 0.08) * 0.001f);
    rayTracingShader.setVec3("material_mesh.Ks", glm::vec3(0.26, 0.14, 0.09));
    rayTracingShader.setFloat("material_mesh.shininess", 12.8);
    rayTracingShader.setVec3("material_mesh.R0", glm::vec3(0.1));
    rayTracingShader.setInt("material_mesh.material_type", 0);

    bool startFlag = false;
    float oldTime = 0;
    float startTime;

    bool convertFlagX = false;
    bool convertFlagZ = false;
    int frame = 0;
    char date_char[128];
    char coord_char[128];
    int finish = 800;
    int episode_len = 503;
    while (!glfwWindowShouldClose(window))// render loop
    {
        rayTracingShader.setFloat("start", start);
        float currentTime = glfwGetTime();
        if(!startFlag){
            startTime = currentTime;
            startFlag = true;
        }
//        cout << "Front: " << glm::to_string(camera.Front) <<endl;
//        cout << "UP: " << glm::to_string(camera.Up) << endl;
        float timeSpent = -startTime + currentTime;
        float dt = currentTime - oldTime;

        if(frame % episode_len == 0){
            if(frame/episode_len == finish) break;
            cout << "episode: " << frame / episode_len << endl;
            sprintf(date_char, "images/ball_episode_%03d.txt", frame / episode_len);
            sprintf(coord_char, "coords/coords_episode_%03d.txt", frame / episode_len);
            ground_color = glm::vec3(random_value(0.5,1), random_value(0.5,1), random_value(0.5,1));
            box_color = glm::vec3(random_value(0,1), random_value(0,1), random_value(0,1));
            ball_color = glm::vec3(random_value(0,1), random_value(0,1), random_value(0,1));
            rayTracingShader.setVec3("material_ground.Kd", ground_color);
            rayTracingShader.setVec3("material_ground.Ks", ground_color*0.01f);
            rayTracingShader.setVec3("material_gold.Kd", ball_color);
            rayTracingShader.setVec3("material_gold.Ks", ball_color);
            rayTracingShader.setVec3("material_box.Kd", box_color);
            rayTracingShader.setVec3("material_box.Ks", box_color);
            current_pos = glm::vec3(random_value(-4,4),0.5,random_value(-4,4));
            ball_speed = glm::vec3(random_value(0.5,4),0,random_value(0.5,5));
            ball_speed = (rand()%2==0)? ball_speed : -ball_speed;
            damp_speed = random_value(0.05, 0.2) * ball_speed;
            //if you want to eliminate friction
            //damp_speed = glm::vec3(0.0f);
        }
        frame++;

        glm::vec3 old_pos = current_pos;
        glm::vec3 old_speed = ball_speed;
        ball_speed -= damp_speed * dt;


        if (old_speed.x * ball_speed.x < 0){
            ball_speed.x = 0.0f;
            damp_speed.x = 0.0f;
        }


        if (old_speed.z * ball_speed.z < 0){
            ball_speed.z = 0.0f;
            damp_speed.z = 0.0f;
        }

        glm::vec3 noise =  random_value(0, 0.0001) * ball_speed;
        current_pos += (ball_speed + noise) * dt;
//        cout << dt << endl;

        if (current_pos.x > GUIDE_WIDTH/2.0 - 0.5){
            if(!convertFlagX) {
                float parse = abs((old_pos.x - GUIDE_WIDTH/2.0 + 0.5)/ball_speed.x);
                current_pos.x = GUIDE_WIDTH/2.0 - 0.5 - ball_speed.x * (dt - parse);
                ball_speed.x = - ball_speed.x;
                damp_speed.x = - damp_speed.x;
                convertFlagX = true;
            }
            else convertFlagX = false;
        }
        else if(current_pos.x < -GUIDE_WIDTH/2.0 + 0.5) {
            if(!convertFlagX) {
                float parse = abs((old_pos.x + GUIDE_WIDTH/2.0 - 0.5)/ball_speed.x);
                current_pos.x = -GUIDE_WIDTH/2.0 + 0.5 - ball_speed.x * (dt - parse);
                ball_speed.x = - ball_speed.x;
                damp_speed.x = - damp_speed.x;
                convertFlagX = true;
            }
            else convertFlagX = false;
        }

        if (current_pos.z > GUIDE_HEIGHT/2.0 - 0.5){
            if(!convertFlagZ) {
                float parse = abs((old_pos.z - GUIDE_HEIGHT/2.0 + 0.5)/ball_speed.z);
                current_pos.z = GUIDE_HEIGHT/2.0 - 0.5 - ball_speed.z * (dt - parse);
                ball_speed.z = - ball_speed.z;
                damp_speed.z = - damp_speed.z;
                convertFlagZ = true;
            }
            else convertFlagZ = false;
        }
        else if (current_pos.z < -GUIDE_HEIGHT/2.0 + 0.5) {
            if(!convertFlagZ) {
                float parse = abs((old_pos.z + GUIDE_HEIGHT/2.0 - 0.5)/ball_speed.z);
                current_pos.z = - GUIDE_HEIGHT/2.0 + 0.5 - ball_speed.z * (dt - parse);
                ball_speed.z = - ball_speed.z;
                damp_speed.z = - damp_speed.z;
                convertFlagZ = true;
            }
            else convertFlagZ = false;
        }
        rayTracingShader.setVec3("current_pos", current_pos);

        deltaTime = dt;
        oldTime = currentTime;
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        rayTracingShader.use();
        rayTracingShader.setVec3("cameraPosition", camera.Position);
        //cout << "x : " << camera.Position.x << " y : " << camera.Position.y << " z : " << camera.Position.z<<endl;
        rayTracingShader.setMat3("cameraToWorldRotMatrix", glm::transpose(glm::mat3(camera.GetViewMatrix())) );
        rayTracingShader.setFloat("mask_mode", mask_mode);


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture.textureID);

        glBindVertexArray(quad->ID);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // input
        processInput(window);

        saveImage(date_char);
        ofstream myfile(coord_char,ios::app);
        if(myfile.is_open()){
            myfile << current_pos.x << "," << current_pos.z << ",";
            myfile << ball_speed.x << "," << ball_speed.z << endl;
        }
        myfile.close();

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    //glDeleteVertexArrays(1,&VAOcube);
    //glDeleteBuffers(1, VBOcube);
    //glDeleteVertexArrays(1, &VAOquad);
    //glDeleteBuffers(1, &VBOquad);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

void setToggle(GLFWwindow* window, unsigned int key, bool *value) {
    if (glfwGetKey(window, key) == GLFW_PRESS && !isKeyboardDone[key]) {
        *value = !*value;
        isKeyboardDone[key] = true;
    }
    if (glfwGetKey(window, key) == GLFW_RELEASE) {
        isKeyboardDone[key] = false;
    }
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
        std::cout << "Position" << camera.Position.x << "," << camera.Position.y << "," << camera.Position.z << std::endl;
        std::cout << "Yaw" << camera.Yaw << std::endl;
    }

    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS && isKeyboardDone[GLFW_KEY_V] == false) {
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        char date_char[128];
        sprintf(date_char, "%d_%d_%d_%d_%d_%d.png", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        saveImage(date_char);
        isKeyboardDone[GLFW_KEY_V] = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_V) == GLFW_RELEASE) {
        isKeyboardDone[GLFW_KEY_V] = false;
    }


    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS && isKeyboardDone[GLFW_KEY_Z] == false)
    {
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        if (isWindowed)
            glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);
        else
            glfwSetWindowMonitor(window, NULL, 0, 0, SCR_WIDTH, SCR_HEIGHT, mode->refreshRate);
        isWindowed = !isWindowed;
        isKeyboardDone[GLFW_KEY_Z] = true;
    }
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_RELEASE) {
        isKeyboardDone[GLFW_KEY_Z] = false;
    }

    // toggle useLighting
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !changeFlagSPACE){
        start = !start;
        current_pos = glm::vec3(0,0.5,0);
        ball_speed = glm::vec3(rand()%20-10,0,rand()%20-10);
        changeFlagSPACE = true;
    }
    else if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && changeFlagSPACE){
        // key is pressed but changed at last call
        // nothing
    }
    else if (glfwGetKey(window, GLFW_KEY_SPACE) != GLFW_PRESS && changeFlagSPACE){
        changeFlagSPACE = false;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

//    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
//    camera.ProcessMouseScroll(yoffset);
}
