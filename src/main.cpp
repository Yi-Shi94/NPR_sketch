// This example is heavily based on the tutorial at https://open.gl
// OpenGL Helpers to reduce the clutter
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "Helpers.h"
#include "stb_image_write.h"
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION

// GLFW is necessary to handle the OpenGL context
#include <GLFW/glfw3.h>
#else
// GLFW is necessary to handle the OpenGL context
#include <GLFW/glfw3.h>
#endif

// Linear Algebra Library
#include <Eigen/Core>
// Timer
#include <chrono>

#include <iostream>
#include <fstream>
#include <math.h>
#include "Helpers.h"

using namespace std;
using namespace Eigen;

// VertexBufferObject wrapper
VertexBufferObject VBO;
VertexBufferObject VBO_N;

// Contains the vertex positions
TriMesh object("cube.off",0,0);
//string fname;
MatrixXf V(3,3);
MatrixXf V_normal(3,3);

MatrixXf V_trans(4,4);
Matrix4f V_view(4,4);
Matrix4f V_proj(4,4);
Matrix4f V_scope(4,4);
int ball_flag = 0;
# define pi 3.1415926
/*CONFIGURE*/

//int camera_pers_type = 0; //0,1 //perspective,ortho
int camera_pers_type = 0 ;
int screenWidth = 224;
int screenHeight = 224;
int ball = 0;

Vector3d light_pos(0.,0.,5.);
Vector3d light_color(1.,1.,1.);
Vector3d cam_pos(0.,1.,3.);
/************/

int get_viewport_image(string fname_out){
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    
    int x = viewport[0];
    int y = viewport[1];
    int width = viewport[2];
    int height = viewport[3];
    
    char *data = (char*) malloc((size_t) (width * height * 3)); // 3 components (R, G, B)
    if (!data){
        return -1;
    }
    
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(x,y,width,height,GL_RGB, GL_UNSIGNED_BYTE,data);
    int saved = stbi_write_png(fname_out.c_str(), width, height, 3, data, 0);
    
    free(data);
    cout<<"save status:"<<saved<<endl;
    return saved;
    //writeImg(fname_out,pixel,screenWidth,screenHeight);
}

MatrixXf scope_transform(float aspect_ratio){
    Matrix4f vmat;
    vmat<<aspect_ratio, 0, 0, 0,
          0,            1, 0, 0,
          0,            0, 1, 0,
          0,            0, 0, 1;
    return vmat;
}

MatrixXf view_transform(Vector3d eye, Vector3d center){
    Matrix4f camera_lookat;
    Vector3d up(0.0f,1.0f,0.0f);
    Vector3d X,Y,Z;
    Z = (eye-center).normalized();
    Y = up;
    X = Y.cross(Z);
    Y = Z.cross(X);
    
    X = X.normalized();
    Y = Y.normalized();
    
    camera_lookat<<X(0),X(1),X(2),-X.dot(eye),
                   Y(0),Y(1),Y(2),-Y.dot(eye),
                   Z(0),Z(1),Z(2),-Z.dot(eye),
                   0   ,0   ,0   ,1.0f;
    return camera_lookat;
}

MatrixXf projection_transform(int cam_type, float fov, int width, int height , float near, float far){
    Matrix4f pmat;
    
    if (cam_type==0){
        float yScale = 1./(tan(fov*3.1415926535/360));
        float xScale = yScale;
        //cout<<"xcale"<<xScale<<' '<<yScale<<endl;
        
        pmat << xScale, 0, 0, 0,
                0, yScale, 0, 0,
                0, 0,  -(near+far)/(far-near),-2*near*far/(far-near),
                0, 0, -1, 0;
    } else {
        float left  = -3;
        float right =  3;
        float up    =  3;
        float down  = -3;
        
        float xScale = 2./(right-left);
        float yScale = 2./(up-down);
        //cout<<"xcale"<<xScale<<' '<<yScale<<endl;
        pmat << xScale, 0, 0, -(right+left)/(right-left),
                 0, yScale, 0,-(up+down)/(up-down),
                 0, 0,  -2./(far-near),-(near+far)*1.0/(far-near),
                 0, 0, 0, 1;
    }
    return pmat;
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    // Get the size of the window
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    //Vector4f p_screen(xpos,height-1-ypos,0,1);
    //Vector4f p_canonical((p_screen[0]/width)*2-1,(p_screen[1]/height)*2-1,0,1);
    
}

Vector3d fromCart2Sphere(float x, float y, float z){
    //r, theta, phi
    float r     = sqrt(x*x+y*y+z*z);
    float theta = acos(z/(r+0.000001));
    float phi   = atan2(y,x);
    Vector3d spherical(r,theta,phi);
    
    return spherical;
}

Vector3d fromSphere2Cart(float r,float theta,float phi){
    float x = r*sin(theta)*cos(phi);
    float y = r*sin(theta)*sin(phi);
    float z = r*cos(theta);
    Vector3d cart(x,y,z);
    return cart;
}

void ball_move(float dr,float dtheta,float dphi){
    
    Vector3d spher = fromCart2Sphere(cam_pos(0),cam_pos(1),cam_pos(2));
    float r = dr+spher(0);
    
    float addon = dtheta*pi/180;
    float theta = spher(1)+addon;
    float phi = dphi*pi/180+spher(2);
    if (r<0) r=0;
    cout<<theta/pi*180<<' '<<phi/pi*180<<endl;
    
    Vector3d cart = fromSphere2Cart(r,theta,phi);
    cam_pos(0) = cart(0);
    cam_pos(1) = cart(1);
    cam_pos(2) = cart(2);
}

void load_mesh(string name){
    object= TriMesh(name,0,0);
}

void update_mesh(){
    MatrixXf V = object.get_matrix();
    MatrixXf V_normal = object.get_normal_matrix();
    VBO.update(V);
    VBO_N.update(V_normal);
}

void append_mesh(string name){
    load_mesh(name);
    update_mesh();
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    
}

int main(int argc, char *argv[])
{

    if (argc!=4) {
        cout<<"arg error! Correct arg: [1] file name, [2] mode, [3] input directory, [4] output directory";
        return 0;
    }
    string fname = argv[1];
    string mode = argv[2];
    string in_dir = argv[3];
    //double rotation_deg = stoi(mode);
    cout<<fname<<mode<<in_dir;
    GLFWwindow* window;
    
    // Initialize the library
    if (!glfwInit())
        return -1;
    
    // Activate supersampling
    glfwWindowHint(GLFW_SAMPLES, 8);

    // Ensure that we get at least a 3.2 context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

    // On apple we have to load a core profile with forward compatibility
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create a windowed mode window and its OpenGL context
    window = glfwCreateWindow(screenWidth,screenHeight, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);
    
    #ifndef __APPLE__
      glewExperimental = true;
      GLenum err = glewInit();
      if(GLEW_OK != err)
      {
        /* Problem: glewInit failed, something is seriously wrong. */
       fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
      }
      glGetError(); // pull and savely ignonre unhandled errors like GL_INVALID_ENUM
      fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
    #endif
    
    //glDepthFunc(GL_LESS);
    int major, minor, rev;
    major = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MAJOR);
    minor = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MINOR);
    rev = glfwGetWindowAttrib(window, GLFW_CONTEXT_REVISION);
    printf("OpenGL version recieved: %d.%d.%d\n", major, minor, rev);
    printf("Supported OpenGL is %s\n", (const char*)glGetString(GL_VERSION));
    printf("Supported GLSL is %s\n", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
    
    // Initialize the VAO
    // A Vertex Array Object (or VAO) is an object that describes how the vertex
    // attributes are stored in a Vertex Buffer Object (or VBO). This means that
    // the VAO is not the actual object storing the vertex data,
    // but the descriptor of the vertex data.
    //glEnable(GL_DEPTH_TEST);
    VertexArrayObject VAO(0);
    VAO.init();
    VAO.bind();

    // Initialize the VBO with the vertices data
    // A VBO is a data container that lives in the GPU memory
    VBO.init();
    VBO_N.init();
    
    V.resize(3,3);
    V <<0,0,0,
        0,0,0,
        0,0,0;
    VBO.update(V);

    V_normal.resize(3,3);
    V_normal<<0,0,0,
              0,0,0,
              0,0,0;
    
    VBO_N.update(V_normal);
    
    V_trans<<1,0,0,0,
             0,1,0,0,
             0,0,1,0,
             0,0,0,1;
    
    // Initialize the OpenGL Program
    // A program controls the OpenGL pipeline and it must contains
    // at least a vertex shader and a fragment shader to be valid
    Program program;
    const GLchar* vertex_shader =
            "#version 150 core\n"
                    "in vec3 position;"
                    "in vec3 normal_local;"
    
                    "out vec3 normal;"
                    "out vec3 frag_pos;"
    
                    "uniform mat4 scope;"
                    "uniform mat4 trans;"
                    "uniform mat4 view;"
                    "uniform mat4 proj;"
    
                    "void main()"
                    "{"
                    "    frag_pos = vec3(trans * vec4(position, 1.0));"
                    "    gl_Position = scope*proj*view*trans*vec4(position, 1.0);"
                    "    normal = mat3(transpose(inverse(trans))) * normal_local;"
                    "}";
    
    
    const GLchar* fragment_shader;
    if (mode == "depth"){
        // depth buffer
        fragment_shader =
        "#version 150 core\n"
        "in vec3 normal;"
        "in vec3 frag_pos;"
        "in vec3 f_color;"

        "out vec4 outColor;"
        "uniform vec3 mesh_color;"
        "uniform vec3 light_color;"
        "uniform vec3 light_pos;"
        "uniform vec3 cam_pos;"
    
        "void main()"
        "{"
    
        "    float ambient_weight =  1.0;"
        //"    float specular_weight = 0.9;"
        //"    float diff_weight =     1.1;"
        "    vec3 ambient_cont = ambient_weight * light_color;"
        "    vec3 normed_normal = normalize(normal);"
        "    vec3 rgb_normal = normed_normal * 0.5 + 0.5;"
        "    outColor = vec4(vec3(frag_pos.z), 1.0);"
        //"    outColor = vec4(rgb_normal, 1.0);"
        //"    outColor = vec4(mesh_color * (ambient_cont), 1.0);"
        "}";
    } else if(mode == "normal") {
        // normal buffer
        fragment_shader =
        "#version 150 core\n"
        "in vec3 normal;"
        "in vec3 frag_pos;"
        "in vec3 f_color;"
        
        "out vec4 outColor;"
        "uniform vec3 mesh_color;"
        "uniform vec3 light_color;"
        "uniform vec3 light_pos;"
        "uniform vec3 cam_pos;"
        
        "void main()"
        "{"
        
        "    float ambient_weight =  1.0;"
        //"    float specular_weight = 0.9;"
        //"    float diff_weight =     1.1;"
        "    vec3 ambient_cont = ambient_weight * light_color;"
        "    vec3 normed_normal = normalize(normal);"
        "    vec3 rgb_normal = normed_normal * 0.5 + 0.5;"
        //"    outColor = vec4(vec3(frag_pos.z), 1.0);"
        "    outColor = vec4(rgb_normal, 1.0);"
        //"    outColor = vec4(mesh_color * (ambient_cont), 1.0);"
        "}";
    } else if (mode=="wireframe"){
        // .off mesh as wireframe
        fragment_shader =
        "#version 150 core\n"
        "in vec3 normal;"
        "in vec3 frag_pos;"
        "in vec3 f_color;"
        
        "out vec4 outColor;"
        "uniform vec3 mesh_color;"
        "uniform vec3 light_color;"
        "uniform vec3 light_pos;"
        "uniform vec3 cam_pos;"
        
        "void main()"
        "{"
        
        "    float ambient_weight =  1.0;"
        //"    float specular_weight = 0.9;"
        //"    float diff_weight =     1.1;"
        "    vec3 ambient_cont = ambient_weight * light_color;"
        "    vec3 normed_normal = normalize(normal);"
        "    vec3 rgb_normal = normed_normal * 0.5 + 0.5;"
        "    outColor = vec4(mesh_color * (ambient_cont), 1.0);"
        "}";
    } else {
        cout<<"mode error! supported: 1) depth, 2)normal, 3)wireframe "<<endl;
        return 0;
    }
    
    // Compile the two shaders and upload the binary to the GPU
    // Note that we have to explicitly specify that the output "slot" called outColor
    // is the one that we want in the fragment buffer (and thus on screen)
    
    program.init(vertex_shader,fragment_shader,"outColor");
    program.bind();
    
    // The vertex shader wants the position of the vertices as an input.
    // The following line connects the VBO we defined above with the position "slot"
    // in the vertex shader
    program.bindVertexAttribArray("position",VBO);
    program.bindVertexAttribArray("normal_local",VBO_N);
    glUniform3f(program.uniform("light_color"),
                light_color(0),light_color(1),light_color(2));
    glUniform3f(program.uniform("light_pos"),
                light_pos(0),light_pos(1),light_pos(2));
    
    // Register the keyboard callback
    glfwSetKeyCallback(window, key_callback);
    // Register the mouse callback
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    // Update viewport
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Bind your program
    VAO.bind();
    program.bind();
    //int width, height;
    append_mesh(fname+".off");
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glfwSwapBuffers(window);
    glfwPollEvents();
    
    for (int i=0;i<360;i=i+15){
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        object.set_trans_mat(0,0,0,0,i*1.0,0,1);
        update_mesh();
    
        glfwGetWindowSize(window, &screenWidth, &screenHeight);
        float aspect_ratio = float(screenHeight)/float(screenWidth);
        
        V_scope = scope_transform(aspect_ratio);
        glUniformMatrix4fv(program.uniform("scope"), 1, GL_FALSE, V_scope.data());
        
        Vector3d center(0.0f,0.0f,0.0f);
        V_view = view_transform(cam_pos,center);
        glUniformMatrix4fv(program.uniform("view"), 1, GL_FALSE,  V_view.data());
        
        V_proj = projection_transform(camera_pers_type, 60, screenWidth,screenHeight, 0.1, 100);
        glUniformMatrix4fv(program.uniform("proj"), 1, GL_FALSE,  V_proj.data());
        
        glUniform3f(program.uniform("cam_pos"),
                    cam_pos(0),cam_pos(1),cam_pos(2));
        
        MatrixXf trans = object.get_trans_mat();
        glUniformMatrix4fv(program.uniform("trans"), 1, GL_FALSE,trans.data());
        
        glUniform3f(program.uniform("mesh_color"),0.0f,0.0f,0.0f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawArrays(GL_TRIANGLES,object.start,object.tri_num*3);
        
        glUniform3f(program.uniform("mesh_color"),1.0f,1.0f,1.0f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDrawArrays(GL_TRIANGLES,object.start,object.tri_num*3);
        
        int flag = get_viewport_image("data_tmp/"+fname+"_"+mode+"_"+to_string(i)+".png");
        cout<<i<<endl;
        glfwSwapBuffers(window);
        // Poll for and process events
        glfwPollEvents();
    }
    
    // Deallocate opengl memory
    program.free();
    
    VAO.free();
    VBO.free();
    VBO_N.free();
    
    // Deallocate glfw internals
    glfwTerminate();
    return 0;
}
