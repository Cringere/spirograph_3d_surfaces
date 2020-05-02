#include <CREngine/GUI.h>

#include <CREngine/AssetManager.h>
#include <CREngine/MainSpace.h>
#include <CREngine/InputManager.h>
#include <cmath>

#include <iostream>

using namespace CREngine::GUI;
using CREngine::Math::Vector2D;


Camera::Camera() {}

Camera::Camera(Vector2D position, Vector2D size) : position(position), size(size) {}

Vector2D Camera::zoomed_size() {
	return size * zoom;
}

Vector2D Camera::to_world(Vector2D screen) {
	return position + zoomed_size() * (screen - Vector2D(0.5f, 0.5f));
}

///////////////////// Buffer /////////////////////
GUIBatcher::GUIBatcher() {
	
}

GUIBatcher::GUIBatcher(unsigned int size) {
	init(size);
}

void GUIBatcher::init(unsigned int size) {
	Batcher::init(size, std::vector<int> {2, 2, 3});
}

void GUIBatcher::add(float x, float y, float w, float h, float u, float v, float us, float vs) {
	add(x, y, w, h, u, v, us, vs, 1.0f, 1.0f, 1.0f);
}

void GUIBatcher::add(float x, float y, float w, float h, float u, float v, float us, float vs, float r, float g, float b) {
	//note that the uv height is flipped
	int elements = 6 * (2 + 2 + 3);
	float arr[elements] {
		x,		y,		u,		v + vs,		r, g ,b,
		x + w,	y,		u + us,	v + vs,		r, g ,b,
		x + w,	y + h,	u + us,	v,			r, g ,b,
		x + w,	y + h,	u + us,	v,			r, g ,b,
		x,		y + h,	u,		v,			r, g ,b,
		x,		y,		u,		v + vs,		r, g ,b,
	};
	add_data(arr, elements);
}

void GUIBatcher::add(const Vector2D &pos, const Vector2D &size, const Vector2D &uv, const Vector2D &uvs, const Math::Vector3D &color) {
	add(pos[0], pos[1], size[0], size[1], uv[0], uv[1], uvs[0], uvs[1], color[0], color[1], color[2]);
}

///////////////////// TextureContainers /////////////////////
TextureContainer::TextureContainer(Vector2D uv, Vector2D uvs) : TextureContainer(uv, uvs, CONTAINER_TYPE::NORMAL) {}
TextureContainer::TextureContainer(Vector2D uv, Vector2D uvs, CONTAINER_TYPE type) : uv(uv), uvs(uvs), type(type) {}
ThreePatchContainer::ThreePatchContainer(Vector2D uv, Vector2D uvs, Vector2D split, bool direction) : TextureContainer(uv, uvs, CONTAINER_TYPE::THREE_PATCH), split(split), direction(direction) {}
NinePatchContainer::NinePatchContainer(Vector2D uv, Vector2D uvs, Vector2D h_split, Vector2D v_split) : TextureContainer(uv, uvs, CONTAINER_TYPE::NINE_PATCH), h_split(h_split), v_split(v_split) {}

TextureAtlas::TextureAtlas(const std::string &pack_file, RenderUtils::Texture *texture) : texture(texture) {
	std::vector<std::string> lines = Utils::file_to_lines(pack_file);
	float image_w = 0, image_h = 0;
	for (std::string &line: lines) {
		std::vector<std::string> line_parts = Utils::split_string(line, ' ');

		if (line[0] == 'i') {
			image_w = std::stoi(line_parts[3]);
			image_h = std::stoi(line_parts[4]);
			continue;
		}
		
		if(line[0] != 't' && line[0] != '3' && line[0] != '9') continue;
		
		float pad = 1.0f;
		Vector2D uv = Vector2D((std::stoi(line_parts[2]) - pad) / image_w, (std::stoi(line_parts[3]) - pad) / image_h);
		Vector2D uvs = Vector2D((std::stoi(line_parts[4]) + 2 * pad) / image_w, (std::stoi(line_parts[5]) + 2 * pad) / image_h);

		switch(line[0]) {
			
			case 't': {
				containers[line_parts[1]] = std::shared_ptr<TextureContainer>(new TextureContainer(
						uv,
						uvs
				));
				break;
			}
			case '3': {
				//read 3-patch
				float d, s1 = std::stoi(line_parts[7]), s2 = std::stoi(line_parts[8]);
				if (line_parts[6].compare("h") == 0) {
					d = 0.0f;
					s1 /= image_w;
					s2 /= image_w;
				} else {
					s1 /= image_h;
					s2 /= image_h;
					d = 1.0f;
				}
				containers[line_parts[1]] = std::shared_ptr<TextureContainer>(new ThreePatchContainer(
						uv,
						uvs,
						Vector2D(s1, s2),
						d
				));
				break;
			}
			case '9': {
				//read 9-patch
				containers[line_parts[1]] = std::shared_ptr<TextureContainer>(new NinePatchContainer(
						uv,
						uvs,
						//hs
						Vector2D(std::stoi(line_parts[6]) / image_w, std::stoi(line_parts[7]) / image_h),
						//vs
						Vector2D(std::stoi(line_parts[8]) / image_w, std::stoi(line_parts[9]) / image_h)
				));
				break;
			}
		}
	}
}

TextureContainer *TextureAtlas::get(const std::string &name) {
	std::map<std::string, std::shared_ptr<TextureContainer>>::iterator it = containers.find(name);
	if (it == containers.end()) return nullptr;
	return it->second.get();
}

///////////////////// GUIElement /////////////////////
GUIElement::GUIElement() : GUIElement(Vector2D(0.0f, 0.0f), Vector2D(0.0f, 0.0f)) {}

GUIElement::GUIElement(const Vector2D &size) : GUIElement(Vector2D(0.0f, 0.0f), size) {}

GUIElement::GUIElement(const Vector2D &position, const Vector2D &size) : position(position), size(size), color(1.0f, 1.0f, 1.0f), pressed(false), pressed_time(0.0f), tex(nullptr) {}

void GUIElement::set_color(float r, float g, float b) {
	color.set(r, g, b);
}

void GUIElement::set_tex(TextureContainer *tex) {
	this->tex = tex;
}

//void ::GUIElementadd_transition(TextureContainer *tex) {}

void GUIElement::set_window(Window *window) {
	this->window = window;
}

bool GUIElement::is_point_inside(const Vector2D &point) {
	return point[0] > position[0] && point[0] < position[0] + size[0] &&
			point[1] > position[1] - size[1] && point[1] < position[1];
}

void GUIElement::on_press() {
	pressed = true;
	pressed_time = 0;
	pressed_position = position;
	for (std::function<bool(GUIElement *)> &e: on_press_events) {
		if (e(this))
			break;
	}
}

void GUIElement::on_release() {
	pressed = false;
}

bool GUIElement::handle_input() {
	if (InputManager::mouse_keys[0] == InputManager::KEY_STATE::JUST_PRESSED) {
		if (is_point_inside(window->wm->mouse_position)) {
			on_press();
			return true;
		}
	}
	return false;
}

void GUIElement::update(float dt) {
	if (pressed) {
		if (pressed) {
			if (InputManager::mouse_keys[0] == InputManager::KEY_STATE::JUST_RELEASED) {
				on_release();
			}
		}
		pressed_time += dt;
	}
	
	for (std::function<bool(GUIElement *)> &e: update_events) {
		if (e(this))
			break;
	}
	
	/*if (current_transition != null) {
		current_transition.update(dt);
		current_transition.apply();
		if (current_transition.is_finished()) {
			current_transition = null;
		}
	}*/
}

void GUIElement::add_to_batcher(GUIBatcher *base_batcher, GUIBatcher *text_batcher) {
	if (tex == nullptr) {
		base_batcher->add(
			position[0], position[1] - size[1],
			size[0], size[1],
			0.0f, 0.0f, 0.0f, 0.0f,
			color[0], color[1], color[2]
		);
	} else {
		if (tex->type == TextureContainer::CONTAINER_TYPE::NORMAL) {
			add_to_batcher_base(base_batcher);
		} else if (tex->type == TextureContainer::CONTAINER_TYPE::THREE_PATCH) {
			add_to_batcher_3_patch_base(base_batcher);
		} else { //NINE_PATCH
			add_to_batcher_9_patch_base(base_batcher);
		}
	}
}

void GUIElement::add_to_batcher_base(GUIBatcher *base_batcher) {
	base_batcher->add(
		position[0], position[1] - size[1],
		size[0], size[1],
		tex->uv[0], tex->uv[1],
		tex->uvs[0], tex->uvs[1],
		color[0], color[1], color[2]
	);
}

void GUIElement::add_to_batcher_3_patch_base(GUIBatcher *base_batcher) {
	ThreePatchContainer *c = (ThreePatchContainer *) tex;
	//uv
	float u = c->uv[0], v = c->uv[1], us = c->uvs[0], vs = c->uvs[1];
	float s1 = c->split[0], s2 = c->split[1];

	float y = position[1] - size[1];

	float c1 = size[1] * s1 / vs;
	float c2 = size[1] * s2 / vs;

	//left patch
	base_batcher->add(
			position[0], y,
			c1, size[1],
			u, v,
			s1, vs,
			color[0], color[1], color[2]);

	//middle patch
	base_batcher->add(
			position[0] + c1, y,
			size[0] - c1 - c2, size[1],
			u + s1, v,
			us - s1 - s2, vs,
			color[0], color[1], color[2]);

	//right patch
	base_batcher->add(
			position[0] + size[0] - c2, y,
			c2, size[1],
			u + us - s2, v,
			s2, vs,
			color[0], color[1], color[2]);
}

void GUIElement::add_to_batcher_9_patch_base(GUIBatcher *base_batcher) {
	NinePatchContainer *c = (NinePatchContainer *) tex;

	//uv
	float u = c->uv[0], v = c->uv[1], us = c->uvs[0], vs = c->uvs[1];
	float hs1 = c->h_split[0], hs2 = c->h_split[1];
	float vs1 = c->v_split[0], vs2 = c->v_split[1];
	float x, w, ux, uw;    //ever column starts at the same x and has the same width

	float y = position[1] - size[1];

	//top left
	x = position[0];
	w = nine_u;
	ux = u;
	uw = hs1;
	base_batcher->add(
			x, y,
			w, nine_u * (vs1 / hs1),
			ux, v + vs - vs2,
			uw, vs2,
			color[0], color[1], color[2]);

	//middle left
	base_batcher->add(
			x, y + nine_u,
			w, size[1] - nine_u * (vs1 / hs1) - nine_u * (vs2 / hs1),
			ux, v + vs1,
			uw, vs - vs1 - vs2,
			color[0], color[1], color[2]);

	//bottom left
	base_batcher->add(
			x, y + size[1] - nine_u * (vs2 / hs1),
			w, nine_u * (vs2 / hs1),
			ux, v,
			uw, vs1,
			color[0], color[1], color[2]);

	//top middle
	x = position[0] + nine_u;
	w = size[0] - 2.0f * nine_u;
	ux = u + hs1;
	uw = us - hs1 - hs2;
	base_batcher->add(
			x, y,
			w, nine_u * (vs1 / hs1),
			ux, v + vs - vs2,
			uw, vs2,
			color[0], color[1], color[2]);

	//middle middle
	base_batcher->add(
			x, y + nine_u,
			w, size[1] - nine_u * (vs1 / hs1) - nine_u * (vs2 / hs1),
			ux, v + vs1,
			uw, vs - vs1 - vs2,
			color[0], color[1], color[2]);

	//bottom middle
	base_batcher->add(
			x, y + size[1] - nine_u * (vs2 / hs1),
			w, nine_u * (vs2 / hs1),
			ux, v,
			uw, vs1,
			color[0], color[1], color[2]);

	//top right
	x = position[0] + size[0] - nine_u;
	w = nine_u;
	ux = u + us - hs2;
	uw = hs2;
	base_batcher->add(
			x, y,
			w, w * (vs1 / hs2),
			ux, v + vs - vs2,
			uw, vs2,
			color[0], color[1], color[2]);

	//middle right
	base_batcher->add(
			x, y + w * (vs1 / hs2),
			w, size[1] - w * (vs1 / hs2) - w * (vs2 / hs2),
			ux, v + vs1,
			uw, vs - vs1 - vs2,
			color[0], color[1], color[2]);

	//bottom right
	base_batcher->add(
			x, y + size[1] - w * (vs2 / hs2),
			w, w * (vs2 / hs2),
			ux, v,
			uw, vs1,
			color[0], color[1], color[2]);
}

///////////////////// Layout /////////////////////
Layout::Layout() : GUIElement() {}

Layout::Layout(const Vector2D &size) : GUIElement(size) {}

Layout::Layout(const Vector2D &position, const Vector2D &size) : GUIElement(position, size) {}

void Layout::add(GUIElement *e) {
	elements.push_back(std::shared_ptr<GUIElement>(e));
}

void Layout::set_window(Window *window) {
	GUIElement::set_window(window);
	for (std::shared_ptr<GUIElement> &e: elements) {
		e.get()->set_window(window);
	}
}

bool Layout::handle_input() {
	for (std::shared_ptr<GUIElement> &e: elements) {
		if (e.get()->handle_input())
			return true;
	}
	return GUIElement::handle_input();
}

void Layout::update(float dt) {
	for (std::shared_ptr<GUIElement> &e: elements) {
		e.get()->update(dt);
	}
	GUIElement::update(dt);
}

void Layout::add_to_batcher(GUIBatcher *base_batcher, GUIBatcher *text_batcher) {
	if(draw_base) {
		GUIElement::add_to_batcher(base_batcher, text_batcher);
	}
	for (std::shared_ptr<GUIElement> &e: elements) {
		e->add_to_batcher(base_batcher, text_batcher);
	}
}

//VLayout
VLayout::VLayout() : Layout() {}

VLayout::VLayout(const Math::Vector2D &size) : Layout(size) {}

VLayout::VLayout(const Vector2D &position, const Vector2D &size) : Layout(position, size) {}

void VLayout::place_elements() {
	float y = position[1] - margin;
	float x = position[0] + margin;
	for (std::shared_ptr<GUIElement> &e: elements) {
		GUIElement *g = e.get();

		g->position[0] = x;
		g->position[1] = y;
		y -= g->size[1] + margin;
	}

	//place elements inside the rows
	for (std::shared_ptr<GUIElement> &e: elements) {
		if (Layout *v = dynamic_cast<Layout *>(e.get())) {
			v->place_elements();
		}
	}
}

void VLayout::calculate_size() {
	float w = 0;
	float h = 0;
	//calculate rows sizes
	for (std::shared_ptr<GUIElement> &e: elements) {
		GUIElement *g = e.get();

		if (Layout *v = dynamic_cast<Layout *>(g)) {
			v->calculate_size();
		}

		w = fmax(g->size[0], w);
		h += g->size[1] + margin;
	}

	//set the window size
	size[0] = w + 2 * margin;
	size[1] = h + margin * (elements.size() - 1);
}

//HLayout
HLayout::HLayout() : Layout() {}

HLayout::HLayout(const Vector2D &size) : Layout(size) {}

HLayout::HLayout(const Vector2D &position, const Vector2D &size) : Layout(position, size) {}

void HLayout::place_elements() {
	float x = position[0];
	float y = position[1];
	for (std::shared_ptr<GUIElement> &e: elements) {
		GUIElement *g = e.get();

		g->position[0] = x;
		g->position[1] = y;

		x += g->size[0] + margin;
	}
}

void HLayout::calculate_size() {
	float w = 0, h = 0;
	for (std::shared_ptr<GUIElement> &e: elements) {
		GUIElement *g = e.get();
		if (Layout *v = dynamic_cast<Layout *>(g)) {
			v->calculate_size();
		}
		
		w += g->size[0];
		h = fmax(h, g->size[1]);
	}

	if (elements.size() != 0)
		w += margin * (elements.size() - 1);

	size[0] = w;
	size[1] = h + margin;
}

///////////////////// Label /////////////////////
Label::Label(const Vector2D &size, RenderUtils::Font *font) : Label(size, font, "") {
	font_size = size[1] * 0.9f;
}

Label::Label(const Math::Vector2D &size, RenderUtils::Font *font, const std::string &text)  : GUIElement(size), font(font), editable(false), active(false) {
	set_text(text);
}
void Label::set_text(const std::string &text) {
	this->text = text;
	cursor_location = text.length();
}

void Label::set_text_alignment(TEXT_ALIGNMENT text_alignment) {
	this->text_alignment = text_alignment;
}

void Label::set_text_color(float r, float g, float b) {
	text_color.set(r, g, b);
}

void Label::add_string(char c) {
	cursor_time = 0;
	cursor_state = true;
	text += c;
	cursor_location++;
}

void Label::add_string(const std::string &text) {
	cursor_time = 0;
	cursor_state = true;
	this->text += text;
	cursor_location += text.length();
}

void Label::delete_text() {
	if (cursor_location >= 0) {
		if (cursor_location == text.length()) {
			text = text.substr(0, text.length() - 1);
		} else {
			text = text.substr(0, cursor_location) + text.substr(cursor_location + 1);
		}
		cursor_location--;
		cursor_state = true;
	}
}

void Label::fit_to_text(const std::string &text) {
	float s = 0.0f;
	for (int i = 0; i < text.length(); i++) {
		const RenderUtils::Font::Glyph *glyph = font->get_glyph((int) text[i]);
		s += font_size * glyph->advance;
	}
	s += font_size * (font->get_glyph(' ')->advance);
	text_width = s;
	size[0] = s;
	std::cout<<size[0]<<std::endl;
}

void Label::fit_to_text() {
	fit_to_text(text);
}

void Label::on_press() {
	GUIElement::on_press();
	if (editable) {
		cursor_time = 0;
		cursor_state = true;
		cursor_location = text.length() - 1;
	}
}

void Label::on_release() {
	GUIElement::on_release();
}

void Label::update(float dt) {
	if (pressed) active = true;
	else if (active) if (InputManager::mouse_keys[0] == InputManager::KEY_STATE::JUST_PRESSED) active = false;
	
	GUIElement::update(dt);

	if (editable) {
		cursor_time += dt;
		if (active) {
			if (cursor_time > cursor_rate) {
				cursor_state = !cursor_state;
				cursor_time -= cursor_rate;
			}
	
	
			//TODO: make more efficient
			//letters
			for (int i = 'a'; i <= 'z'; i++) {
				//capital letter
				int c = i;
				if (InputManager::keys[i] == InputManager::KEY_STATE::JUST_PRESSED) {
					if (InputManager::keys[InputManager::KEYS::KEY_L_SHIFT] == InputManager::KEY_STATE::DOWN) c += 65 - 97;
					add_string((char) c);
				}
			}
	
			//numbers - numpad
			/*for (int i = 320; i <= 329; i++) {
			if (InputManager::keys[i] == InputManager::KEY_STATE::JUST_PRESSED) {
			System.out.println(i);
			add_string('0' + (i - 320));
			}
			}*/
	
			//numbers
			for (int i = '0'; i <= '9'; i++) {
				if (InputManager::keys[i] == InputManager::KEY_STATE::JUST_PRESSED) {
					add_string((char) (i));
				}
			}
	
			//dot
			if (InputManager::keys['.'] == InputManager::KEY_STATE::JUST_PRESSED)
				add_string('.');
	
			//backspace
			if (InputManager::keys[InputManager::KEYS::KEY_BACKSPACE] == InputManager::KEY_STATE::JUST_PRESSED) {
				if (text.length() > 0)
					delete_text();
			}
			//backspace
			if (InputManager::keys[' '] == InputManager::KEY_STATE::JUST_PRESSED) {
				add_string(' ');
			}
		}
	}
}

void Label::add_to_batcher(GUIBatcher *base_batcher, GUIBatcher *text_batcher) {
	GUIElement::add_to_batcher(base_batcher, text_batcher);
	add_text_to_batcher(base_batcher, text_batcher);
}

void Label::add_text_to_batcher(GUIBatcher *base_batcher, GUIBatcher *text_batcher) {
	//calculate text width
	text_width = 0;
	for (int i = 0; i < text.length(); i++) {
		text_width += font_size * (font->get_glyph((int) text[i])->advance);
	}

	//horizontally center the text
	float x = position[0] + size[1] * 0.1f * 0.5f;
	switch (text_alignment) {
		case LEFT: {
			x = position[0] + size[1] * 0.1f * 0.5f;
			break;
		}
		case CENTER: {
			x = position[0] + (size[0] - text_width) / 2.0f;
			break;
		}
		case RIGHT: {
			x = position[0] + size[0] - text_width - size[1] * 0.1f * 0.5f;
			break;
		}
	}

	//vertically center the text
	float y = position[1] - size[1] - font->vertical_bounds[0] * font_size;
	char c;
	float w;

	if (-1 == cursor_location) {
		if (active && editable && cursor_state) {
			c = '|';
			const RenderUtils::Font::Glyph *g = font->get_glyph((int) c);
			w = font_size * g->uv_size[0];
			text_batcher->add(x, y,
					w, font_size,
					g->uv_pos[0], g->uv_pos[1], g->uv_size[0], g->uv_size[1],
					text_color[0], text_color[1], text_color[2]
			);
		}
	}

	for (int i = 0; i < text.length(); i++) {
		//get a character, and convert it to a * if necessary
		c = text[i];
		if (password) c = '*';
		const RenderUtils::Font::Glyph *g = font->get_glyph((int) c);

		//text width
		w = font_size * g->size[0];
		
		//add to batcher
		text_batcher->add(x + font_size * g->offset[0], y + font_size * g->offset[1],
				font_size * g->size[0], font_size * g->size[1],
				g->uv_pos[0], g->uv_pos[1], g->uv_size[0], g->uv_size[1],
				text_color[0], text_color[1], text_color[2]
		);
		
		//horizontally advance
		x += font_size * g->advance;

		if (i == cursor_location) {
			if (active && editable && cursor_state) {
				c = '|';
				const RenderUtils::Font::Glyph *g = font->get_glyph((int) c);
				w = font_size * g->uv_size[0];
				text_batcher->add(x + font_size * g->offset[0], y + font_size * g->offset[1],
						font_size * g->size[0], font_size * g->size[1],
						g->uv_pos[0], g->uv_pos[1], g->uv_size[0], g->uv_size[1],
						text_color[0], text_color[1], text_color[2]
				);
			}
		}
	}

	if (text.length() == cursor_location) {
		if (active && editable && cursor_state) {
			c = '|';
			const RenderUtils::Font::Glyph *g = &font->glyphs[(int) c];
			w = font_size * g->uv_size[0];
			text_batcher->add(x, y,
					w, font_size,
					g->uv_pos[0], g->uv_pos[1], g->uv_size[0], g->uv_size[1],
					text_color[0], text_color[1], text_color[2]
			);
		}
	}
}


///////////////////// Slider /////////////////////
Slider::Slider(const Math::Vector2D &size, RenderUtils::Font *font) : Label(size, font) {}

void Slider::set_fill_color(float r, float g, float b) {
	fill_color.set(r, g, b);
}

void Slider::update(float dt) {
	Label::update(dt);
	if (pressed) {
		progress = (window->wm->mouse_position[0] - position[0]) / size[0];
		progress = (float) fmin(fmax(progress, 0.0f), 1.0);
	}
}

void Slider::add_to_batcher(GUIBatcher *base_batcher, GUIBatcher *text_batcher) {
	Label::add_to_batcher(base_batcher, text_batcher);
	if (pointer_tex != nullptr)
		add_pointer_to_batcher(base_batcher, text_batcher);
}

void Slider::add_pointer_to_batcher(GUIBatcher *base_batcher, GUIBatcher *text_batcher) {
	float u = pointer_tex->uv[0], v = pointer_tex->uv[1], us = pointer_tex->uvs[0], vs = pointer_tex->uvs[1];
	float h = size[1];
	float w = h * us / vs;

	base_batcher->add(
		position[0] + size[0] * progress - w / 2.0f, position[1] - size[1],
		w, h,
		u, v, us, vs,
		color[0], color[1], color[2]
	);
}

void Slider::add_to_batcher_3_patch_base(GUIBatcher *base_batcher) {
	ThreePatchContainer *c = (ThreePatchContainer *) tex;
	//uv
	float u = c->uv[0], v = c->uv[1], us = c->uvs[0], vs = c->uvs[1];
	float s1 = c->split[0], s2 = c->split[1];

	float y = position[1] - size[1];

	float c1 = size[1] * s1 / vs;
	float c2 = size[1] * s2 / vs;

	Math::Vector3D selected_color = fill_color;
	float br = size[0] * progress;

	//left patch
	if (br < c1) {
		float w = c1;
		base_batcher->add(
				position[0], y,
				br, size[1],
				u, v,
				s1 * br / c1, vs,
				selected_color[0], selected_color[1], selected_color[2]);

		selected_color = color;

		base_batcher->add(
				position[0] + br, y,
				c1 - br, size[1],
				u + s1 * br / c1, v,
				s1 - s1 * br / c1, vs,
				selected_color[0], selected_color[1], selected_color[2]);
	} else {
		base_batcher->add(
				position[0], y,
				c1, size[1],
				u, v,
				s1, vs,
				selected_color[0], selected_color[1], selected_color[2]);
	}

	//middle patch
	if (br >= c1 && br < size[0] - c2) {
		float w = size[0] - c1 - c2;
		br -= c1;
		float uu = u + s1;
		float uus = us - s1 - s2;
		base_batcher->add(
				position[0] + c1, y,
				br, size[1],
				uu, v,
				uus * br / w, vs,
				selected_color[0], selected_color[1], selected_color[2]);

		selected_color = color;

		base_batcher->add(
				position[0] + c1 + br, y,
				w - br, size[1],
				uu, v,
				uus - uus * br / w, vs,
				selected_color[0], selected_color[1], selected_color[2]);
	} else {
		base_batcher->add(
				position[0] + c1, y,
				size[0] - c1 - c2, size[1],
				u + s1, v,
				us - s1 - s2, vs,
				selected_color[0], selected_color[1], selected_color[2]);
	}

	//right patch
	if (br >= size[0] - c2) {
		float x = position[0] + size[0] - c2;
		float w = c2;
		float uu = u + us - s2;
		float uus = s2;
		br -= size[0] - c2;
		base_batcher->add(
				x, y,
				br, size[1],
				uu, v,
				uus * br / w, vs,
				selected_color[0], selected_color[1], selected_color[2]);

		selected_color = color;

		base_batcher->add(
				x + br, y,
				c2 - br, size[1],
				uu + uus * br / w, v,
				uus - uus * br / w, vs,
				selected_color[0], selected_color[1], selected_color[2]);
	} else {
		base_batcher->add(
				position[0] + size[0] - c2, y,
				c2, size[1],
				u + us - s2, v,
				s2, vs,
				selected_color[0], selected_color[1], selected_color[2]);
	}
}

///////////////////// ToggleButton /////////////////////
ToggleButton::ToggleButton(const Math::Vector2D &size, RenderUtils::Font *font) : Label(size, font) {}

void ToggleButton::set_color(float r, float g, float b) {
	color_on.set(r, g, b);
	color_off.set(r, g, b);
}

void ToggleButton::set_color_on(float r, float g, float b) {
	color_on.set(r, g, b);
	apply();
}
void ToggleButton::set_color_off(float r, float g, float b) {
	color_off.set(r, g, b);
	apply();
}

void ToggleButton::set_tex(TextureContainer *tex) {
	this->tex_on = tex;
	this->tex_off = tex;
}

void ToggleButton::set_tex_on(TextureContainer *tex_on) {
	this->tex_on = tex_on;
	apply();
}
void ToggleButton::set_tex_off(TextureContainer *tex_off) {
	this->tex_off = tex_off;
	apply();
}

void ToggleButton::on_press() {
	Label::on_press();
	toggle();
}

void ToggleButton::toggle() {
	toggled = !toggled;
	apply();
}

void ToggleButton::apply() {
	if (toggled) {
		color = color_on;
		tex = tex_on;
	} else {
		color = color_off;
		tex = tex_off;
	}
}

///////////////////// Button /////////////////////
Button::Button(const Math::Vector2D &size, RenderUtils::Font *font) : ToggleButton(size, font) {}

void Button::on_release() {
	ToggleButton::on_release();
	ToggleButton::toggle();
}


///////////////////// Window /////////////////////
Window::Window(const Vector2D &position, const Vector2D &size, void (*f)(Window * w, WindowManager *wm)) : GUIElement(position, size), draw_header(false), init(f) {}

void Window::add_header(HEADER_TYPE header_type, const std::string &text, float size, RenderUtils::Font *font) {
	draw_header = true;
	header = std::shared_ptr<Label>(new GUI::Label(Vector2D(size, size), font));
	header->text = text;
	header->set_window(this);

	switch (header_type) {
		case CARD: {
			header->set_tex(wm->texture_atlas->get("label_c"));
			header->set_text_alignment(GUI::Label::TEXT_ALIGNMENT::CENTER);
			header->fit_to_text();
			header->size[0] += 2.0f * header->size[1];
			header->update_events.push_back([](GUIElement *e) {
					if (e->pressed) {
						e->window->position[0] = e->pressed_position[0] + e->window->wm->mouse_position[0] - e->window->wm->pressed_mouse_position[0];
						e->window->position[1] = e->pressed_position[1] - e->size[1] + e->window->wm->mouse_position[1] - e->window->wm->pressed_mouse_position[1];
					}
					return false;
				}
			);
			break;
		}
		case RECTANGLE: {
			header->set_tex(wm->texture_atlas->get("label"));
			header->set_text_alignment(GUI::Label::TEXT_ALIGNMENT::LEFT);
			header->update_events.push_back([](GUIElement *e) {
					e->size[0] = e->window->size[0];
					return false;
				}
			);
			header->update_events.push_back([](GUIElement *e) {
					if (e->pressed) {
						e->window->position[0] = e->pressed_position[0] + e->window->wm->mouse_position[0] - e->window->wm->pressed_mouse_position[0];
						e->window->position[1] = e->pressed_position[1] - e->size[1] + e->window->wm->mouse_position[1] - e->window->wm->pressed_mouse_position[1];
					}
					return false;
				}
			);
			break;
		}
	}
}

void Window::set_window() {
	GUIElement::set_window(this);
	main_layout->set_window(this);
	if (draw_header) {
		header->set_window(this);
	}
}

bool Window::handle_input() {
	if (main_layout->handle_input()) return true;

	if (InputManager::mouse_keys[0] == InputManager::KEY_STATE::JUST_PRESSED) {
		if (is_point_inside(wm->mouse_position)) {
			wm->bring_to_front(this);

			//bottom left is pressed - window resize event
			if (wm->mouse_position[0] > position[0] + size[0] - nine_u)
				if (wm->mouse_position[1] < position[1] - size[1] + nine_u)
					set_pressed();
		}
	}

	if (pressed) {
		//handel resize
		//TODO: move mouse_pressed_position to the wm
		if (resizable) {
			size[0] = pressed_size[0] + wm->mouse_position[0] - mouse_pressed_position[0];
			size[1] = pressed_size[1] + mouse_pressed_position[1] - wm->mouse_position[1];
			size[0] = fmax(size[0], min_size[0]);
			size[1] = fmax(size[1], min_size[1]);
		}

		//TODO: idk what to do with these lines
		/*Utils.copy(size, main_layout->size);
		Utils.copy(position, main_layout->position);
		main_layout->align_elements();
		*/

		//handel move

		//handel release
		if (InputManager::mouse_keys[0] == InputManager::KEY_STATE::JUST_RELEASED)
			pressed = false;
	}

	//header
	if (draw_header) {
		if (header->handle_input()) return true;
	}

	return GUIElement::handle_input();
}

void Window::set_pressed() {
	pressed = true;
	mouse_pressed_position = wm->mouse_position;
	pressed_size = size;
}

void Window::calculate_size() {
	main_layout->calculate_size();
	size[0] = main_layout->size[0] + 2.0f * row_margin;
	size[1] = main_layout->size[1] + 2.0f * row_margin;
}

void Window::update(float dt) {
	GUIElement::update(dt);

	//set window size
	//Utils.copy(main_layout->size, size);

	//header
	if (draw_header) {
		header->update(dt);
		header->position[0] = position[0];
		header->position[1] = position[1] + header->size[1];
	}

	//place window
	calculate_size();
	main_layout->position[0] = position[0] + row_margin;
	main_layout->position[1] = position[1] - row_margin;
	main_layout->place_elements();

	//place elements
	main_layout->update(dt);
	main_layout->calculate_size();
}

void Window::add_to_batcher(GUIBatcher *base_batcher, GUIBatcher *text_batcher) {
	if (draw_base) GUIElement::add_to_batcher(base_batcher, text_batcher);
	main_layout->add_to_batcher(base_batcher, text_batcher);

	//header
	if (draw_header)
		header->add_to_batcher(base_batcher, text_batcher);
}

///////////////////// SimpleWindow /////////////////////
SimpleWindow::SimpleWindow(const Vector2D &position, void (*f)(Window * w, WindowManager *wm)) : Window(position, Vector2D(0.0f, 0.0f), f) {
	resizable = false;

	main_layout = std::shared_ptr<VLayout>(new VLayout(position, Vector2D(0.5f, 0.5f)));
	nine_u = 0.02f;
	min_size[0] = nine_u * 2.5f;
	min_size[1] = nine_u * 2.5f;

	current_row = new HLayout();
}

void SimpleWindow::add_element(GUIElement *e) {
	current_row->elements.push_back(std::shared_ptr<GUIElement>(e));
}

void SimpleWindow::add_row() {
	current_row->margin = row_margin;
	main_layout->add(current_row);
	current_row = new HLayout();
}


///////////////////// WindowManager /////////////////////
WindowManager::WindowManager() : draw_bg(false) {
	camera.size[0] = 1.0f;
	camera.size[1] = camera.size[0] * (float) (MainSpace::get_height()) / MainSpace::get_width();
	
	camera.position[0] = camera.size[0] / 2.0f;
	camera.position[1] = camera.size[1] / 2.0f;
}

void WindowManager::init(TextureAtlas *texture_atlas, RenderUtils::Font *font, int base_batcher_size, int text_batcher_size){
	this->texture_atlas = std::shared_ptr<TextureAtlas>(texture_atlas);
	this->font = font;
	this->shader = AssetManager::get_shader("GUI_shader");
	base_batcher.init(base_batcher_size);
	text_batcher.init(text_batcher_size);
	image_batcher.init(1000);
}

void WindowManager::add_window(Window *window) {
	window->window_number = window_order.size();
	window->wm = this;
	window->texture_atlas = texture_atlas.get();

	window->init(window, this);
	window->calculate_size();
	window->set_window();
	std::cout<<"add"<<std::endl;
	
	windows.push_back(std::shared_ptr<Window>(window));
	window_order.insert(window_order.begin(), window_order.size());
}

void WindowManager::set_bg(RenderUtils::Texture *texture) {
	draw_bg = true;
	this->bg_texture = texture;
	image_batcher.clear();
	
	float r = (float) bg_texture->get_height() / bg_texture->get_width();
	float s = 1.0f;
	if (r < camera.size[1] / camera.size[0])
		s = camera.size[1] / r;
	image_batcher.add(0, 0, 1.0f * s, r * s, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

	image_batcher.update();
}

void WindowManager::bring_to_front(Window *window) {
	int n = window->window_number;
	int move = -1;
	for (int i = 0; i < window_order.size(); i++) {
		if (window_order[i] == n) {
			move = i;
		}
	}
	if (move >= 0) {			//move window if found (!=-1) and if not already on top (!=0)
		window_order.erase(window_order.begin() + move);
		window_order.insert(window_order.begin(), n);
	}
}

bool WindowManager::update(float dt) {
	//handel window resize
	camera.size[0] = 1.0f;
	camera.size[1] = camera.size[0] * (float) (MainSpace::get_height()) / MainSpace::get_width();

	bool handled_input = false;

	//project mouse to camera view
	mouse_position = camera.to_world(InputManager::mouse_position);

	//mouse pressed positoin
	if (InputManager::mouse_keys[0] == InputManager::KEY_STATE::JUST_PRESSED) {
		pressed_mouse_position = mouse_position;
	}

	//handle input
	for (int i = 0; i < window_order.size(); i++) {
		Window *w = windows[window_order[i]].get();
		if (w->handle_input()) {
			bring_to_front(w);
			handled_input = true;
			break;
		}
	}

	//update
	for (int i = 0; i < window_order.size(); i++) {
		windows[window_order[i]].get()->update(dt);
	}

	return handled_input;
}

void WindowManager::render() {
	shader->use();

	shader->bind_vec("camera_position", camera.position);
	shader->bind_vec("camera_size", camera.size);
	shader->bind_vec("viewport_size", Vector2D(MainSpace::get_width(), MainSpace::get_height()));

	if (draw_bg) {
		bg_texture->use_as_0();
		image_batcher.render();
	}

	for (int i = window_order.size() - 1; i >= 0; i--) { //the first window should be rendered last because it is on top
		Window *w = windows[window_order[i]].get();

		base_batcher.clear();
		text_batcher.clear();

		w->add_to_batcher(&base_batcher, &text_batcher);

		base_batcher.update();
		text_batcher.update();

		texture_atlas->texture->use_as_0();
		base_batcher.render();

		font->use_as_0();
		text_batcher.render();
	}
}
