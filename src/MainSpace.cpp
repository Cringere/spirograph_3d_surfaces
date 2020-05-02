#include <CREngine/MainSpace.h>

#include <GL/glew.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <CREngine/AssetManager.h>
#include <CREngine/InputManager.h>

//main functions
static void (*init_function)();
static void (*handle_input_function)();
static void (*update_function)(float dt);
static void (*render_function)();
static void (*dispose_function)();

//window variables
static SDL_Window *window = nullptr;
static SDL_GLContext glContex;

//libraries variables

using namespace CREngine::MainSpace;

//general variables
static bool running = true;
static std::string exe_path;
static const Configuration *conf;

//extern variables
const std::string &CREngine::MainSpace::EXE_PATH = exe_path;
FT_Library CREngine::MainSpace::ft;

//Configuration
Configuration::Configuration(): Configuration("CREngine projcet", 1280, 720, 60) {}

Configuration::Configuration(std::string window_name, unsigned int window_width, unsigned int window_height, unsigned int fps) : window_name(window_name), window_width(window_width), window_height(window_height), fps(fps), gl_major_version(3), gl_minor_version(3) ,gl_red_size(8), gl_green_size(8), gl_blue_size(8), gl_alpha_size(8), gl_depth_size(2), gl_double_buffer(1) {}

Configuration::Configuration(const Configuration &configuration) : window_name(window_name), window_width(window_width), window_height(window_height), fps(fps), gl_major_version(gl_major_version), gl_minor_version(gl_minor_version), gl_red_size(gl_red_size), gl_green_size(gl_green_size), gl_blue_size(gl_blue_size), gl_alpha_size(gl_alpha_size), gl_depth_size(gl_depth_size), gl_double_buffer(gl_double_buffer) {}

//Mainspace
void CREngine::MainSpace::set_main_function_pointers(void (*init)(), void (*handle_input)(), void (*update)(float dt), void (*render)(), void (*dispose)()) {
	init_function = init;
	handle_input_function = handle_input;
	update_function = update;
	render_function = render;
	dispose_function = dispose;
}

int CREngine::MainSpace::run(Configuration &configuration) {
	conf = &configuration;
	exe_path = std::string(SDL_GetBasePath());
	
	//init libraries
	if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to initialize SDL: %s", SDL_GetError());
		return -1;
	}

	if (FT_Init_FreeType(&ft)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to initialize FreeType.");
		return -1;
	}

	if (!(IMG_Init(IMG_INIT_PNG))) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to initialize SDL_image: %s", IMG_GetError());
		return -1;
	}
	
	//create window
	//SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);
	window = SDL_CreateWindow(configuration.window_name.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, configuration.window_width, configuration.window_height, SDL_WINDOW_OPENGL);
	if(window == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to open window: %s", SDL_GetError());
		return 1;
	}

	//set opengl settings
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, configuration.gl_minor_version);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, configuration.gl_major_version);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	//SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 32);

	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);

	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1); 
	glEnable(GL_MULTISAMPLE);


	//init opengl
	glContex = SDL_GL_CreateContext(window);
	glewExperimental = GL_TRUE;
	GLenum error = glewInit();
	if (error != GLEW_OK) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize glew: %s\n", glewGetErrorString(error));
		return -1;
	}

	//init
	AssetManager::init();
	InputManager::init();
	init_function();
	unsigned int mspf = 1000 / configuration.fps;
	Uint32 time,currentTime;
	Uint64 now = SDL_GetPerformanceCounter(), last;
	SDL_Delay(mspf);

	//main loop
	running = true;
	while (running) {
		time  = SDL_GetTicks();

		SDL_Event event;
		while(SDL_PollEvent(&event)){
			InputManager::handle_input(event);
			if(SDL_QuitRequested() || event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)){
				running = false;
			}
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
				configuration.window_width = event.window.data1;
				configuration.window_height = event.window.data2;
			}
		}
		handle_input_function();
		
		//update
		last = now;
		now = SDL_GetPerformanceCounter();
		float delta = (float)(now-last) / SDL_GetPerformanceFrequency();
		update_function(delta);
		InputManager::update(delta);
		
		render_function();
		
		SDL_GL_SwapWindow(window);

		//render
		currentTime = SDL_GetTicks();
		if(currentTime - time < mspf)SDL_Delay(mspf - (currentTime - time));
	}

	//resources - dispose
	AssetManager::dispose();
	InputManager::dispose();
	dispose_function();
	
	//libraries - dispose
	FT_Done_FreeType(ft);
	SDL_GL_DeleteContext(glContex);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

void CREngine::MainSpace::exit() {
	running = false;
}

int CREngine::MainSpace::get_width() {
	return conf->window_width;
}

int CREngine::MainSpace::get_height() {
	return conf->window_height;
}
