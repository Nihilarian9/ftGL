/*
    A orthographic rendering of text using freetype and OpenGL
    created for arch linux but should be easily convertable to other operating systems
    Copyright (C) 2018 Peter Martin
    GPL v2.0
*/

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"

#include <iostream>
#include <vector>
#include <map>
#include <stdio.h>
#include <assert.h>
#include <string>
#include <chrono>
#include <algorithm>
#include <array>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_IMAGE_H

#define FT_CHECK(r) { FT_Error err = (r); assert(!err); } while (0)
#define NUMBER_OF_GLYPHS 128
#define GLYPH_START 32

const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1050;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

double mousePosX, mousePosY, mouseRelX, mouseRelY;
float lastMouseX = 0.0f;
float lastMouseY = 0.0f;
float xTranslate = 0.0f;
float yTranslate = 0.0f;
float scroll_float = 0.0f;
bool mouseClicked = false;

float deltaTime;
float lastTime = 0.0f;

GLubyte GLYPH_NUMBER = GLYPH_START;

// struct holds values for font
struct FP_Glyph_Prop {
    glm::vec2 advance;
    glm::vec2 dimensions;
    glm::vec2 bearing;
    float texOffset;
};

class FontData {

public:
    void run() {
        GL_Program();
    }

private:
    GLFWwindow* window;
    std::map<GLchar, FP_Glyph_Prop> glyphs;
    //use vertices for orthographic projection
    std::vector<float> vertices;
    //use normVerts for -1 to 1 values
    std::vector<float> normVerts;

    GLuint VBO, VAO;
    GLuint atlas_texture;
    int frameSeconds = 0, frameCount = 0;
    int atlas_width, atlas_height;
    float smallestX, smallestY, biggestX, biggestY;

    void GL_Program() {

        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "window", NULL, NULL);
        if (window == NULL)
        {
            std::cout << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
        }
        glfwMakeContextCurrent(window);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cout << "Failed to initialize GLAD" << std::endl;
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        Shader shader("shaders/shader.vert", "shaders/shader.frag");
        //glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(SCR_WIDTH), 0.0f, static_cast<GLfloat>(SCR_HEIGHT));
        glm::mat4 projection = glm::mat4(1.0f);
        shader.use();
        shader.setMat4("projection", projection);

        glm::mat4 model;

        //////////////////////////////////////
        //  don't forget to create font atlas
        //  and create text vertex array
        createFont();
        float textPositionY = static_cast<float>(SCR_HEIGHT) - 100.0f;

        //createVertexArray(posX, posY, scaleX, scaleY);
        createVertexArray(50.0f, -textPositionY, 1.0f, 1.0f);


        for (int i = 0; i < vertices.size(); i+=4) {
            normVerts.push_back(normaliseFloat(vertices[i], smallestX, biggestX));
            normVerts.push_back(normaliseFloat(vertices[i+1], smallestY, biggestY) * 2 - 1.0f);
            normVerts.push_back(vertices[i+2]);
            normVerts.push_back(vertices[i+3]);
        }

        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, normVerts.size() * sizeof(normVerts[0]), &normVerts[0], GL_STATIC_DRAW);

        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        while (!glfwWindowShouldClose(window))
        {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            static auto startTime = std::chrono::high_resolution_clock::now();
            auto currentTime = std::chrono::high_resolution_clock::now();
            float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
            deltaTime = time - lastTime;
            lastTime = time;

            if(time > frameSeconds)
            {
                frameSeconds++;
                std::cout << "Frames: " << frameCount << std::endl;
                frameCount = 0;
            } else {
                frameCount++;
            }

            processInput();

            xTranslate += normaliseFloat((float)mousePosX - (float)lastMouseX, 0.0f, (float)SCR_WIDTH) + 1.0f;
            yTranslate -= normaliseFloat((float)mousePosY - (float)lastMouseY, 0.0f, (float)SCR_HEIGHT) + 1.0f;
            lastMouseX = mousePosX;
            lastMouseY = mousePosY;

            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(xTranslate, yTranslate, 0.0f));
            shader.setMat4("model", model);

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, normVerts.size() / 4);
            glBindVertexArray(0);

            glfwSwapInterval(0);
            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glfwTerminate();
    }




    void createVertexArray(float posX, float posY, float scaleX, float scaleY) {

        // text to be printed to screen
        std::vector<std::string> sonnet = {
            "When I have seen by Time's fell hand defaced",
            "The rich proud cost of outworn buried age;",
            "When sometime lofty towers I see down-razed,",
            "And brass eternal slave to mortal rage;",
            "When I have seen the hungry ocean gain",
            "Advantage on the kingdom of the shore,",
            "And the firm soil win of the watery main,",
            "Increasing store with loss, and loss with store;",
            "When I have seen such interchange of state,",
            "Or state itself confounded to decay;",
            "Ruin hath taught me thus to ruminate",
            "That Time will come and take my love away.",
            "    This thought is as a death which cannot choose",
            "    But weep to have that which it fears to lose."
         };

        FP_Glyph_Prop gl;
        float xPos, yPos, height, width, texWidth, texHeight;
        float xOrigin = posX;
        bool firstLetter = true;

        for(int y = 0; y < sonnet.size(); y++) {
            posX = xOrigin;
            for (int x = 0; x < sonnet[y].length(); x++) {
                gl = glyphs[sonnet[y].at(x)];
                xPos = posX + gl.bearing.x * scaleX;
                yPos = posY - gl.bearing.y * scaleY;
                width = gl.dimensions.x * scaleX;
                height = gl.dimensions.y * scaleY;
                texWidth = gl.dimensions.x / atlas_width;
                texHeight = gl.dimensions.y / atlas_height;

                //  triangle 1
                vertices.push_back(xPos);                       vertices.push_back(-yPos);
                vertices.push_back(gl.texOffset);               vertices.push_back(0.0f);
                vertices.push_back(xPos + width);               vertices.push_back(-yPos);
                vertices.push_back(gl.texOffset + texWidth);    vertices.push_back(0.0f);
                vertices.push_back(xPos);                       vertices.push_back(-yPos - height);
                vertices.push_back(gl.texOffset);               vertices.push_back(texHeight);

                //  triangle 2
                vertices.push_back(xPos + width);               vertices.push_back(-yPos);
                vertices.push_back(gl.texOffset + texWidth);    vertices.push_back(0.0f);
                vertices.push_back(xPos);                       vertices.push_back(-yPos - height);
                vertices.push_back(gl.texOffset);               vertices.push_back(texHeight);
                vertices.push_back(xPos + width);               vertices.push_back(-yPos - height);
                vertices.push_back(gl.texOffset + texWidth);    vertices.push_back(texHeight);

                if(firstLetter) {
                    smallestX = xPos; biggestX = xPos; smallestY = yPos; biggestY = yPos;
                    firstLetter = false;
                } else {
                    smallestX = std::min(smallestX, std::min(xPos, xPos + width));
                    biggestX = std::max(biggestX, std::max(xPos, xPos + width));
                    smallestY = std::min(smallestY, std::min(-yPos, -yPos - height));
                    biggestY = std::max(biggestY, std::max(-yPos, -yPos - height));
                }


                posX += gl.advance.x * scaleX;
            }
            posY += gl.advance.y * scaleY;
        }
        glBindVertexArray(0);
    }




    void createFont() {

        FP_Glyph_Prop glyphTemp;
        FT_Library  library;
        FT_Face     face;

        FT_CHECK(FT_Init_FreeType(&library));
        FT_CHECK(FT_New_Face(library, "/usr/share/fonts/TTF/Hack-Regular.ttf", 0, &face));
        FT_CHECK(FT_Set_Pixel_Sizes(face, 0, 32));
        FT_GlyphSlot g = face->glyph;

        int w = 0, h = 0, x = 0;

        for (GLubyte c = GLYPH_START; c < NUMBER_OF_GLYPHS; c++)
        {
            FT_CHECK(FT_Load_Char(face, c, FT_LOAD_RENDER));
            g = face->glyph;
            w += g->bitmap.width;
            h = std::max(h, (int)g->bitmap.rows);
        }
        atlas_width = w;
        atlas_height = h;

        glActiveTexture(GL_TEXTURE0);
        glGenTextures(1, &atlas_texture);
        glBindTexture(GL_TEXTURE_2D, atlas_texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        for (GLubyte c = GLYPH_START; c < NUMBER_OF_GLYPHS; c++)
        {
            FT_CHECK(FT_Load_Char(face, c, FT_LOAD_RENDER));
            g = face->glyph;

            glTexSubImage2D(GL_TEXTURE_2D, 0, x, 0, g->bitmap.width, g->bitmap.rows, GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer);
            glyphTemp.texOffset = (float)x / w;
            x += g->bitmap.width;

            glyphTemp.advance.x = g->advance.x >> 6;
            glyphTemp.advance.y = g->metrics.vertAdvance >> 6;

            glyphTemp.dimensions.x = g->bitmap.width;
            glyphTemp.dimensions.y = g->bitmap.rows;

            glyphTemp.bearing.x = g->bitmap_left;
            glyphTemp.bearing.y = g->bitmap_top;

            glyphs.insert(std::pair<GLchar, FP_Glyph_Prop>(c, glyphTemp));

        }

        FT_Done_Face(face);
        FT_Done_FreeType(library);
    }

    void processInput()
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        glfwSetCursorPosCallback(window, mouse_callback);
        glfwSetScrollCallback(window, scroll_callback);
    }

    //normalise values between -1 and 1
    float normaliseFloat(float num, float min, float max)
    {
        return 2 * ((num - min) / (max - min)) - 1;
    }
};

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {

    int left_mouse_button_state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if (left_mouse_button_state == GLFW_PRESS) {
        if(!mouseClicked) {
            lastMouseX = xpos;
            lastMouseY = ypos;
            mouseClicked = true;
        }
        mousePosX = xpos;
        mousePosY = ypos;
    }
    else if (left_mouse_button_state == GLFW_RELEASE) {
        mouseRelX = xpos;
        mouseRelY = ypos;
        mouseClicked = false;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    scroll_float += yoffset / 10;
    //std::cout << "Scroll Offset: " << xoffset << ", " << yoffset << std::endl;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
