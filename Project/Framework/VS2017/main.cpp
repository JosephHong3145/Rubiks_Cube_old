
#include <iostream>
#include <list>
#include <map>
#include <fstream>

#define GLEW_STATIC 1
#include <GL/glew.h>
#include <GLFW/glfw3.h>


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/common.hpp>
#include "shader_compile.h"
#include "stb_image.h"
#include "Cube.h"
#include <array>
#include <vector>
#define _USE_MATH_DEFINES //so I can use math constants like PI
#include <math.h>
#include "Sphere.h"
#include "shaderloader.h"
#include "Rubiks_Cube.h"
#include <irrKlang.h>
#pragma comment(lib, "irrKlang.lib") // link with irrKlang.dll

#include <ft2build.h>
#include FT_FREETYPE_H

using namespace glm;
using namespace std;
using namespace irrklang;

bool initContext();
GLuint loadTexture(const char* filename);

float RNGpos();
vec3 crossProduct(vec3, vec3, vec3);
vector<vec3> generateSphere(float, int);

void RenderText(Shader& shader, string text, float x, float y, float scale, vec3 color);

GLFWwindow* window = NULL;

//different settings you can change
float normalCameraSpeed = 10.0f;
float fastCameraSpeed = 50.0f;
float FOV = 70.0f;
const float mouseSensitivity = 50.0f;

// settings
const unsigned int SCREEN_WIDTH = 1024;
const unsigned int SCREEN_HEIGHT = 768;


vec3 defaultSize = vec3(1.0f, 6.5f, 1.0f);

// lighting
//vec3 lightPos = vec3(0.0f, 30.0f, 0.0f);
vec3 lightPos = vec3(0.0f, 1.0f, 30.0f);
vec3 lightFocus(0, 0, -1); // the point in 3D space the light "looks" at

struct Character {
    unsigned int TextureID; //Id handle of the glyph texture
    ivec2 Size;             //Size of glyph
    ivec2 Bearing;          //Offset from baseline to left/top of glyph
    unsigned int Advance;   //Horizontal offset to advance to next glyph
};

map<GLchar, Character> Characters;
GLuint charVAO, charVBO;

vec3 selectCord = vec3(0.0f); //coordinate of the red "select" cube
int gameState = 0; // 0 = idle, 1 = won, 2 = loss, 3 = ongoing

int main()
{
    if (!initContext()) return -1;

    // start the sound engine with default parameters
    ISoundEngine* engine = createIrrKlangDevice();

    // play some sound stream, looped
    engine->play2D("gameSound.mp3", true);

    // Black background
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Compile and link shaders here
    Shader AffectedByLightingShader("AffectedByLighting.vert", "AffectedByLighting.frag");
    Shader NotAffectedByLightingShader("NotAffectedByLighting.vert", "NotAffectedByLighting.frag");
    Shader ShadowShader("shadowVertex.glsl", "shadowFragment.glsl");
    Shader TextShader("text.vert", "text.frag");

    //Initiating camera
    vec3 cameraPosition(0.6f, 1.0f, 10.0f);
    vec3 cameraLookAt(0.0f, 0.0f, -1.0f);
    vec3 cameraUp(0.0f, 1.0f, 0.0f);
    float cameraHorizontalAngle = 90.0f;
    float cameraVerticalAngle = 0.0f;
    float cameraTiltAngle = 90.0f;
    float cameraSpeed = 0.0f;

    // Set projection matrix for shader, this won't change


    //Freetype
    FT_Library ft;
    //All functions return a value different than 0 whenever an error occured
    if (FT_Init_FreeType(&ft))
    {
        cout << "ERROR::FREETYPE: Could not init FreeType Library" << endl;
        return -1;
    }

    // find path to font
    string font_name = "C:/Windows/Fonts/ariblk.ttf";
    if (font_name.empty())
    {
        std::cout << "ERROR::FREETYPE: Failed to load font_name" << std::endl;
        return -1;
    }


    // load font as face
    FT_Face face;
    if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
        cout << "ERROR::FREETYPE: Failed to load font" << endl;
        return -1;
    }
    else {
        // set size to load glyphs as
        FT_Set_Pixel_Sizes(face, 0, 48);

        // disable byte-alignment restriction
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // load first 128 characters of ASCII set
        for (unsigned char c = 0; c < 128; c++)
        {
            // Load character glyph
            if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            {
                cout << "ERROR::FREETYTPE: Failed to load Glyph" << endl;
                continue;
            }
            // generate texture
            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );
            // set texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // now store character for later use
            Character character = {
                texture,
                ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                face->glyph->advance.x
            };
            Characters.insert(pair<char, Character>(c, character));
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    // destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    //configure VAO/VBO for texture quads
    glGenVertexArrays(1, &charVAO);
    glGenBuffers(1, &charVBO);
    glBindVertexArray(charVAO);
    glBindBuffer(GL_ARRAY_BUFFER, charVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Set initial view matrix
    mat4 viewMatrix = lookAt(cameraPosition, cameraPosition + cameraLookAt, cameraUp);

    AffectedByLightingShader.use();
    AffectedByLightingShader.setMat4("viewMatrix", viewMatrix);

    NotAffectedByLightingShader.use();
    NotAffectedByLightingShader.setMat4("viewMatrix", viewMatrix);

    struct Vertex
    {
        Vertex(vec3 _position, vec3 _color, vec3 _normal, vec2 _uv)
            : position(_position), color(_color), normal(_normal), uv(_uv) {}

        vec3 position;
        vec3 color;
        vec3 normal;
        vec2 uv;
    };

    const unsigned int DEPTH_MAP_TEXTURE_SIZE = 1024;

    ///////////////////////////////////////SHADOW MAPPING///////////////////////////////////////

      // Variable storing index to framebuffer used for shadow mapping
    GLuint depth_map_texture;
    // Get the texture
    glGenTextures(1, &depth_map_texture);
    // Bind the texture so the next glTex calls affect it
    glBindTexture(GL_TEXTURE_2D, depth_map_texture);
    // Create the texture and specify it's attributes, including width and height, components (only depth is stored, no colour information)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, DEPTH_MAP_TEXTURE_SIZE, DEPTH_MAP_TEXTURE_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    // Set texture sampler parameters
    // The two calls below tell the texture sampler the shader how to upsample and downsample the texture. Here we choose the
    //    nearest filtering option, which means we just use the value of the closest pixel to the chosen image coordinate
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //The two calls below tell the texture sampler inside the shader how it should deal with texture coordinates outside of the [0, 1]
    //    range. Here we decide to just tile the image.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    ////////////////////////////////DEPTH MAP////////////////////////////////////////////

    // Variable storing index to framebuffer used for shadow mapping
    GLuint depth_map_fbo; //fbo: framebuffer object
    // Get the flamebuffer
    glGenFramebuffers(1, &depth_map_fbo);
    // Bind the framebuffer so the next glFramebuffer calls affect it
    glBindFramebuffer(GL_FRAMEBUFFER, depth_map_fbo);
    //Attach the depth map texture to the depth map framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_map_fbo, 0);
    glDrawBuffer(GL_NONE); // Disable rendering colors, only write depth values

    const Vertex vertexArray[] = {
        // position,                            color                        normal                                                                                                       uv
        Vertex(vec3(-0.5f,-0.5f,-0.5f), vec3(0.6f, 0.6f, 0.6f), crossProduct(vec3(-0.5f,-0.5f,-0.5f), vec3(-0.5f,-0.5f, 0.5f), vec3(-0.5f, 0.5f, 0.5f)),  vec2(0.0f, 0.0f)),
        Vertex(vec3(-0.5f,-0.5f, 0.5f), vec3(0.3f, 0.3f, 0.3f), crossProduct(vec3(-0.5f,-0.5f,-0.5f), vec3(-0.5f,-0.5f, 0.5f), vec3(-0.5f, 0.5f, 0.5f)),  vec2(0.0f, 1.0f)),
        Vertex(vec3(-0.5f, 0.5f, 0.5f), vec3(0.1f, 0.1f, 0.1f), crossProduct(vec3(-0.5f,-0.5f,-0.5f), vec3(-0.5f,-0.5f, 0.5f), vec3(-0.5f, 0.5f, 0.5f)),  vec2(1.0f, 1.0f)),

        Vertex(vec3(-0.5f,-0.5f,-0.5f), vec3(0.6f, 0.6f, 0.6f), crossProduct(vec3(-0.5f,-0.5f,-0.5f), vec3(-0.5f, 0.5f, 0.5f), vec3(-0.5f, 0.5f,-0.5f)),  vec2(0.0f, 0.0f)),
        Vertex(vec3(-0.5f, 0.5f, 0.5f), vec3(0.3f, 0.3f, 0.3f), crossProduct(vec3(-0.5f,-0.5f,-0.5f), vec3(-0.5f, 0.5f, 0.5f), vec3(-0.5f, 0.5f,-0.5f)),  vec2(1.0f, 1.0f)),
        Vertex(vec3(-0.5f, 0.5f,-0.5f), vec3(0.1f, 0.1f, 0.1f), crossProduct(vec3(-0.5f,-0.5f,-0.5f), vec3(-0.5f, 0.5f, 0.5f), vec3(-0.5f, 0.5f,-0.5f)),  vec2(1.0f, 0.0f)),

        Vertex(vec3(0.5f, 0.5f,-0.5f), vec3(0.6f, 0.6f, 0.6f), crossProduct(vec3(0.5f, 0.5f,-0.5f), vec3(-0.5f,-0.5f,-0.5f), vec3(-0.5f, 0.5f,-0.5f)),  vec2(1.0f, 1.0f)),
        Vertex(vec3(-0.5f,-0.5f,-0.5f), vec3(0.3f, 0.3f, 0.3f), crossProduct(vec3(0.5f, 0.5f,-0.5f), vec3(-0.5f,-0.5f,-0.5f), vec3(-0.5f, 0.5f,-0.5f)),  vec2(0.0f, 0.0f)),
        Vertex(vec3(-0.5f, 0.5f,-0.5f), vec3(0.1f, 0.1f, 0.1f), crossProduct(vec3(0.5f, 0.5f,-0.5f), vec3(-0.5f,-0.5f,-0.5f), vec3(-0.5f, 0.5f,-0.5f)),  vec2(0.0f, 1.0f)),

        Vertex(vec3(0.5f, 0.5f,-0.5f), vec3(0.6f, 0.6f, 0.6f), crossProduct(vec3(0.5f, 0.5f,-0.5f), vec3(0.5f,-0.5f,-0.5f), vec3(-0.5f,-0.5f,-0.5f)),  vec2(1.0f, 1.0f)),
        Vertex(vec3(0.5f,-0.5f,-0.5f), vec3(0.3f, 0.3f, 0.3f), crossProduct(vec3(0.5f, 0.5f,-0.5f), vec3(0.5f,-0.5f,-0.5f), vec3(-0.5f,-0.5f,-0.5f)),  vec2(1.0f, 0.0f)),
        Vertex(vec3(-0.5f,-0.5f,-0.5f), vec3(0.1f, 0.1f, 0.1f), crossProduct(vec3(0.5f, 0.5f,-0.5f), vec3(0.5f,-0.5f,-0.5f), vec3(-0.5f,-0.5f,-0.5f)),  vec2(0.0f, 0.0f)),

        Vertex(vec3(0.5f,-0.5f, 0.5f), vec3(0.6f, 0.6f, 0.6f), crossProduct(vec3(0.5f,-0.5f, 0.5f), vec3(-0.5f,-0.5f,-0.5f), vec3(0.5f,-0.5f,-0.5f)), vec2(1.0f, 1.0f)),
        Vertex(vec3(-0.5f,-0.5f,-0.5f), vec3(0.3f, 0.3f, 0.3f), crossProduct(vec3(0.5f,-0.5f, 0.5f), vec3(-0.5f,-0.5f,-0.5f), vec3(0.5f,-0.5f,-0.5f)), vec2(0.0f, 0.0f)),
        Vertex(vec3(0.5f,-0.5f,-0.5f), vec3(0.1f, 0.1f, 0.1f), crossProduct(vec3(0.5f,-0.5f, 0.5f), vec3(-0.5f,-0.5f,-0.5f), vec3(0.5f,-0.5f,-0.5f)), vec2(1.0f, 0.0f)),

        Vertex(vec3(0.5f,-0.5f, 0.5f), vec3(0.6f, 0.6f, 0.6f), crossProduct(vec3(0.5f,-0.5f, 0.5f), vec3(-0.5f,-0.5f, 0.5f), vec3(-0.5f,-0.5f,-0.5f)), vec2(1.0f, 1.0f)),
        Vertex(vec3(-0.5f,-0.5f, 0.5f), vec3(0.3f, 0.3f, 0.3f), crossProduct(vec3(0.5f,-0.5f, 0.5f), vec3(-0.5f,-0.5f, 0.5f), vec3(-0.5f,-0.5f,-0.5f)), vec2(0.0f, 1.0f)),
        Vertex(vec3(-0.5f,-0.5f,-0.5f), vec3(0.1f, 0.1f, 0.1f), crossProduct(vec3(0.5f,-0.5f, 0.5f), vec3(-0.5f,-0.5f, 0.5f), vec3(-0.5f,-0.5f,-0.5f)), vec2(0.0f, 0.0f)),

        Vertex(vec3(-0.5f, 0.5f, 0.5f), vec3(0.6f, 0.6f, 0.6f), crossProduct(vec3(-0.5f, 0.5f, 0.5f), vec3(-0.5f,-0.5f, 0.5f), vec3(0.5f,-0.5f, 0.5f)),  vec2(0.0f, 1.0f)),
        Vertex(vec3(-0.5f,-0.5f, 0.5f), vec3(0.3f, 0.3f, 0.3f), crossProduct(vec3(-0.5f, 0.5f, 0.5f), vec3(-0.5f,-0.5f, 0.5f), vec3(0.5f,-0.5f, 0.5f)),  vec2(0.0f, 0.0f)),
        Vertex(vec3(0.5f,-0.5f, 0.5f), vec3(0.1f, 0.1f, 0.1f), crossProduct(vec3(-0.5f, 0.5f, 0.5f), vec3(-0.5f,-0.5f, 0.5f), vec3(0.5f,-0.5f, 0.5f)),  vec2(1.0f, 0.0f)),

        Vertex(vec3(0.5f, 0.5f, 0.5f), vec3(0.6f, 0.6f, 0.6f), crossProduct(vec3(0.5f, 0.5f, 0.5f), vec3(-0.5f, 0.5f, 0.5f), vec3(0.5f,-0.5f, 0.5f)),  vec2(1.0f, 1.0f)),
        Vertex(vec3(-0.5f, 0.5f, 0.5f), vec3(0.3f, 0.3f, 0.3f), crossProduct(vec3(0.5f, 0.5f, 0.5f), vec3(-0.5f, 0.5f, 0.5f), vec3(0.5f,-0.5f, 0.5f)),  vec2(0.0f, 1.0f)),
        Vertex(vec3(0.5f,-0.5f, 0.5f), vec3(0.1f, 0.1f, 0.1f), crossProduct(vec3(0.5f, 0.5f, 0.5f), vec3(-0.5f, 0.5f, 0.5f), vec3(0.5f,-0.5f, 0.5f)),  vec2(1.0f, 0.0f)),

        Vertex(vec3(0.5f, 0.5f, 0.5f), vec3(0.6f, 0.6f, 0.6f), crossProduct(vec3(0.5f, 0.5f, 0.5f), vec3(0.5f,-0.5f,-0.5f), vec3(0.5f, 0.5f,-0.5f)),  vec2(1.0f, 1.0f)),
        Vertex(vec3(0.5f,-0.5f,-0.5f), vec3(0.3f, 0.3f, 0.3f), crossProduct(vec3(0.5f, 0.5f, 0.5f), vec3(0.5f,-0.5f,-0.5f), vec3(0.5f, 0.5f,-0.5f)),  vec2(0.0f, 0.0f)),
        Vertex(vec3(0.5f, 0.5f,-0.5f), vec3(0.1f, 0.1f, 0.1f), crossProduct(vec3(0.5f, 0.5f, 0.5f), vec3(0.5f,-0.5f,-0.5f), vec3(0.5f, 0.5f,-0.5f)),  vec2(1.0f, 0.0f)),

        Vertex(vec3(0.5f,-0.5f,-0.5f), vec3(0.6f, 0.6f, 0.6f), crossProduct(vec3(0.5f,-0.5f,-0.5f), vec3(0.5f, 0.5f, 0.5f), vec3(0.5f,-0.5f, 0.5f)),  vec2(0.0f, 0.0f)),
        Vertex(vec3(0.5f, 0.5f, 0.5f), vec3(0.3f, 0.3f, 0.3f), crossProduct(vec3(0.5f,-0.5f,-0.5f), vec3(0.5f, 0.5f, 0.5f), vec3(0.5f,-0.5f, 0.5f)),  vec2(1.0f, 1.0f)),
        Vertex(vec3(0.5f,-0.5f, 0.5f), vec3(0.1f, 0.1f, 0.1f), crossProduct(vec3(0.5f,-0.5f,-0.5f), vec3(0.5f, 0.5f, 0.5f), vec3(0.5f,-0.5f, 0.5f)),  vec2(0.0f, 1.0f)),

        Vertex(vec3(0.5f, 0.5f, 0.5f), vec3(0.6f, 0.6f, 0.6f), crossProduct(vec3(0.5f, 0.5f, 0.5f), vec3(0.5f, 0.5f,-0.5f), vec3(-0.5f, 0.5f,-0.5f)),  vec2(1.0f, 1.0f)),
        Vertex(vec3(0.5f, 0.5f,-0.5f), vec3(0.3f, 0.3f, 0.3f), crossProduct(vec3(0.5f, 0.5f, 0.5f), vec3(0.5f, 0.5f,-0.5f), vec3(-0.5f, 0.5f,-0.5f)),  vec2(1.0f, 0.0f)),
        Vertex(vec3(-0.5f, 0.5f,-0.5f), vec3(0.1f, 0.1f, 0.1f), crossProduct(vec3(0.5f, 0.5f, 0.5f), vec3(0.5f, 0.5f,-0.5f), vec3(-0.5f, 0.5f,-0.5f)),  vec2(0.0f, 0.0f)),

        Vertex(vec3(0.5f, 0.5f, 0.5f), vec3(0.6f, 0.6f, 0.6f), crossProduct(vec3(0.5f, 0.5f, 0.5f), vec3(-0.5f, 0.5f,-0.5f), vec3(-0.5f, 0.5f, 0.5f)),  vec2(1.0f, 1.0f)),
        Vertex(vec3(-0.5f, 0.5f,-0.5f), vec3(0.3f, 0.3f, 0.3f), crossProduct(vec3(0.5f, 0.5f, 0.5f), vec3(-0.5f, 0.5f,-0.5f), vec3(-0.5f, 0.5f, 0.5f)),  vec2(0.0f, 0.0f)),
        Vertex(vec3(-0.5f, 0.5f, 0.5f), vec3(0.1f, 0.1f, 0.1f), crossProduct(vec3(0.5f, 0.5f, 0.5f), vec3(-0.5f, 0.5f,-0.5f), vec3(-0.5f, 0.5f, 0.5f)),  vec2(0.0f, 1.0f)),
    };

    // Create a vertex array
    GLuint cubeVAO;
    glGenVertexArrays(1, &cubeVAO);
    glBindVertexArray(cubeVAO);

    // Upload Vertex Buffer to the GPU, keep a reference to it (vertexBufferObject)
    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexArray), vertexArray, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)sizeof(vec3));
    glEnableVertexAttribArray(1);

    // Normal attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(vec3)));
    glEnableVertexAttribArray(2);

    // Texture attribute
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(vec3)));
    glEnableVertexAttribArray(3);

    vector<vec3> sphereArray = generateSphere(1.0f, 48);

    //creating sphere VAO:
    GLuint sphereVAO;
    glGenVertexArrays(1, &sphereVAO);
    glBindVertexArray(sphereVAO);

    // Upload Vertex Buffer to the GPU, keep a reference to it (vertexBufferObject)
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sphereArray.size() * sizeof(vec3), &sphereArray[0], GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(vec3), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(vec3), (void*)sizeof(vec3));
    glEnableVertexAttribArray(1);

    // Normal attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(vec3), (void*)(2 * sizeof(vec3)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(vec3)));
    glEnableVertexAttribArray(3);


    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindVertexArray(0);

    const Vertex triangleArray[] = {
        Vertex(vec3(-1, -1, 0), vec3(0.6f, 0.6f, 0.6f), crossProduct(vec3(-1, -1, 0), vec3(1, -1, 0), vec3(0, 1, 0)),  vec2(0.0f, 0.0f)),
        Vertex(vec3(1, -1, 0), vec3(0.3f, 0.3f, 0.3f), crossProduct(vec3(-1, -1, 0), vec3(1, -1, 0), vec3(0, 1, 0)),  vec2(0.0f, 1.0f)),
        Vertex(vec3(0, 1, 0), vec3(0.1f, 0.1f, 0.1f), crossProduct(vec3(-1, -1, 0), vec3(1, -1, 0), vec3(0, 1, 0)),  vec2(1.0f, 1.0f)),

        Vertex(vec3(-1, -1, 0), vec3(0.6f, 0.6f, 0.6f), -crossProduct(vec3(-1, -1, 0), vec3(1, -1, 0), vec3(0, 1, 0)),  vec2(0.0f, 0.0f)),
        Vertex(vec3(0, 1, 0), vec3(0.3f, 0.3f, 0.3f), -crossProduct(vec3(-1, -1, 0), vec3(1, -1, 0), vec3(0, 1, 0)),  vec2(1.0f, 1.0f)),
        Vertex(vec3(1, -1, 0), vec3(0.1f, 0.1f, 0.1f), -crossProduct(vec3(-1, -1, 0), vec3(1, -1, 0), vec3(0, 1, 0)),  vec2(1.0f, 0.0f))
    };

    //creating triangle VAO
    GLuint triangleVAO;
    glGenVertexArrays(1, &triangleVAO);
    glBindVertexArray(triangleVAO);

    // Upload Vertex Buffer to the GPU, keep a reference to it (vertexBufferObject)
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangleArray), triangleArray, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)sizeof(vec3));
    glEnableVertexAttribArray(1);

    // Normal attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(vec3)));
    glEnableVertexAttribArray(2);

    // Texture attribute
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(vec3)));
    glEnableVertexAttribArray(3);


    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindVertexArray(0);


    // second, configure the light's VAO (VBO stays the same; the vertices are the same for the light object which is also a 3D cube)
    GLuint lightSourceVAO;
    glGenVertexArrays(1, &lightSourceVAO);
    glBindVertexArray(lightSourceVAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // note that we update the lamp's position attribute's stride to reflect the updated buffer data ( 9 * sizeof(vec3) since we only care about position here)

    //loading texture pack
     // Load Textures
#if defined(PLATFORM_OSX)
    //color
    GLuint redTextureID = loadTexture("Textures/red.bmp");
    GLuint yellowTextureID = loadTexture("Textures/yellow.bmp");
    GLuint greenTextureID = loadTexture("Textures/green.bmp");
    GLuint whiteTextureID = loadTexture("Textures/white.bmp");
    GLuint blueTextureID = loadTexture("Textures/blue.bmp");
    GLuint orangeTextureID = loadTexture("Textures/orange.bmp");
    GLuint skyboxTextureID = loadTexture("Textures/skybox.jpg");
    GLuint selectCubeID = loadTexture("Textures/select.png");

    //shapes
    GLuint squareID = loadTexture("Textures/square.png");
    GLuint circleID = loadTexture("Textures/circle.png");
    GLuint triangleID = loadTexture("Textures/triangle.png");
    GLuint rhombusID = loadTexture("Textures/rhombus.png");
    GLuint pentagonID = loadTexture("Textures/pentagon.png");
    GLuint hexagonID = loadTexture("Textures/hexagon.png");

    // Animal rubiks cube
    GLuint zebraCubeID = loadTexture("Textures/zebra.png");
    GLuint lionCubeID = loadTexture("Textures/lion.jpg");
    GLuint giraffeCubeID = loadTexture("Textures/giraffe.jpg");
    GLuint leopardCubeID = loadTexture("Textures/leopard.jpg");
    GLuint snakeCubeID = loadTexture("Textures/snake.png");
    GLuint whiteLeopardCubeID = loadTexture("Textures/whiteLeopard.png");

    // Movies rubiks cube
    GLuint backIntoTheFutureCubeID = loadTexture("Textures/backIntoTheFuture.jpg");
    GLuint elfCubeID = loadTexture("Textures/elf.jpg");
    GLuint harryPotterCubeID = loadTexture("Textures/harryPotter.jpg");
    GLuint homeAloneCubeID = loadTexture("Textures/homeAlone.jpg");
    GLuint spaceJamCubeID = loadTexture("Textures/spaceJam.png");
    GLuint lordOfTheRingCubeID = loadTexture("Textures/lordOfTheRing.jpg");

    // Gaming characters rubiks cube
    GLuint marioCubeID = loadTexture("Textures/mario.png");
    GLuint nessCubeID = loadTexture("Textures/ness.png");
    GLuint pikachuCubeID = loadTexture("Textures/pikachu.png");
    GLuint sonicCubeID = loadTexture("Textures/sonic.png");
    GLuint pacmanCubeID = loadTexture("Textures/pacman.png");
    GLuint zeldaCubeID = loadTexture("Textures/zelda.png");

    // Skybox texture
    GLuint skyboxTextureID = loadTexture("Textures/skybox.jpg");

    // Selector texture
    GLuint selectCubeID = loadTexture("../Assets/Textures/select.png");

#else
    // Color rubiks cube
    GLuint redTextureID = loadTexture("../Assets/Textures/red.bmp");
    GLuint yellowTextureID = loadTexture("../Assets/Textures/yellow.bmp");
    GLuint greenTextureID = loadTexture("../Assets/Textures/green.bmp");
    GLuint whiteTextureID = loadTexture("../Assets/Textures/white.bmp");
    GLuint blueTextureID = loadTexture("../Assets/Textures/blue.bmp");
    GLuint orangeTextureID = loadTexture("../Assets/Textures/orange.bmp");

    // Animal rubiks cube
    GLuint zebraCubeID = loadTexture("../Assets/Textures/zebra.png");
    GLuint lionCubeID = loadTexture("../Assets/Textures/lion.jpg");
    GLuint giraffeCubeID = loadTexture("../Assets/Textures/giraffe.jpg");
    GLuint leopardCubeID = loadTexture("../Assets/Textures/leopard.jpg");
    GLuint snakeCubeID = loadTexture("../Assets/Textures/snake.png");
    GLuint whiteLeopardCubeID = loadTexture("../Assets/Textures/whiteLeopard.png");

    // Movies rubiks cube
    GLuint backIntoTheFutureCubeID = loadTexture("../Assets/Textures/backIntoTheFuture.jpg");
    GLuint elfCubeID = loadTexture("../Assets/Textures/elf.jpg");
    GLuint harryPotterCubeID = loadTexture("../Assets/Textures/harryPotter.jpg");
    GLuint homeAloneCubeID = loadTexture("../Assets/Textures/homeAlone.jpg");
    GLuint spaceJamCubeID = loadTexture("../Assets/Textures/spaceJam.png");
    GLuint lordOfTheRingCubeID = loadTexture("../Assets/Textures/lordOfTheRing.jpg");

    // Gaming characters rubiks cube
    GLuint marioCubeID = loadTexture("../Assets/Textures/mario.png");
    GLuint nessCubeID = loadTexture("../Assets/Textures/ness.png");
    GLuint pikachuCubeID = loadTexture("../Assets/Textures/pikachu.png");
    GLuint sonicCubeID = loadTexture("../Assets/Textures/sonic.png");
    GLuint pacmanCubeID = loadTexture("../Assets/Textures/pacman.png");
    GLuint zeldaCubeID = loadTexture("../Assets/Textures/zelda.png");

    // Skybox texture
    GLuint skyboxTextureID = loadTexture("../Assets/Textures/skybox.jpg");

    // Selector texture
    GLuint selectCubeID = loadTexture("../Assets/Textures/select.png");

    GLuint squareID = loadTexture("../Assets/Textures/square.png");
    GLuint circleID = loadTexture("../Assets/Textures/circle.png");
    GLuint rhombusID = loadTexture("../Assets/Textures/rhombus.png");
    GLuint triangleID = loadTexture("../Assets/Textures/triangle.png");
    GLuint pentagonID = loadTexture("../Assets/Textures/pentagon.png");
    GLuint hexagonID = loadTexture("../Assets/Textures/hexagon.png");
#endif

    //setting up texture packs
    vector<GLuint> texturePackColor = { redTextureID, yellowTextureID, greenTextureID, whiteTextureID, blueTextureID, orangeTextureID };
    vector<GLuint> texturePackShape = { squareID, circleID, rhombusID, triangleID, pentagonID, hexagonID };
    vector<GLuint> texturePackAnimals = { zebraCubeID, lionCubeID, giraffeCubeID, leopardCubeID, snakeCubeID, whiteLeopardCubeID };
    vector<GLuint> texturePackMovies = { backIntoTheFutureCubeID, elfCubeID, harryPotterCubeID, homeAloneCubeID, spaceJamCubeID, lordOfTheRingCubeID };
    vector<GLuint> texturePackGaming = { marioCubeID, nessCubeID, pikachuCubeID, sonicCubeID, pacmanCubeID, zeldaCubeID };

    float lightAngleOuter = radians(30.0f);
    float lightAngleInner = radians(20.0f);

    // Set light cutoff angles on scene shader
    AffectedByLightingShader.use();
    AffectedByLightingShader.setFloat("light_cutoff_inner", cos(lightAngleInner));
    AffectedByLightingShader.setFloat("light_cutoff_outer", cos(lightAngleOuter));


    // For frame time
    float lastFrameTime = glfwGetTime();
    int lastMouseLeftState = GLFW_RELEASE;
    double lastMousePosX, lastMousePosY;
    glfwGetCursorPos(window, &lastMousePosX, &lastMousePosY);

    //enable OpenGL components
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    int width, height;
    float scaleFactor = 0.0f;

    int choose = 1; //choosing the model

    //choosing specific cameras
    vec3 storedCameraPosition = vec3(1.0f);
    vec2 storedCameraAngle = vec2(1.0f);
    int chooseCamera = 0;
    float bulletTimeAngle = 0.0f;

    int skyboxChoose = 0;

    string timer = "";
    double time = 0.0;
    int TimeUpdate = 1;
    double seconds = 0.0;
    double newseconds = 0.0;
    bool timeUp = false;
    double pausedSeconds = 0.0;

    ShadowShader.use();
    //creating all cube objects
    Rubiks_Cube normalCube = Rubiks_Cube(vec3(0.0f), texturePackColor);
    normalCube.generateCube(ShadowShader);

    Rubiks_Cube shapeCube = Rubiks_Cube(vec3(15.0f, 0.0f, 0.0f), texturePackShape);
    shapeCube.generateCube(ShadowShader);

    Rubiks_Cube animalCube = Rubiks_Cube(vec3(-15.0f, 0.0f, 0.0f), texturePackAnimals);
    animalCube.generateCube(ShadowShader);

    Rubiks_Cube gameCube = Rubiks_Cube(vec3(0.0f, 0.0f, -15.0f), texturePackGaming);
    gameCube.generateCube(ShadowShader);

    Rubiks_Cube movieCube = Rubiks_Cube(vec3(0.0f, 0.0f, 15.0f), texturePackMovies);
    movieCube.generateCube(ShadowShader);

    //setting up for button debouncing
    int oldStateQ = GLFW_RELEASE;
    int oldStateE = GLFW_RELEASE;
    int oldStateZ = GLFW_RELEASE;
    int oldStateC = GLFW_RELEASE;
    int oldStateR = GLFW_RELEASE;
    int oldStateV = GLFW_RELEASE;


    int oldStateUp = GLFW_RELEASE;
    int oldStateDown = GLFW_RELEASE;
    int oldStateLeft = GLFW_RELEASE;
    int oldStateRight = GLFW_RELEASE;
    int oldStateU = GLFW_RELEASE;
    int oldStateJ = GLFW_RELEASE;

    int oldStateF1 = GLFW_RELEASE;
    int oldStateF2 = GLFW_RELEASE;

    int oldState1 = GLFW_RELEASE;
    int oldState2 = GLFW_RELEASE;
    int oldState3 = GLFW_RELEASE;
    int oldState4 = GLFW_RELEASE;


    //for flashlight
    int lightOldState = GLFW_RELEASE;
    bool enableLight = true;

    Rubiks_Cube* selectedCube = &normalCube;

    // Entering Game Loop
    while (!glfwWindowShouldClose(window))
    {
        bool debugTick = false;

        NotAffectedByLightingShader.use();
        NotAffectedByLightingShader.setInt("currentAxis", 0);

        //changing projection view every second to accomodate for zoom
        mat4 projectionMatrix = perspective(FOV, 1024.0f / 768.0f, 0.01f, 1000.0f);

        AffectedByLightingShader.use();
        AffectedByLightingShader.setMat4("projectionMatrix", projectionMatrix);
        AffectedByLightingShader.setMat4("viewMatrix", viewMatrix);

        NotAffectedByLightingShader.use();
        NotAffectedByLightingShader.setMat4("projectionMatrix", projectionMatrix);

        mat4 projection = ortho(0.0f, static_cast<float>(1024), 0.0f, static_cast<float>(768));
        TextShader.use();
        TextShader.setMat4("projection", projection);

        // Frame time calculation
        float dt = glfwGetTime() - lastFrameTime;
        lastFrameTime += dt;

        // Each frame, reset buffers
        glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_DEPTH_BUFFER_BIT);

        //Rendering Text
        double realTime = glfwGetTime();
        const float maxTime = 300.0f;

        if (TimeUpdate == 0)
        {
            seconds = pausedSeconds + realTime;
            time = maxTime - seconds;

            if (time > 0.0) {
                timer = to_string(time);
            }
            else if (time <= 0.0 && timeUp == false) {//this is when you lose 
                seconds = 0.0;
                timer = "0.0";
                engine->play2D("lost.mp3", false);
                timeUp = true;

                gameState = 2;
            }
        }
        else if (TimeUpdate == 1)
        {
            pausedSeconds = seconds;
            time = maxTime - seconds;
            timer = to_string(time);
        }

        //modules for controlling model and world behaviour =================================================================================================================

        //number key: select which cube we're acting on =======================================================================================

        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && TimeUpdate == 1) {
            selectedCube = &normalCube;
        }

        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && TimeUpdate == 1) {
            selectedCube = &shapeCube;
        }

        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && TimeUpdate == 1) {
            selectedCube = &animalCube;
		}

        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS && TimeUpdate == 1) {
            selectedCube = &gameCube;
		}

        if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS && TimeUpdate == 1) {
            selectedCube = &movieCube;
		}

        //manipulating cube ==================================================================================================
        //Q = rotate left
        //E = rotate right
        //Z = tilt left
        //C = tilt right
        //R = rotate up
        //V = rotate down
        if (gameState == 0 || gameState == 3) {
            int newStateQ = glfwGetKey(window, GLFW_KEY_F);
            if (newStateQ == GLFW_RELEASE && oldStateQ == GLFW_PRESS) {
                selectedCube->rotate_y("CCW", selectCord.y);
                debugTick = true;
                engine->play2D("rotateRubiksCube.mp3", false);
            }
            oldStateQ = newStateQ;

            int newStateE = glfwGetKey(window, GLFW_KEY_H);
            if (newStateE == GLFW_RELEASE && oldStateE == GLFW_PRESS) {
                selectedCube->rotate_y("CW", selectCord.y);
                debugTick = true;
                engine->play2D("rotateRubiksCube.mp3", false);
            }
            oldStateE = newStateE;

            int newStateZ = glfwGetKey(window, GLFW_KEY_T);
            if (newStateZ == GLFW_RELEASE && oldStateZ == GLFW_PRESS) {
                selectedCube->rotate_z("CCW", selectCord.z);
                debugTick = true;
                engine->play2D("rotateRubiksCube.mp3", false);
            }
            oldStateZ = newStateZ;

            int newStateC = glfwGetKey(window, GLFW_KEY_G);
            if (newStateC == GLFW_RELEASE && oldStateC == GLFW_PRESS) {
                selectedCube->rotate_z("CW", selectCord.z);
                debugTick = true;
                engine->play2D("rotateRubiksCube.mp3", false);
            }
            oldStateC = newStateC;

            int newStateR = glfwGetKey(window, GLFW_KEY_R);
            if (newStateR == GLFW_RELEASE && oldStateR == GLFW_PRESS) {
                selectedCube->rotate_x("CCW", selectCord.x);
                debugTick = true;
                engine->play2D("rotateRubiksCube.mp3", false);
            }
            oldStateR = newStateR;

            int newStateV = glfwGetKey(window, GLFW_KEY_Y);
            if (newStateV == GLFW_RELEASE && oldStateV == GLFW_PRESS) {
                selectedCube->rotate_x("CW", selectCord.x);
                debugTick = true;
                engine->play2D("rotateRubiksCube.mp3", false);
            }
            oldStateV = newStateV;
        }
        

        //moving selection block: up/down/left/right/u/j =========================================================================

        int newStateUp = glfwGetKey(window, GLFW_KEY_UP);
        if (newStateUp == GLFW_RELEASE && oldStateUp == GLFW_PRESS) {
            if (selectCord.z != 0) {
                selectCord.z -= 1;
                engine->play2D("selectNewBlock.mp3", false);

                if (selectCord == vec3(1.0f)) {
                    selectCord.z -= 1;
                    engine->play2D("selectNewBlock.mp3", false);
                }
            }
        }
        oldStateUp = newStateUp;

        int lightNewState = glfwGetKey(window, GLFW_KEY_TAB);
        if (lightNewState == GLFW_RELEASE && lightOldState == GLFW_PRESS) {
            if (enableLight == true) {
                enableLight = false;
            }
            else {
                enableLight = true;
            }
        }
        lightOldState = lightNewState;

        int newStateDown = glfwGetKey(window, GLFW_KEY_DOWN);
        if (newStateDown == GLFW_RELEASE && oldStateDown == GLFW_PRESS) {
            if (selectCord.z != 2) {
                selectCord.z += 1;
                engine->play2D("selectNewBlock.mp3", false);

                if (selectCord == vec3(1.0f)) {
                    selectCord.z += 1;
                    engine->play2D("selectNewBlock.mp3", false);
                }
            }
        }
        oldStateDown = newStateDown;

        int newStateLeft = glfwGetKey(window, GLFW_KEY_LEFT);
        if (newStateLeft == GLFW_RELEASE && oldStateLeft == GLFW_PRESS) {
            if (selectCord.x != 0) {
                selectCord.x -= 1;
                engine->play2D("selectNewBlock.mp3", false);

                if (selectCord == vec3(1.0f)) {
                    selectCord.x -= 1;
                    engine->play2D("selectNewBlock.mp3", false);
                }
            }
        }
        oldStateLeft = newStateLeft;

        int newStateRight = glfwGetKey(window, GLFW_KEY_RIGHT);
        if (newStateRight == GLFW_RELEASE && oldStateRight == GLFW_PRESS) {
            if (selectCord.x != 2) {
                selectCord.x += 1;
                engine->play2D("selectNewBlock.mp3", false);

                if (selectCord == vec3(1.0f)) {
                    selectCord.x += 1;
                    engine->play2D("selectNewBlock.mp3", false);
                }
            }
        }
        oldStateRight = newStateRight;

        int newStateU = glfwGetKey(window, GLFW_KEY_U);
        if (newStateU == GLFW_RELEASE && oldStateU == GLFW_PRESS) {
            if (selectCord.y != 2) {
                selectCord.y += 1;
                engine->play2D("selectNewBlock.mp3", false);

                if (selectCord == vec3(1.0f)) {
                    selectCord.y += 1;
                    engine->play2D("selectNewBlock.mp3", false);
                }
            }
        }
        oldStateU = newStateU;

        int newStateJ = glfwGetKey(window, GLFW_KEY_J);
        if (newStateJ == GLFW_RELEASE && oldStateJ == GLFW_PRESS) {
            if (selectCord.y != 0) {
                selectCord.y -= 1;
                engine->play2D("selectNewBlock.mp3", false);

                if (selectCord == vec3(1.0f)) {
                    selectCord.y -= 1;
                    engine->play2D("selectNewBlock.mp3", false);
                }
            }
        }
        oldStateJ = newStateJ;


        //F1: Reset Cube =================================================================================================
        int newStateF1 = glfwGetKey(window, GLFW_KEY_F1);
        if (newStateF1 == GLFW_RELEASE && oldStateF1 == GLFW_PRESS) {
            selectedCube->resetPosition();
            gameState = 0;
        }
        oldStateF1 = newStateF1;

        //F2: randomly shuffle the cube
        int newStateF2 = glfwGetKey(window, GLFW_KEY_F2);
        if (newStateF2 == GLFW_RELEASE && oldStateF2 == GLFW_PRESS) {
            selectedCube->randomizePosition(25);
            engine->play2D("rotateRubiksCube.mp3", false);
            gameState = 3;
        }
        oldStateF2 = newStateF2;

        //TIMER CONTROL ==========================================================================================================================
       if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS) // pause timer
        {
            TimeUpdate = 0;
            glfwSetTime(0.0);
        }

        if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS) // reset timer
        {
            glfwSetTime(0.0);
            pausedSeconds = 0.0;
            seconds = 0.0;
            TimeUpdate = 1;
        }

        //testing for game state 
        if (gameState == 3) {//if the game is ongoing
            selectedCube->testforWinCondition();

            if (selectedCube->getWin() == true) {
                gameState = 1;//set game state to won
                TimeUpdate = 1;//stop timer
                engine->play2D("win.mp3", false);

            }
        }

        

        //vec3 lightDirection = normalize(lightFocus - lightPos);

        //lighting setup

        float lightNearPlane = 0.01f;
        float lightFarPlane = 400.0f;
        vec3 lightDirection = cameraLookAt;

        //frustum(-1.0f, 1.0f, -1.0f, 1.0f, lightNearPlane, lightFarPlane);
        //mat4 lightProjMatrix = perspective(50.0f, (float)DEPTH_MAP_TEXTURE_SIZE / (float)DEPTH_MAP_TEXTURE_SIZE, lightNearPlane, lightFarPlane);
        mat4 lightProjMatrix = ortho(-50.0f, 50.0f, -50.0f, 50.0f, lightNearPlane, lightFarPlane);
        mat4 lightViewMatrix = lookAt(lightPos, lightFocus, vec3(0, 1, 0));

        AffectedByLightingShader.use();
        if (enableLight == true) {
            AffectedByLightingShader.setMat4("light_proj_view_matrix", lightProjMatrix * lightViewMatrix);
        }
        else {
            AffectedByLightingShader.setMat4("light_proj_view_matrix", mat4(0.0f));
        }
        AffectedByLightingShader.setFloat("light_near_plane", lightNearPlane);
        AffectedByLightingShader.setFloat("light_far_plane", lightFarPlane);
        AffectedByLightingShader.setVec3("objectColor", vec3(1));
        AffectedByLightingShader.setVec3("lightColor", vec3(1));
        AffectedByLightingShader.setVec3("lightPos", lightPos);
        AffectedByLightingShader.setVec3("light_direction", lightDirection);
        AffectedByLightingShader.setVec3("viewPos", cameraPosition);


        //////////////////////////////////////////////
        //FIRST PASS/////////////////////////////////
        //////////////////////////////////////////////
        ShadowShader.use();
        // Use proper image output size
        glViewport(0, 0, DEPTH_MAP_TEXTURE_SIZE, DEPTH_MAP_TEXTURE_SIZE);
        // Bind depth map texture as output framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, depth_map_fbo);
        // Clear depth data on the framebuffer
        glClear(GL_DEPTH_BUFFER_BIT);
        // Bind geometry

        if (gameState != 0) {
            RenderText(TextShader, "Timer:", 25.0f, 25.0f, 1.0f, vec3(0.5, 0.8f, 0.2f));
            RenderText(TextShader, timer, 200.0f, 25.0f, 1.0f, vec3(0.5, 0.8f, 0.2f));
        }

        RenderText(TextShader, "Use arrow key and U/J to move the cube of selection, press F/H/T/G/R/Y to rotate it", 25.0, 75.0, 0.40f, vec3(0.5, 0.8f, 0.2f));

        if (gameState == 2) {
            RenderText(TextShader, "You lost! Press F1 to reset and F2 to start a new game. ", 35.0f, 768 / 2, 0.65f, vec3(0.5, 0.8f, 0.2f));
        }
        else if (gameState == 1) {
            RenderText(TextShader, "You won! Press F1 to reset and F2 to start a new game. ", 25.0f, 768 / 2, 0.65f, vec3(0.5, 0.8f, 0.2f));
        }

        if (gameState == 0) {
            RenderText(TextShader, "Use number key 1-5 to select a Cube.", 25.0, 130.0, 0.5f, vec3(0.5, 0.8f, 0.2f));
            RenderText(TextShader, "Press F2 to start a game on your selected cube. ", 25.0, 100.0, 0.5f, vec3(0.5, 0.8f, 0.2f));
        }



        glBindVertexArray(cubeVAO);
        ShadowShader.use();

        normalCube.setShader(ShadowShader);
        normalCube.drawModel();

        shapeCube.setShader(ShadowShader);
        shapeCube.drawModel();

        animalCube.setShader(ShadowShader);
        animalCube.drawModel();

        gameCube.setShader(ShadowShader);
        gameCube.drawModel();

        movieCube.setShader(ShadowShader);
        movieCube.drawModel();

        //drawing selection cube
        glBindTexture(GL_TEXTURE_2D, selectCubeID);
        Cube selectCube = Cube(selectCord + selectedCube->getPosition(), ShadowShader);
        selectCube.setDefaultSize(vec3(1.01f));
        selectCube.drawModel();

        glBindVertexArray(cubeVAO);
        //drawing skybox
        glBindTexture(GL_TEXTURE_2D, skyboxTextureID);
        mat4 skybox = translate(mat4(1.0f), cameraPosition) * scale(mat4(1.0f), vec3(-500.0f));
        ShadowShader.setMat4("worldMatrix", skybox);
        glDrawArrays(GL_TRIANGLES, 0, 36); // 36 vertices, starting at index 0

        //drawing rubiks cube selector triangle 
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(triangleVAO);
        NotAffectedByLightingShader.use();

        mat4 cubeMarker = translate(mat4(1.0f), selectedCube->getPosition() + vec3(1.0f, 5.0f, 1.0f)) * rotate(mat4(1.0f), (float)(glfwGetTime()), vec3(0.0f, 1.0f, 0.0f)) * rotate(mat4(1.0f), radians(180.0f), vec3(1.0f, 0.0f, 0.0f)) * scale(mat4(1.0f), vec3(1.0f));
        NotAffectedByLightingShader.setMat4("worldMatrix", cubeMarker);
        NotAffectedByLightingShader.setInt("currentAxis", 1);

        glDrawArrays(GL_TRIANGLES, 0, 6); // 36 vertices, starting at index 0

        glBindVertexArray(0);

        //glBindTexture(GL_TEXTURE_2D, 0);

        //////////////////////////////////
        ///////////SECOND PASS////////////
        //////////////////////////////////

        AffectedByLightingShader.use();
        // Use proper image output size
        // Side note: we get the size from the framebuffer instead of using WIDTH
        // and HEIGHT because of a bug with highDPI displays
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        // Bind screen as output framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // Clear color and depth data on framebuffer
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Bind depth map texture: not needed, by default it is active
        glActiveTexture(GL_TEXTURE0);
        // Activate any texture unit you use for your models
        // Bind geometry

        if (gameState != 0) {
            RenderText(TextShader, "Timer:", 25.0f, 25.0f, 1.0f, vec3(0.5, 0.8f, 0.2f));
            RenderText(TextShader, timer, 200.0f, 25.0f, 1.0f, vec3(0.5, 0.8f, 0.2f));
        }

        RenderText(TextShader, "Use arrow key and U/J to move the cube of selection, press F/H/T/G/R/Y to rotate it", 25.0, 75.0, 0.40f, vec3(0.5, 0.8f, 0.2f));

        if (gameState == 2) {
            RenderText(TextShader, "You lost! Press F1 to reset and F2 to start a new game. ", 35.0f, 768 / 2, 0.65f, vec3(0.5, 0.8f, 0.2f));
        }
        else if (gameState == 1) {
            RenderText(TextShader, "You won! Press F1 to reset and F2 to start a new game. ", 25.0f, 768 / 2, 0.65f, vec3(0.5, 0.8f, 0.2f));
        }

        if (gameState == 0) {
            RenderText(TextShader, "Use number key 1-5 to select a Cube.", 25.0, 130.0, 0.5f, vec3(0.5, 0.8f, 0.2f));
            RenderText(TextShader, "Press F2 to start a game on your selected cube. ", 25.0, 100.0, 0.5f, vec3(0.5, 0.8f, 0.2f));
        }



        glBindVertexArray(cubeVAO);
        AffectedByLightingShader.use();

        normalCube.setShader(AffectedByLightingShader);
        normalCube.drawModel();

        shapeCube.setShader(AffectedByLightingShader);
        shapeCube.drawModel();

        animalCube.setShader(AffectedByLightingShader);
        animalCube.drawModel();

        gameCube.setShader(AffectedByLightingShader);
        gameCube.drawModel();

        movieCube.setShader(AffectedByLightingShader);
        movieCube.drawModel();

        //drawing selection cube
        glBindTexture(GL_TEXTURE_2D, selectCubeID);
        selectCube.setCurrentShader(AffectedByLightingShader);
        selectCube.drawModel();

        //drawing skybox
        glBindVertexArray(cubeVAO);
        glBindTexture(GL_TEXTURE_2D, skyboxTextureID);
        AffectedByLightingShader.setMat4("worldMatrix", skybox);
        glDrawArrays(GL_TRIANGLES, 0, 36); // 36 vertices, starting at index 0

        //drawing cube selector
        glBindTexture(GL_TEXTURE_2D, 0);

        glBindVertexArray(triangleVAO);
        NotAffectedByLightingShader.use();
        NotAffectedByLightingShader.setMat4("worldMatrix", cubeMarker);
        NotAffectedByLightingShader.setInt("currentAxis", 1);

        glDrawArrays(GL_TRIANGLES, 0, 6); // 36 vertices, starting at index 0

        glBindVertexArray(0);

        // End Frame
        glfwSwapBuffers(window);
        glfwPollEvents();

        // Handle inputs
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        double mousePosX, mousePosY;
        glfwGetCursorPos(window, &mousePosX, &mousePosY);

        double dx = mousePosX - lastMousePosX;
        double dy = mousePosY - lastMousePosY;

        lastMousePosX = mousePosX;
        lastMousePosY = mousePosY;

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            //turn mouse into zoom mode
            FOV += dy / 100;
            if (FOV > 71) {
                FOV = 71;
            }
            else if (FOV < 69.2) {
                FOV = 69.2;
            }
        }
        else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
            cameraTiltAngle += dx / 100;
            cout << cameraTiltAngle << endl;
        }
        else {
            //mouse in normal fps mode
            // Convert to spherical coordinate
            cameraHorizontalAngle -= dx * mouseSensitivity * dt;
            cameraVerticalAngle -= dy * mouseSensitivity * dt;

            //limit the vertical camera angel
            if (cameraVerticalAngle > 85.0f) {
                cameraVerticalAngle = 85.0f;
            }
            else if (cameraVerticalAngle < -85.0f) {
                cameraVerticalAngle = -85.0f;
            }

            if (cameraHorizontalAngle > 360)
            {
                cameraHorizontalAngle -= 360;
            }
            else if (cameraHorizontalAngle < -360)
            {
                cameraHorizontalAngle += 360;
            }

            float theta = radians(cameraHorizontalAngle);
            float phi = radians(cameraVerticalAngle);

            cameraLookAt = vec3(cosf(phi) * cosf(theta), sinf(phi), -cosf(phi) * sinf(theta));
            vec3 cameraSideVector = glm::cross(cameraLookAt, vec3(0.0f, 1.0f, 0.0f));

            glm::normalize(cameraSideVector);
        }

        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) // increase camera movement speed
        {
            cameraSpeed = fastCameraSpeed;
        }
        else {
            cameraSpeed = normalCameraSpeed;
        }

        //using WASD to control movement adjusted
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) // move camera to the left
        {
            cameraPosition.z -= cameraSpeed * dt * cameraLookAt.x;
            cameraPosition.x += cameraSpeed * dt * cameraLookAt.z;
        }

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) // move camera to the right
        {
            cameraPosition.z += cameraSpeed * dt * cameraLookAt.x;
            cameraPosition.x -= cameraSpeed * dt * cameraLookAt.z;
        }

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) // move camera forward
        {
            cameraPosition.x -= cameraSpeed * dt * cameraLookAt.x;
            cameraPosition.z -= cameraSpeed * dt * cameraLookAt.z;
        }

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) // move camera backwards
        {
            cameraPosition.x += cameraSpeed * dt * cameraLookAt.x;
            cameraPosition.z += cameraSpeed * dt * cameraLookAt.z;
        }

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) // move camera to the right
        {
            cameraPosition.y += cameraSpeed * dt;
        }

        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) // move camera to the right
        {
            cameraPosition.y -= cameraSpeed * dt;
        }

        viewMatrix = lookAt(cameraPosition, cameraPosition + cameraLookAt, cameraUp);

        AffectedByLightingShader.use();
        AffectedByLightingShader.setMat4("viewMatrix", viewMatrix);
        NotAffectedByLightingShader.use();
        NotAffectedByLightingShader.setMat4("viewMatrix", viewMatrix);

        lightPos = cameraPosition;

       
    }

    // Shutdown GLFW
    glfwTerminate();

    return 0;
}

bool initContext() {     // Initialize GLFW and OpenGL version
    glfwInit();

#if defined(PLATFORM_OSX)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    // On windows, we set OpenGL version to 2.1, to support more hardware
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#endif

    // Create Window and rendering context using GLFW, resolution is 1024x768
    window = glfwCreateWindow(1024, 768, "COMP 371 - PROJECT 2A", NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to create GLEW" << std::endl;
        glfwTerminate();
        return false;
    }
    return true;
}


GLuint loadTexture(const char* filename)
{
    // Step1 Create and bind textures
    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    assert(textureId != 0);


    glBindTexture(GL_TEXTURE_2D, textureId);

    // Step2 Set filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Step3 Load Textures with dimension data
    int width, height, nrChannels;
    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (!data)
    {
        std::cerr << "Error::Texture could not load texture file:" << filename << std::endl;
        return 0;
    }

    // Step4 Upload the texture to the PU
    GLenum format = 0;
    if (nrChannels == 1)
        format = GL_RED;
    else if (nrChannels == 3)
        format = GL_RGB;
    else if (nrChannels == 4)
        format = GL_RGBA;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height,
        0, format, GL_UNSIGNED_BYTE, data);

    // Step5 Free resources
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return textureId;
}

//randomly generate character position
float RNGpos() {
    return (rand() % 101 - 50);
}

//using cross product to find the normal vector to a surface
vec3 crossProduct(vec3 point1, vec3 point2, vec3 point3) {
    vec3 vector1 = point2 - point1;
    vec3 vector2 = point3 - point1;
    return cross(vector1, vector2);
}


vector<vec3> generateSphere(float radius, int polyCount) {
    float sectorCount = polyCount;
    float stackCount = polyCount;
    vector<vec3> vertices;
    vector<vec3> result;

    float x, y, z, xy;                              // vertex position
    float nx, ny, nz, lengthInv = 1.0f / radius;    // vertex normal
    float s, t;                                     // vertex texCoord

    float sectorStep = 2 * M_PI / sectorCount;
    float stackStep = M_PI / stackCount;
    float sectorAngle, stackAngle;

    for (int i = 0; i <= stackCount; ++i)
    {
        stackAngle = M_PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2
        xy = radius * cosf(stackAngle);             // r * cos(u)
        z = radius * sinf(stackAngle);              // r * sin(u)

        // add (sectorCount+1) vertices per stack
        // the first and last vertices have same position and normal, but different tex coords
        for (int j = 0; j <= sectorCount; ++j)
        {
            sectorAngle = j * sectorStep;           // starting from 0 to 2pi

            // vertex position (x, y, z)
            x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
            y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
            vertices.push_back(vec3(x, y, z));
        }
    }

    int k1, k2;
    for (int i = 0; i < stackCount; ++i)
    {
        k1 = i * (sectorCount + 1);     // beginning of current stack
        k2 = k1 + sectorCount + 1;      // beginning of next stack

        for (int j = 0; j < sectorCount; ++j, ++k1, ++k2)
        {
            // 2 triangles per sector excluding first and last stacks
            // k1 => k2 => k1+1
            if (i != 0)
            {
                result.push_back(vertices[k1]);
                result.push_back(vec3(0.3, 0.3, 0.3));
                result.push_back(vec3(0.0, 1.0, 0.0));
                result.push_back(vertices[k2]);
                result.push_back(vec3(0.3, 0.3, 0.3));
                result.push_back(vec3(0.0, 1.0, 0.0));
                result.push_back(vertices[k1 + 1]);
                result.push_back(vec3(0.3, 0.3, 0.3));
                result.push_back(vec3(0.0, 1.0, 0.0));
            }

            // k1+1 => k2 => k2+1
            if (i != (stackCount - 1))
            {
                result.push_back(vertices[k1 + 1]);
                result.push_back(vec3(0.3, 0.3, 0.3));
                result.push_back(vec3(0.0, 1.0, 0.0));
                result.push_back(vertices[k2]);
                result.push_back(vec3(0.3, 0.3, 0.3));
                result.push_back(vec3(0.0, 1.0, 0.0));
                result.push_back(vertices[k2 + 1]);
                result.push_back(vec3(0.3, 0.3, 0.3));
                result.push_back(vec3(0.0, 1.0, 0.0));
            }
        }
    }
    return result;
}

void RenderText(Shader& s, string text, float x, float y, float scale, vec3 color)
{
    // activate corresponding render state
    s.use();
    glUniform3f(glGetUniformLocation(s.ID, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(charVAO);

    // iterate through all characters
    string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, charVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
