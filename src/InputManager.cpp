#include <CREngine/InputManager.h>
#include <CREngine/MainSpace.h>

#include <iostream>

using namespace CREngine::InputManager;

KEY_STATE CREngine::InputManager::keys[1024];
float CREngine::InputManager::keys_time_pressed[1024];

KEY_STATE CREngine::InputManager::mouse_keys[3];
float CREngine::InputManager::mouse_keys_time_pressed[3];

CREngine::Math::Vector2D CREngine::InputManager::mouse_position;

void CREngine::InputManager::init() {
	for (int i = 0; i < 1024; ++i) {
		keys[i] = UP;
		keys_time_pressed[i] = 0.0f;
	}

	for (int i = 0; i < 3; ++i) {
		keys_time_pressed[i] = 0.0f;
		mouse_keys[i] = UP;
	}
}

void CREngine::InputManager::handle_input(const SDL_Event &event) {
	int press_mode = 0;

	KEY_STATE mouse_state = KEY_STATE::JUST_RELEASED;

	switch(event.type) {
		case SDL_MOUSEMOTION : {
			int x, y;
			SDL_GetMouseState(&x, &y);
			mouse_position.set((float) (x - 1) / MainSpace::get_width(), 1.0f - (float) (y) / MainSpace::get_height());
			break;
		}

		case SDL_MOUSEBUTTONDOWN : {
			mouse_state = KEY_STATE::JUST_PRESSED;
		}
		case SDL_MOUSEBUTTONUP : {
			if (event.button.button == SDL_BUTTON_LEFT)
				mouse_keys[0] = mouse_state;
			else if (event.button.button == SDL_BUTTON_MIDDLE)
				mouse_keys[1] = mouse_state;
			else if (event.button.button == SDL_BUTTON_RIGHT)
				mouse_keys[2] = mouse_state;
			break;
		}



		case SDL_KEYDOWN: {
			press_mode = 1;
		}
		case SDL_KEYUP: {
			int i = event.key.keysym.sym, target = -1;

			//characters
			if (i >= 97 && i <= 122) target = i;
			//numbers
			else if (i >= 48 && i <= 57) target = i;
				//special keys
			else {
				switch (i) {
					case 8: target = KEY_BACKSPACE; break;
					case 32: target = KEY_SPACE; break;
					
					case 1073742048: target = KEY_L_CTRL; break;
					case 1073742052: target = KEY_R_CTRL; break;
					case 1073742049: target = KEY_L_SHIFT; break;
					case 1073742053: target = KEY_R_SHIFT; break;
					case 1073742050: target = KEY_L_ALT; break;
					case 1073742054: target = KEY_R_ALT; break;

					case 1073741904: target = KEY_ARROW_LEFT; break;
					case 1073741905: target = KEY_ARROW_DOWN; break;
					case 1073741906: target = KEY_ARROW_UP; break;
					case 1073741903: target = KEY_ARROW_RIGHT; break;
				}
			}

			if (target != -1) {
				if (press_mode == 0) {
					keys[target] = JUST_RELEASED;
					keys_time_pressed[target] = 0;
				} else if (keys[target] == UP) {
					keys[target] = JUST_PRESSED;
					keys_time_pressed[target] = 0;
				}
			}
			break;
		}
	}
}

void CREngine::InputManager::update(float dt) {
	for (int i = 0; i < 1024; ++i) {
		if (keys[i] == DOWN) keys_time_pressed[i] += dt;
		if (keys[i] == JUST_PRESSED) keys[i] = DOWN;
		if (keys[i] == JUST_RELEASED) keys[i] = UP;
	}

	for (int i = 0; i < 3; ++i) {
		if (mouse_keys[i] == DOWN) keys_time_pressed[i] += dt;
		if (mouse_keys[i] == JUST_PRESSED) mouse_keys[i] = DOWN;
		if (mouse_keys[i] == JUST_RELEASED) mouse_keys[i] = UP;
	}
}

void CREngine::InputManager::dispose() {

}
