#ifndef CRENGINE_PUBLIC_HEADER_GUI
#define CRENGINE_PUBLIC_HEADER_GUI

#include <vector>
#include <memory>
#include <functional>

#include <CREngine/Math.h>
#include <CREngine/RenderUtils.h>

namespace CREngine {
	namespace GUI {
		///////////////// Class List /////////////////
		class Camera;
		class GUIBatcher;
		class TextureContainer;
		class ThreePatchContainer;
		class NinePatchContainer;
		class TextureAtlas;

		class GUIElement;
		class Layout;
		
		class VLayout;
		class HLayout;
		
		class Label;
		class Slider;
		
		class Window;
		class WindowManager;
		class SimpleWindow;
		
		///////////////// GUI Utils /////////////////
		class Camera {
		public:
			Math::Vector2D position, size;
			float zoom = 1.0f;

			Camera();
			Camera(Math::Vector2D position, Math::Vector2D size);

			Math::Vector2D zoomed_size();

			Math::Vector2D to_world(Math::Vector2D screen);
		};

		class GUIBatcher : public RenderUtils::Batcher {
			std::vector<float> buffer;
		public:
			GUIBatcher();
			
			GUIBatcher(unsigned int size);
			
			void init(unsigned int size);

			void add(float x, float y, float w, float h, float u, float v, float us, float vs);
			void add(float x, float y, float w, float h, float u, float v, float us, float vs, float r, float g, float b);
			void add(const Math::Vector2D &pos, const Math::Vector2D &size, const Math::Vector2D &uv, const Math::Vector2D &uvs, const Math::Vector3D &color);
		};

		class TextureContainer {
		public:
			enum CONTAINER_TYPE {NORMAL, THREE_PATCH, NINE_PATCH};
			const CONTAINER_TYPE type;

			Math::Vector2D uv, uvs;

			TextureContainer(Math::Vector2D uv, Math::Vector2D uvs);

		protected:
			TextureContainer(Math::Vector2D uv, Math::Vector2D uvs, CONTAINER_TYPE type);
		};

		class ThreePatchContainer : public TextureContainer {
		public:
			Math::Vector2D split;
			bool direction;

			ThreePatchContainer(Math::Vector2D uv, Math::Vector2D uvs, Math::Vector2D split, bool direction);
		};

		class NinePatchContainer : public TextureContainer {
		public:
			Math::Vector2D h_split, v_split;

			NinePatchContainer(Math::Vector2D uv, Math::Vector2D uvs, Math::Vector2D h_split, Math::Vector2D v_split);
		};

		class TextureAtlas {
		public:
			std::map<std::string, std::shared_ptr<TextureContainer>> containers;

			RenderUtils::Texture *texture;

			TextureAtlas(const std::string &pack_file, RenderUtils::Texture *texture);

			TextureContainer *get(const std::string &name);
		};

		///////////////// Abstract Classes /////////////////
		class GUIElement {
		public:
			//properties
			Math::Vector2D position, size;
			Math::Vector3D color;

			//input
			bool pressed;
			float pressed_time;
			Math::Vector2D pressed_position;
			
			//events
			std::vector<std::function<bool(GUIElement *e)>> update_events, on_press_events;

			//transitions

			//texture
			TextureContainer *tex;
			float nine_u;

			//global container
			Window *window;

			GUIElement();
			
			GUIElement(const Math::Vector2D &size);
			
			GUIElement(const Math::Vector2D &position, const Math::Vector2D &size);

			virtual void set_color(float r, float g, float b);

			virtual void set_tex(TextureContainer *tex);
			
			//void add_transition( ... );

			virtual void set_window(Window *window);

			bool is_point_inside(const Math::Vector2D &point);

			virtual void on_press();

			virtual void on_release();

			virtual bool handle_input();

			virtual void update(float dt);

			virtual void add_to_batcher(GUIBatcher *base_batcher, GUIBatcher *text_batcher);

			virtual void add_to_batcher_base(GUIBatcher *base_batcher);
			virtual void add_to_batcher_3_patch_base(GUIBatcher *base_batcher);
			virtual void add_to_batcher_9_patch_base(GUIBatcher *base_batcher);
		};

		class Layout : public GUIElement {
		public:
			std::vector<std::shared_ptr<GUIElement>> elements;
			bool draw_base = false;

			float margin = 0.0f;

			void add(GUIElement * e);

			Layout();
			
			Layout(const Math::Vector2D &size);

			Layout(const Math::Vector2D &position, const Math::Vector2D &size);

			virtual void place_elements() = 0;
			
			virtual void calculate_size() = 0;
			
			virtual void set_window(Window *window);

			virtual bool handle_input();

			virtual void update(float dt);

			virtual void add_to_batcher(GUIBatcher *base_batcher, GUIBatcher *text_batcher);
		};

		///////////////// Containers /////////////////
		class VLayout : public Layout {
		public:
			VLayout();

			VLayout(const Math::Vector2D &size);

			VLayout(const Math::Vector2D &position, const Math::Vector2D &size);

			void place_elements();
			
			void calculate_size();
		};

		class HLayout : public Layout {
		public:
			HLayout();

			HLayout(const Math::Vector2D &size);

			HLayout(const Math::Vector2D &position, const Math::Vector2D &size);

			void place_elements();
			
			void calculate_size();
		};

		///////////////// Elements /////////////////
		class Label : public GUIElement {
		public:
			std::string text;
			enum TEXT_ALIGNMENT {LEFT, CENTER, RIGHT};

			TEXT_ALIGNMENT text_alignment = TEXT_ALIGNMENT::LEFT;

			bool editable = false;
			bool active = false;
			RenderUtils::Font *font;
			float text_width;

			float font_size;

			Math::Vector3D text_color;

			float cursor_rate = 1.0f, cursor_time = 0.0f;
			bool cursor_state = false;
			int cursor_location = 0;

			bool password = false;

			Label(const Math::Vector2D &size, RenderUtils::Font *font);
			Label(const Math::Vector2D &size, RenderUtils::Font *font, const std::string &text);

			void set_text(const std::string &text);

			void set_text_alignment(TEXT_ALIGNMENT text_alignment);

			void set_text_color(float r, float g, float b);

			void add_string(char c);
			void add_string(const std::string &text);

			void delete_text();

			void fit_to_text(const std::string &text);
			void fit_to_text();

			virtual void on_press();
			virtual void on_release();
			
			virtual void update(float dt);

			virtual void add_to_batcher(GUIBatcher *base_batcher, GUIBatcher *text_batcher);

			void add_text_to_batcher(GUIBatcher *base_batcher, GUIBatcher *text_batcher);
		};

		class Slider : public Label {
		public:
			float progress = 0.5f; //always in the range [0, 1]

			//additional color
			Math::Vector3D fill_color;

			//pointer texture
			TextureContainer *pointer_tex;

			Slider(const Math::Vector2D &size, RenderUtils::Font *font);

			void set_fill_color(float r, float g, float b);

			virtual void update(float dt);

			virtual void add_to_batcher(GUIBatcher *base_batcher, GUIBatcher *text_batcher);
			
			virtual void add_pointer_to_batcher(GUIBatcher *base_batcher, GUIBatcher *text_batcher);

			virtual void add_to_batcher_3_patch_base(GUIBatcher *base_batcher);
		};

		class ToggleButton : public Label {
		public:
			bool toggled = false;
			Math::Vector3D color_on, color_off;

			TextureContainer *tex_on, *tex_off;

			ToggleButton(const Math::Vector2D &size, RenderUtils::Font *font);

			void set_color(float r, float g, float b);
			void set_color_on(float r, float g, float b);
			void set_color_off(float r, float g, float b);

			virtual void set_tex(TextureContainer *tex);
			void set_tex_on(TextureContainer *tex_on);
			void set_tex_off(TextureContainer *tex_off);
			
			virtual void on_press();

			virtual void toggle();

			virtual void apply();
		};

		class Button : public ToggleButton {
		public:
			Button(const Math::Vector2D &size, RenderUtils::Font *font);

			virtual void on_release();
		};

		///////////////// Windows /////////////////
		class Window : public GUIElement {
		public:
			enum HEADER_TYPE {CARD, RECTANGLE};

			bool draw_base = true;

			TextureAtlas *texture_atlas;
			WindowManager *wm;
			int window_number;

			float margin = 0.002f;
			float row_margin = 0.002f;

			Math::Vector2D min_size;

			std::shared_ptr<Layout> main_layout;

			//pressed
			Math::Vector2D mouse_pressed_position;
			Math::Vector2D pressed_size;

			//header
			std::shared_ptr<Label> header;
			bool draw_header;

			bool resizable = false;

			void (*init)(Window * w, WindowManager *wm);

			Window(const Math::Vector2D &position, const Math::Vector2D &size, void (*f)(Window * w, WindowManager *wm));

			void add_header(HEADER_TYPE header_type, const std::string &text, float size, RenderUtils::Font *font);

			void set_window();

			bool handle_input();

			void set_pressed();

			void calculate_size();

			void update(float dt);

			void add_to_batcher(GUIBatcher *base_batcher, GUIBatcher *text_batcher);
		};

		class SimpleWindow : public Window  {
		private:
			HLayout *current_row;
		public:
			
			SimpleWindow(const Math::Vector2D &position, void (*f)(Window * w, WindowManager *wm));

			void add_element(GUIElement *e);

			void add_row();
		};

		class WindowManager {
		public:
			//assets
			RenderUtils::Shader *shader;
			std::shared_ptr<TextureAtlas> texture_atlas;
			RenderUtils::Font *font;

			//batchers
			GUIBatcher image_batcher;
			GUIBatcher base_batcher;
			GUIBatcher text_batcher;

			//camera
			Camera camera;

			//mouse
			Math::Vector2D mouse_position;
			Math::Vector2D pressed_mouse_position;

			//windows
			std::vector<std::shared_ptr<Window>> windows;
			std::vector<int> window_order;

			//bg
			bool draw_bg;
			RenderUtils::Texture *bg_texture;

			WindowManager();
			
			void init(TextureAtlas *texture_atlas, RenderUtils::Font *font, int base_batcher_size, int text_batcher_size);

			void add_window(Window *window);

			void set_bg(RenderUtils::Texture *texture);

			void bring_to_front(Window *window);

			bool update(float dt);

			void render();
		};
	}
}


#endif
