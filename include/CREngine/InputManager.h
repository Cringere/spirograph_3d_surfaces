#ifndef CRENGINE_PUBLIC_HEADER_INPUT_MANAGER
#define CRENGINE_PUBLIC_HEADER_INPUT_MANAGER

#include <SDL2/SDL.h>

#include <CREngine/Math.h>

namespace CREngine {
	namespace InputManager {
		enum KEYS {
			KEY_L_CTRL = 1, KEY_R_CTRL, KEY_L_SHIFT, KEY_R_SHIFT, KEY_L_ALT, KEY_R_ALT,
			KEY_ARROW_LEFT, KEY_ARROW_DOWN, KEY_ARROW_UP, KEY_ARROW_RIGHT,
			KEY_BACKSPACE = 8, KEY_SPACE = 32, 
			KEY_A = 97, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,
			KEY_0 = (int) '0', KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
		};
		enum KEY_STATE {
			JUST_PRESSED = 1, DOWN, JUST_RELEASED, UP
		};

		extern KEY_STATE keys[1024];
		extern float keys_time_pressed[1024];

		extern KEY_STATE mouse_keys[3];
		extern float mouse_keys_time_pressed[3];

		extern Math::Vector2D mouse_position;

		void init();
		void handle_input(const SDL_Event &event);
		void update(float dt);
		void dispose();
	}
}

#endif