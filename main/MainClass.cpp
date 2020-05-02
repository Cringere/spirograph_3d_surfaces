#include <GL/glew.h>

#include <iostream>

#include <CREngine/MainSpace.h>
#include <CREngine/AssetManager.h>
#include <CREngine/RenderUtils.h>
#include <CREngine/GUI.h>
#include <functional>
#include <CREngine/InputManager.h>


using namespace CREngine;

GUI::WindowManager *wm;
GUI::GUIElement *e;
GUI::GUIBatcher b;

void init() {
	wm = new GUI::WindowManager();

	wm->init(
		new GUI::TextureAtlas("skin.pack", AssetManager::get_texture("skin")),
		AssetManager::get_font("font"),
		5000, 5000
	);

	wm->set_bg(AssetManager::get_texture("bg"));

	wm->add_window(new GUI::SimpleWindow(Math::Vector2D(0.2f, 0.5f),
		[](GUI::Window *wi, GUI::WindowManager *wm) {
			GUI::SimpleWindow *w = (GUI::SimpleWindow *) wi;
			w->set_color(1.0f, 1.0f, 1.0f);
			w->set_tex(wm->texture_atlas->get("window_b"));

			float u = 0.02f;
			w->margin = u * 0.3f;
			w->nine_u = u;
			w->row_margin = w->margin * 1.1f;
			RenderUtils::Font *font = wm->font;

			//helper variable
			GUI::Label *l;
			GUI::Slider *s;
			GUI::ToggleButton *b;

			//set elements
			l = new GUI::Label(Math::Vector2D(u * 7.0f, u), font);
			l->set_text("Elt[0, 0] TqPp|");
			l->set_tex(wm->texture_atlas->get("label"));
			l->editable = true;
			w->add_element(l);

			l = new GUI::Label(Math::Vector2D(u * 7.0f, u), font);
			l->set_tex(wm->texture_atlas->get("label"));
			w->add_element(l);

			b = new GUI::Button(Math::Vector2D(u, u), font);
			b->set_tex(wm->texture_atlas->get("monitor_b_off"));
			b->set_color_on(1.0f, 1.0f, 1.0f);
			b->set_color_off(0.0f, 0.0f, 0.0f);
			b->on_press_events.push_back([](GUI::GUIElement *e) {
					GUI::ToggleButton *b = (GUI::ToggleButton *) e;
					std::cout<<"pressed"<<std::endl;
					if (b->toggled) {
						//b->window->add_transition(new GUI::Transition(b->window, 0.0f, 0.2f, true, 0, position[0], position[0] - size[0] / 3.0f, new Utils.SinFunction()));
					} else {
						//b->window->add_transition(new GUI::Transition(b->window, 0.0f, 0.2f, true, 0, position[0], position[0] + size[0] / 3.0f, new Utils.SinFunction()));
					}
					return false;
				}
			);
			w->add_element(b);

			w->add_row();

			l = new GUI::Label(Math::Vector2D(u * 7.0f, u), font);
			l->set_tex(wm->texture_atlas->get("label"));
			l->set_text("element [1, 0]");
			w->add_element(l);

			l = new GUI::Label(Math::Vector2D(u * 7.0f, u), font);
			l->set_tex(wm->texture_atlas->get("label"));
			l->set_text("element [1, 1]");
			l->fit_to_text();
			w->add_element(l);

			w->add_row();

			s = new GUI::Slider(Math::Vector2D(u * 12.0f, u), font);
			s->pointer_tex = wm->texture_atlas->get("slider_pointer");
			s->set_tex(wm->texture_atlas->get("slider"));
			s->set_text("");
			w->add_element(s);


			b = new GUI::ToggleButton(Math::Vector2D(u, u), font);
			b->set_tex(wm->texture_atlas->get("monitor_b_off"));
			b->set_color(1.0f, 1.0f, 1.0f);
			b->set_color_off(0.0f, 0.0f, 0.0f);
			w->add_element(b);

			b = new GUI::ToggleButton(Math::Vector2D(u, u), font);
			b->set_tex(wm->texture_atlas->get("monitor_b_off"));
			b->set_color(1.0f, 1.0f, 1.0f);
			b->set_color_off(0.0f, 0.0f, 0.0f);
			w->add_element(b);

			w->add_row();

			l = new GUI::Label(Math::Vector2D(2.0f * u, u), font);
			l->set_tex(wm->texture_atlas->get("label"));
			l->set_text("some text");
			l->fit_to_text();
			l->set_color(0.5f, 0.8f, 0.9f);
			w->add_element(l);

			w->add_row();

			w->add_header(GUI::Window::HEADER_TYPE::RECTANGLE, "window", u, font);
			w->draw_base = true;
		}
	));

	wm->add_window(new GUI::SimpleWindow(Math::Vector2D(0.1f, 0.5f),
		[](GUI::Window *wi, GUI::WindowManager *wm) {
			GUI::SimpleWindow *w = (GUI::SimpleWindow *) wi;
			w->set_color(1.0f, 1.0f, 1.0f);
			w->set_tex(wm->texture_atlas->get("window_b"));

			float u = 0.03f;
			w->margin = u * 0.3f;
			w->nine_u = u;
			w->row_margin = w->margin * 1.1f;
			RenderUtils::Font *font = wm->font;

			//helper variable
			GUI::Label *l;
			GUI::Slider *s;
			GUI::ToggleButton *b;

			//set elements
			l = new GUI::Label(Math::Vector2D(u * 7.0f, u), font);
			l->set_text("element 1");
			l->set_tex(wm->texture_atlas->get("label"));
			l->editable = true;
			w->add_element(l);

			l = new GUI::Label(Math::Vector2D(u * 7.0f, u), font);
			l->set_color(0.1f, 0.5f, 0.8f);
			l->set_text("immutable");
			l->set_text_color(1.0f, 1.0f, 1.0f);
			l->set_tex(wm->texture_atlas->get("label"));
			w->add_element(l);

			w->add_row();

			s = new GUI::Slider(Math::Vector2D(u * 14.0f, u), font);
			s->pointer_tex = wm->texture_atlas->get("slider_pointer");
			s->set_tex(wm->texture_atlas->get("slider"));
			w->add_element(s);
			
			w->add_row();

			for (int i = 0; i < 10; ++i) {
				b = new GUI::Button(Math::Vector2D(u, u), font);
				b->set_tex(wm->texture_atlas->get("monitor_b_off"));
				b->set_color_on(1.0f, 1.0f, 1.0f);
				b->set_color_off(0.0f, 0.0f, 0.0f);
				b->on_press_events.push_back([i](GUI::GUIElement *e) {
						GUI::Button *b = (GUI::Button *) e;
						std::cout<<"pressed "<<i<<std::endl;
						return false;
					}
				);
				w->add_element(b);
			}

			w->add_row();

			w->add_header(GUI::Window::HEADER_TYPE::CARD, "window", u, font);
			w->draw_base = true;
	}));

	wm->add_window(new GUI::SimpleWindow(Math::Vector2D(0.1f, 0.5f),
		[](GUI::Window *wi, GUI::WindowManager *wm) {
			GUI::SimpleWindow *w = (GUI::SimpleWindow *) wi;
			w->set_color(1.0f, 1.0f, 1.0f);
			w->set_tex(wm->texture_atlas->get("window_b"));

			float u = 0.03f;
			w->margin = u * 0.3f;
			w->nine_u = u;
			w->row_margin = w->margin * 1.1f;
			RenderUtils::Font *font = wm->font;

			//helper variable
			GUI::Label *l;
			GUI::Slider *s;
			GUI::ToggleButton *b;

			//set elements
			l = new GUI::Label(Math::Vector2D(u * 7.0f, u), font);
			l->set_text("element 1");
			l->set_tex(wm->texture_atlas->get("label"));
			l->editable = true;
			w->add_element(l);

			l = new GUI::Label(Math::Vector2D(u * 7.0f, u), font);
			l->set_color(0.1f, 0.5f, 0.8f);
			l->set_text("immutable");
			l->set_tex(wm->texture_atlas->get("label"));
			w->add_element(l);

			w->add_row();

			s = new GUI::Slider(Math::Vector2D(u * 14.0f, u), font);
			s->pointer_tex = wm->texture_atlas->get("slider_pointer");
			s->set_tex(wm->texture_atlas->get("slider"));
			w->add_element(s);
			
			w->add_row();

			for (int i = 0; i < 10; ++i) {
				s = new GUI::Slider(Math::Vector2D(u * 14.0f, u), font);
				s->pointer_tex = wm->texture_atlas->get("slider_pointer");
				s->set_tex(wm->texture_atlas->get("slider"));
				w->add_element(s);
				
				w->add_row();
			}
			w->add_row();

			w->add_header(GUI::Window::HEADER_TYPE::CARD, "window", u, font);
			w->draw_base = true;
	}));
	
	b.init(1000);
	b.clear();
	e = new GUI::GUIElement(Math::Vector2D(0.5f, 0.5f), Math::Vector2D(0.3f, 0.1f));
	e->add_to_batcher(&b, nullptr);
	b.update();
}


void handle_input() {
	//wm->handle_input();
}

void update(float dt) {
	wm->update(dt);

	//wm->windows[0]->position[0] += 0.01f * dt;
	//wm->windows[0]->position[1] += 0.01f * dt;


	e->position[0] += 0.01f * dt;
	e->position[1] += 0.01f * dt;

	//wm->camera.position[0] -= 0.012f * dt;
	//wm->camera.position[1] -= 0.01f * dt;

	b.clear();
	e->add_to_batcher(&b, nullptr);
	b.update();
}

void render() {
	glClearColor(0.8, 0.8, 0.8, 1.0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClear(GL_COLOR_BUFFER_BIT);

	//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	wm->render();

	//b.render();
}

void dispose() {
	delete wm;
}

int main(int argc, char const *argv[]) {
	CREngine::MainSpace::set_main_function_pointers(
		&init,
		&handle_input,
		&update,
		&render,
		&dispose
	);

	CREngine::MainSpace::Configuration conf;

	if (CREngine::MainSpace::run(conf) != 0) return -1;

	return 0;
}
