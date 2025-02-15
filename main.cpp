/**
* Author: Connor Chavez
* Assignment: Simple 2D Scene
* Date due: 2025-02-15, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/


#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"                
#include "glm/gtc/matrix_transform.hpp"  
#include "ShaderProgram.h"               
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };


constexpr int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;


constexpr float BG_RED = 0.1922f,
BG_BLUE = 0.549f,
BG_GREEN = 0.9059f,
BG_OPACITY = 1.0f;


constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr char HEAD_SPRITE[] = "SmileHead.png";
constexpr char HAND_SPRITE[] = "SmileHand.png";

GLuint g_head_texture_id,
       g_hand_texture_id;

constexpr GLint NUMBER_OF_TEXTURES = 1, 
LEVEL_OF_DETAIL = 0,
TEXTURE_BORDER = 0;



AppStatus g_app_status = RUNNING;
SDL_Window* g_display_window;

ShaderProgram g_shader_program;

constexpr glm::vec3 INIT_SCALE_HEAD = glm::vec3(1.0f, 1.0f, 0.0f),
INIT_POS_HEAD = glm::vec3(0.0f, 0.0f, 0.0f),
INIT_ROT_HEAD = glm::vec3(0.0f, 0.0f, 0.0f),
INIT_POS_HAND = glm::vec3(2.0f, 2.0f, 0.0f),
INIT_ROT_HAND = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_head_pos = glm::vec3(0.0f, 0.0f, 0.0f),
g_head_scaler = glm::vec3(0.0f, 0.0f, 0.0f),
g_head_size = glm::vec3(1.0f, 1.0f, 0.0f);

constexpr float ANGLE = 45.0f;


float g_movement_angle = 0.0f,
g_head_rotation = 0.0f,
g_hand_rotation = 0.0f,
g_wave_speed = 160.0f,
g_previous_ticks = 0.0f;

glm::mat4 g_view_matrix,        
    g_head_matrix,
    g_hand_matrix,
    g_projection_matrix;

GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    // HARD INITIALISE ———————————————————————————————————————————————————————————————————
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Simple Scene",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    if (g_display_window == nullptr)
    {
        std::cerr << "ERROR: SDL Window could not be created.\n";
        g_app_status = TERMINATED;

        SDL_Quit();
        exit(1);
    }

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif


    // SOFT INITIALISE ———————————————————————————————————————————————————————————————————
   
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);  
    g_head_matrix = glm::mat4(1.0f); 
    g_hand_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);  

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_head_texture_id = load_texture(HEAD_SPRITE);
    g_hand_texture_id = load_texture(HAND_SPRITE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_app_status = TERMINATED;
        }
    }
}


void update() {
    float g_ticks = (float)SDL_GetTicks() / 1000.0f;
    float g_delta_time = g_ticks - g_previous_ticks;
    g_previous_ticks = g_ticks;

    g_head_matrix = glm::mat4(1.0f);
    g_hand_matrix = glm::mat4(1.0f);

    //Translations
    g_head_pos.x = cos(glm::radians(g_movement_angle));
    g_head_pos.y = abs(sin(glm::radians(g_movement_angle)));
    
    g_head_matrix = glm::translate(g_head_matrix, g_head_pos);
    g_hand_matrix = glm::translate(g_head_matrix, 1.2f*g_head_pos);

    g_movement_angle += ANGLE * g_delta_time;

    //Rotations
    g_head_matrix = glm::rotate(g_head_matrix,glm::radians(g_head_rotation), glm::vec3(0.0f, 0.0f, 1.0f));
    g_head_rotation += ANGLE * g_delta_time;

    if (g_hand_rotation > 90.0f && g_wave_speed >0.0f) {
        g_wave_speed = -300.0f;
    }
    else if (g_hand_rotation < -90.0f && g_wave_speed < 0.0f) {
        g_wave_speed = 300.0f;
    }
    g_hand_matrix = glm::rotate(g_hand_matrix, glm::radians(g_hand_rotation), glm::vec3(0.0f, 0.0f, 1.0f));
    g_hand_rotation += g_wave_speed * g_delta_time;

    //Scaling
    if (sin(glm::radians(g_movement_angle)) >=0.0f) {
        g_head_scaler = glm::vec3(0.2f, 0.2f, 0.0f);
    } else {
        g_head_scaler = glm::vec3(-0.2f, -0.2f, 0.0f);
    }
    g_head_size += g_head_scaler * g_delta_time;
    g_head_matrix = glm::scale(g_head_matrix, INIT_SCALE_HEAD);
    g_head_matrix = glm::scale(g_head_matrix, g_head_size);
    

    
}

void draw_object(glm::mat4& object_g_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    float vertices[] =
    {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    float texture_coordinates[] =
    {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false,
        0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT,
        false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    draw_object(g_hand_matrix, g_hand_texture_id);
    draw_object(g_head_matrix, g_head_texture_id);

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
  

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update(); 
        render();        
    }

    shutdown();
    return 0;
}