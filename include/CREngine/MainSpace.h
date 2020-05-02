#ifndef CRENGINE_PUBLIC_HEADER_MAINSPACE
#define CRENGINE_PUBLIC_HEADER_MAINSPACE

#include <ft2build.h>
#include FT_FREETYPE_H

#include <string>

namespace CREngine {
	namespace MainSpace {
		class Configuration {
			public:
				const std::string window_name;
				unsigned int window_width, window_height;
				const unsigned int fps; 

				const unsigned int gl_major_version, gl_minor_version;
				const unsigned int gl_red_size, gl_green_size, gl_blue_size, gl_alpha_size, gl_depth_size;
				const unsigned int gl_double_buffer;

				Configuration();			
				Configuration(const Configuration &configuration);
				Configuration(std::string window_name, unsigned int window_width, unsigned int window_height, unsigned int fps);
				
				Configuration &operator=(const Configuration &configuration);
			};

		extern const std::string &EXE_PATH;
		extern FT_Library ft;
		
		void set_main_function_pointers(void (*init)(), void (*handle_input)(), void (*update)(float dt), void (*render)(), void (*dispose)());
				
		int run(Configuration &configuration);

		void exit();

		int get_width();

		int get_height();
	}
}

#endif
