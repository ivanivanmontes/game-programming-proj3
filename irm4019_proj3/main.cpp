/**
* Author: Ivan Reynoso Montes
* Assignment: Pong Clone
* Date due: 2025-3-01, 11:59pm
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
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH  = 640 * 2,
WINDOW_HEIGHT = 480 * 2;

constexpr float BG_RED     = 0.9765625f,
BG_GREEN   = 0.6f,
BG_BLUE    = 1.0f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X      = 0,
VIEWPORT_Y      = 0,
VIEWPORT_WIDTH  = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;
constexpr float DEGREES_PER_SECOND = 90.0;

constexpr GLint NUMBER_OF_TEXTURES = 1,
LEVEL_OF_DETAIL    = 0,
TEXTURE_BORDER     = 0;

constexpr char GREEN_GUY_SPRITE[] = "rui.png",
PINK_GIRL_SPRITE[] = "totsuko.png",
BALL_SPRITE[] = "ball.png";

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();

glm::mat4 g_view_matrix,
g_model_matrix1,
g_model_matrix2,
g_model_matrix3,
g_projection_matrix;

float g_previous_ticks = 0.0f;

bool is_one_player = false;
bool go_down = false;

glm::vec3 g_paddle_position = glm::vec3(-3.0f, 0.0f, 0.0f);
glm::vec3 g_paddle_movement = glm::vec3(0, 0, 0);

glm::vec3 g_paddle2_position = glm::vec3(3.0f, 0.0f, 0.0f);
glm::vec3 g_paddle2_movement = glm::vec3(0, 0, 0);

glm::vec3 g_ball_position = glm::vec3(2.0f, 0.0f, 0.0f);
glm::vec3 g_ball_movement = glm::vec3(-1.5f, 0.0f, 0.0f);

constexpr glm::vec3 INIT_SCALE_BALL = glm::vec3(1.0f, 1.0f, 1.0f),
INIT_POS_BALL = glm::vec3(0.0f, 0.0f, 0.0f),
INIT_SCALE_PADDLE = glm::vec3(1.0f, 1.0f, 1.0f),
INIT_SCALE_PADDLE2 = glm::vec3(1.0f, 1.0f, 1.0f),
INIT_POS_PADDLE  = glm::vec3(-3.0f, 0.0f, 0.0f),
INIT_POS_PADDLE2   = glm::vec3(3.0f, 0.0f, 0.0f);



GLuint g_green_guy_texture_id,
g_pink_girl_texture_id,
g_ball_texture_id;


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

constexpr int FONTBANK_SIZE = 16;



void initialise()
{
    // Initialise video
    SDL_Init(SDL_INIT_VIDEO);
    
    g_display_window = SDL_CreateWindow("Project 2",
                                        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        WINDOW_WIDTH, WINDOW_HEIGHT,
                                        SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);
    
    g_view_matrix       = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);
    
    
    glUseProgram(g_shader_program.get_program_id());
    
    glClearColor(BG_RED, BG_GREEN, BG_BLUE, BG_OPACITY);
    
    g_green_guy_texture_id = load_texture(GREEN_GUY_SPRITE);
    g_pink_girl_texture_id = load_texture(PINK_GIRL_SPRITE);
    g_ball_texture_id = load_texture(BALL_SPRITE);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

bool check_collision(glm::vec3 ball_pos, glm::vec3 paddle_pos, glm::vec3 ball_size, glm::vec3 paddle_size) {
    float X_diff = fabs(paddle_pos.x - ball_pos.x);
    float Y_diff = fabs(paddle_pos.y - ball_pos.y);
    float X_distance = X_diff - ((paddle_size.x + ball_size.x) / 2.0f);
    float Y_distance = Y_diff - ((paddle_size.y + ball_size.y) / 2.0f);
    return (X_distance < 0 && Y_distance < 0);
}



void process_input()
{
    g_paddle_movement = glm::vec3(0.0f);
    g_paddle2_movement = glm::vec3(0.0f);
    if (!is_one_player) {
        g_paddle2_movement = glm::vec3(0.0f);
    }
    SDL_Event event;
    const Uint8 *key_state = SDL_GetKeyboardState(NULL); // if non-NULL, receives the length of the returned array
    while (SDL_PollEvent(&event))
    {
        
        switch (event.type)
        {
                // End game
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_app_status = TERMINATED;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_w:
                        g_paddle_movement.y = 1.0f;
                        break;
                        
                    case SDLK_s:
                        g_paddle_movement.y = -1.0f;
                        break;
                        
                    case SDLK_UP:
                        if (!is_one_player) {
                            g_paddle2_movement.y = 1.0f;
                        }
                        break;
                        
                    case SDLK_DOWN:
                        if (!is_one_player) {
                            g_paddle2_movement.y = -1.0f;
                        }
                        
                        break;
                        
                    case SDLK_t:
                        is_one_player = !is_one_player;
                        break;
                        
                    case SDLK_q:
                        g_app_status = TERMINATED;
                        break;
                    
                        
                    default:
                        break;
                }
                
            default:
                break;
        }
        
        if (glm::length(g_paddle_movement) > 1.0f)
        {
            g_paddle_movement = glm::normalize(g_paddle_movement);
        }
        else if (glm::length(g_paddle2_movement) > 1.0f)
        {
            g_paddle2_movement = glm::normalize(g_paddle2_movement);
        }
    }
    
}
    
void update()
{
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    g_paddle_position += g_paddle_movement * 8.0f * delta_time;
    g_paddle2_position += g_paddle2_movement * 8.0f * delta_time;
    g_ball_position += g_ball_movement * delta_time;
    
    if (g_paddle_position.y < 3.1f && g_paddle_position.y > -3.1f) { // if in bounds
        g_model_matrix1 = glm::mat4(1.0f);
        g_model_matrix1 = glm::translate(g_model_matrix1, g_paddle_position);
        g_model_matrix1 = glm::scale(g_model_matrix1, INIT_SCALE_PADDLE);
    }
    
    if (g_paddle2_position.y < 3.1f && g_paddle2_position.y > -3.1f) { // if in bounds
        
        g_model_matrix2 = glm::mat4(1.0f);
        g_model_matrix2 = glm::scale(g_model_matrix2, INIT_SCALE_PADDLE2);
        if (!is_one_player) { /// if we are two players
            g_model_matrix2 = glm::translate(g_model_matrix2, g_paddle2_position);
        }
        else { /// if we are just one player
            g_paddle2_position.y +=  (go_down ? -3.0f : 3.0f) * delta_time;
            g_model_matrix2 = glm::translate(g_model_matrix2, glm::vec3(g_paddle2_position));
            
        }
    }
    else {
        if (is_one_player) { // bounce off
            go_down = (g_paddle2_position.y >= 3.1f ? true : false);
            g_model_matrix2 = glm::mat4(1.0f);
            g_model_matrix2 = glm::scale(g_model_matrix2, INIT_SCALE_PADDLE2);
            g_paddle2_position.y +=  (go_down ? -3.0f : 3.0f) * delta_time;
            g_model_matrix2 = glm::translate(g_model_matrix2, glm::vec3(g_paddle2_position.x, g_paddle2_position.y, 0.0f));
        }
    }
    /// requirement 4: game over
    if (g_ball_position.x < -6.0f || g_ball_position.x > 6.0f) {
        g_app_status = TERMINATED;
    }
    
    g_model_matrix3 = glm::mat4(1.0f);
    g_model_matrix3  = glm::scale(g_model_matrix3, INIT_SCALE_BALL);
    g_model_matrix3 = glm::translate(g_model_matrix3, glm::vec3(g_ball_position));
        
    /// if colliding with left paddle
    if (check_collision(g_ball_position, g_paddle_position, INIT_SCALE_BALL, INIT_SCALE_PADDLE)) {
        g_ball_movement.x = fabs(g_ball_movement.x);
        g_ball_movement.y = (g_ball_position.y - g_paddle_position.y ) * 1.5f;
    }
    
    /// if colliding with right paddle
    if (check_collision(g_ball_position, g_paddle2_position, INIT_SCALE_BALL, INIT_SCALE_PADDLE2)) {
        g_ball_movement.x = -fabs(g_ball_movement.x);
        g_ball_movement.y = (g_ball_position.y - g_paddle2_position.y) * 1.5f;
    }
    
    /// requirement 3: bounce off walls
    if (g_ball_position.y + (INIT_SCALE_BALL.y / 2) >= 3.75f || g_ball_position.y - (INIT_SCALE_BALL.y / 2) <= -3.75f) {
        g_ball_movement.y = -g_ball_movement.y;
    }
    
    
    
    
    
}
    
void draw_object(glm::mat4 &object_g_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
    
    
    
    
void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    
    float vertices[] =
    {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
    };
    
    float texture_coordinates[] =
    {
        0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f,
    };
    
    
    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false,
                          0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT,
                          false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    draw_object(g_model_matrix1, g_green_guy_texture_id);
    draw_object(g_model_matrix2, g_pink_girl_texture_id);
    draw_object(g_model_matrix3, g_ball_texture_id);
    
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
