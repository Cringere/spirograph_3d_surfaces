#include <GL/glew.h>

#include <iostream>
#include <tuple>

#include <CREngine/MainSpace.h>
#include <CREngine/AssetManager.h>
#include <CREngine/RenderUtils.h>
#include <CREngine/GUI.h>
#include <functional>
#include <CREngine/InputManager.h>

using namespace CREngine;

template<class T>
void print(T t) {
	std::cout<<t<<std::endl;
}

template<class T, class... Args>
void print(T t, Args ...args) {
	std::cout<<t<<" ";
	print(args...);
}

class Camera3D {
public:
	float n, f;						//near and far
	Math::Vector2D size;			//width and height
	Math::Vector3D position;		//location
	Math::Vector3D up, direction;	//orientation

	//calculatable objects
	Math::Vector3D right;
	Math::Matrix4D view, projection;	//the matrices that will go to the shader

	Camera3D() : n(0.1f), f(20.0f), size(3.0f, 3.0f), position(0.0f, 0.0f, 5.0f), up(0.0f, 1.0f, 0.0f), direction(0.0f, 0.0f, -1.0f) {}

	void look_at(const Math::Vector3D &target) {
		direction = (target - position).normalize();
	}

	void look_at(const Math::Vector3D &target, float distance) {
		direction = (target - position).normalize();
		position = target - direction * distance;
	}

	void update() {
		direction = direction.normalize();

		right = direction.cross(up).normalize();

		up = right.cross(direction).normalize();

		view = Math::Matrix4D::view(right, up, direction) * Math::Matrix4D::translate(position);
		projection = Math::Matrix4D::orthographic_projection(size, n, f);
	}
};

//camera and input
static Camera3D camera;
static Math::Vector2D mouse_p_frame, mouse_frame;
static Math::Vector2D camera_v;

//resources
static RenderUtils::Batcher b("spiro_batcher"), h("handle_batcher"), g("grid"), t("points"), s("surface");
static RenderUtils::Shader *shader, *surface_shader;

//spirograph parts
static std::vector<std::tuple<Math::Vector3D, float>> spiro_structure;	//axis (magnitude = anglular frequency), length
static std::vector<Math::Matrix3D> spiro_rotation_matrices;
static std::vector<Math::Vector3D> spiro_handles;

//spirograph trace
static std::vector<float> spiro_points_buffer;
static std::vector<Math::Vector3D> spiro_points;

//program runtime
static float total_time = 0.001f;

//spirograph surface
class Triangle;
class Vertex;
static std::vector<Math::Vector3D> trimmed_points;
static std::vector<Vertex> vertices_list;
static std::vector<Triangle *> spiro_surface_triangles;
static std::vector<Triangle *> active_triangles;
int steps;
float step_delta;
float point_r = 0.1f;			//minimum spacing between elements
float angle_cos_limit = 45.0f;	//filtering the direction of the tirangle surfaces


class Vertex : public Math::Vector3D {
public:
	std::vector<Vertex *> nearby_vertices;
	std::vector<Triangle *> connected_triangles;

	Vertex(const Math::Vector3D &position) : Vector3D(position) {}

	void remove_nearby_vertex(const Vertex *v) {
		for (int i = 0; i < nearby_vertices.size(); ++i) {
			if (nearby_vertices[i] == v) {
				nearby_vertices.erase(nearby_vertices.begin() + i);
				return;
			}
		}
	}

	bool connected_to_triangle(const Triangle *t) const {
		for (int i = 0; i < connected_triangles.size(); ++i)
		{
			if (connected_triangles[i] == t)
			{
				return true;
			}
		}
		return false;
	}

};

class Triangle {
public:
	Vertex *v[3];
	Triangle(Vertex *a, Vertex *b, Vertex *c) : v{a, b, c} {
		a->connected_triangles.push_back(this);
		b->connected_triangles.push_back(this);
		c->connected_triangles.push_back(this);
	}

	Math::Vector3D normal() const {
		return (*v[1] - *v[0]).cross(*v[2] - *v[1]).normalize();
	}

	Math::Vector3D center() const {
		return (*v[0] + *v[1] + *v[2]) * (1.0f / 3.0f);
	}

	void flip_vertex_order() {
		Vertex *temp = v[0];
		v[0] = v[1];
		v[1] = temp;
	}

	std::vector<Triangle *> build_aoround() {
		std::vector<Triangle *> list;

		//iterate over edges
		for (int i = 0; i < 3; ++i) {
			Vertex *a = v[i];
			Vertex *b = v[(i + 1) % 3];

			Vertex *c = nullptr;

			Math::Vector3D line_normal = (*b - *a).cross(normal()).normalize();
			Math::Vector3D line_center = (*b + *a) * 0.5f;

			float max_angle_cos = cos(angle_cos_limit * 3.141f / 180.0f);
			float min_distance = point_r * 10.0f;
			float distance = 0;
			for (int i_a = 0; i_a < a->nearby_vertices.size(); ++i_a) {
				Vertex *v = a->nearby_vertices[i_a];
				for (int i_b = 0; i_b < b->nearby_vertices.size(); ++i_b) {
					if (v == b->nearby_vertices[i_b]) {		//common nearby vertex for the edge
						//the angle cosine of the vector on the triangles plane and normal to the edge
						float angle_cos = (*v - line_center).angle_cos(line_normal);
						if (angle_cos > max_angle_cos) {	//checking the deviation of the line direction vector
							Math::Vector3D new_normal = (*v - *a).cross(*b - *v).normalize();	//the new normal of the new triangle
							for (Triangle *t2: a->connected_triangles) {	//keeping from binding to the same triangle
								for (Triangle *t3: b->connected_triangles) {
									if (t2 == t3 && t2 != this) {
										if (new_normal.angle_cos(normal()) > 0.0f) {	//filtering the lines outside the direction of the surface
											goto end;
										}
									}
								}
							}
							
							distance = line_center.distance_from(*v);
							if (distance < min_distance) {		//picking the closest point which passed the filters
								min_distance = distance;
								c = v;
							}

							end:;
						}
					}
				}
			}
			if (c != nullptr) {
				Triangle *triangle = new Triangle(a, c, b);
				list.push_back(triangle);
			}
		}
		return list;
	}
};


//debug herlpers
static std::vector<Math::Vector3D> extra_points;

/*
calculates and adds the current position of the spirograph to spiro_points
*/
static void spiro_step() {
	using namespace CREngine::Math;

	Math::Vector3D origin;
	for (int i = 0; i < spiro_structure.size(); ++i) {
		spiro_handles.emplace_back(std::get<1>(spiro_structure[i]), 0.0f, 0.0f);
		origin += spiro_handles.back();
	}

	Math::Vector3D head = origin;
	for (int i = spiro_structure.size() - 1; i >= 0; --i) {
		origin -= spiro_handles[i];
		Matrix3D rotation_matrix = Math::Matrix3D::rotation(std::get<0>(spiro_structure[i]) * total_time);

		head -= origin;
		head = rotation_matrix * head;
		head += origin;
	}

	spiro_points_buffer.push_back(head[0]);
	spiro_points_buffer.push_back(head[1]);
	spiro_points_buffer.push_back(head[2]);
	spiro_points_buffer.push_back(0.0f);
	spiro_points_buffer.push_back(0.0f);
	spiro_points_buffer.push_back(0.0f);

	spiro_points.push_back(Math::Vector3D(head[0], head[1], head[2]));
}

/*
uses spiro_step to create a full spirograph.
controls spiro_points
*/
static void create_spirograph() {
	for (int i = 0; i < steps; ++i) {
		spiro_step();
		total_time += step_delta;
	}
	b.clear();
	b.add_data(&spiro_points_buffer[0], spiro_points_buffer.size());
	b.update();
}

/*
combines points so no two points will be within point_r of each other.
controls trimmed_points
*/
static void trim_points() {
	int points_left = spiro_points.size();
	Math::Vector3D **points = new Math::Vector3D*[points_left];
	for (int i = 0; i < points_left; ++i) {
		points[i] = &spiro_points[i];
	}

	int start = 0, end;
	while(points_left > 0) {
		Math::Vector3D point = *points[0];
		while(points[start] == nullptr) start++;
		trimmed_points.push_back(*points[start]);

		end = points_left + start;

		int removed = 0;
		for (int i = start; i < end; ++i) {
			if (point.distance_from(*points[i]) < point_r) {
				removed++;
				points[i] = nullptr;
			}
		}


		for (int i = start, j = start; i < end; ++i) {
			if (points[i] == nullptr) continue;
			points[j++] = points[i];
		}

		points_left -= removed;
	}
}

static void create_surfrace() {
	//make a copy of (Vector3D) trimmed_points to (Vertex) vertices_list
	for (int i = 0; i < trimmed_points.size(); ++i) {
		vertices_list.emplace_back(trimmed_points[i]);
	}
	//calculate nerarby points
	for (int i = 0; i < vertices_list.size(); ++i) {
		Vertex *v = &vertices_list[i];
		for (int j = 0; j < vertices_list.size(); ++j) {
			if (j == i) continue;
			Vertex *w = &vertices_list[j];
			if (v->distance_from(*w) <= 3.5f * point_r) {
				v->nearby_vertices.push_back(w);
			}
		}
	}

	//calculate the gemetric center
	Math::Vector3D center;
	for (int i = 0; i < vertices_list.size(); ++i)
		center += vertices_list[i];
	center = center * (1.0f / vertices_list.size());

	//pick the point furthest away from the center
	float max_distance = vertices_list[0].distance_from(center);
	int max_distance_index = 0;
	for (int i = 1; i < vertices_list.size(); ++i){
		float d = vertices_list[i].distance_from(center);
		if (d > max_distance) {
			d = max_distance;
			max_distance_index = i;
		}
	}
	const Math::Vector3D &max_distance_vector = vertices_list[max_distance_index];

	//guess the normal for that point
	Math::Vector3D normal = (vertices_list[max_distance_index] - center).normalize();

	//find next point with minimal angle from max point
	int min_angle_index = 0;
	float min_angle = -1.0f;
	for (int i = 0; i < vertices_list.size(); ++i) {
		if (i == max_distance_index) continue;
		Math::Vector3D &v = vertices_list[i];
		if (v.distance_from(vertices_list[max_distance_index]) > 3.0f * point_r) continue; 

		float c = (vertices_list[i] - max_distance_vector).angle_cos(normal);
		
		//EDIT

		if (c > min_angle) {	//cosine bigger for smaller angles
			min_angle = c;
			min_angle_index = i;
		}
	}
	const Math::Vector3D &min_angle_vector = vertices_list[min_angle_index];

	//adjust the normal
	Math::Vector3D line_center = (max_distance_vector + min_angle_vector) * 0.5f;
	//normal = (line_center - center).normalize();

	//complete the triangle with a final point that will have minimum angle from the normal
	float min_normal = -1.0f;
	int min_normal_index = -1;
	for (int i = 0; i < vertices_list.size(); ++i) {
		if (i == max_distance_index || i == min_angle_index) continue;
		float c = (vertices_list[i] - center).angle_cos(normal);
		if (c >= min_normal) {	//cosine bigger for smaller angles
			min_normal = c;
			min_normal_index = i;
		}
	}

	//fix normal if needed
	Triangle *t = new Triangle( 
		&vertices_list[min_angle_index],
		&vertices_list[max_distance_index],
		&vertices_list[min_normal_index]
	);

	if ((t->center() - center).angle_cos(t->normal()) < 0.0f) {
		//t->flip_vertex_order();
	}

	//spiro_surface_triangles
	spiro_surface_triangles.push_back(t);

	active_triangles.push_back(t);
}

static void create_spirograph_surface() {
	//make a set of points which is easier to work with
	trim_points();

	//create a buffer for rendering the new set of points
	t.init(trimmed_points.size() * 6, std::vector<int> {3, 3});
	t.mode = GL_POINTS;
	t.clear();
	std::vector<float> data(trimmed_points.size() * 6);
	for (int i = 0; i < trimmed_points.size(); ++i) {
		data[i * 6 + 0] = trimmed_points[i][0];
		data[i * 6 + 1] = trimmed_points[i][1];
		data[i * 6 + 2] = trimmed_points[i][2];
		data[i * 6 + 3] = 0.0f;
		data[i * 6 + 4] = 0.5f;
		data[i * 6 + 5] = 0.8f;
	}
	t.add_data(&data[0], data.size());
	t.update();

	//create the surface buffer
	create_surfrace();
	s.init(trimmed_points.size() * 3 * (3 + 3 + 3), std::vector<int> {3, 3, 3});	//3 vectors per triangle, poisition, normal, color per vector
}

void init() {
	//create batchers
	b.init(100000, std::vector<int> {3, 3});
	b.mode = GL_LINE_STRIP;
	h.init(200, std::vector<int> {3, 3});
	h.mode = GL_LINE_STRIP;
	g.init(400, std::vector<int> {3, 3});
	g.mode = GL_LINES;

	//create the grid
	float l = 2.0f, dl = 0.5f;
	g.clear();
	{
		std::vector<float> data = {	//axis
			//x
			-l, 0.0f, 0.0f,		0.0f, 0.0f, 0.0f,
			l, 0.0f, 0.0f,		0.0f, 0.0f, 0.0f,
			//y
			0.0f, -l, 0.0f,		0.0f, 0.0f, 0.0f,
			0.0f, l, 0.0f,		0.0f, 0.0f, 0.0f,
			//y
			0.0f, 0.0f, -l,		0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, l,		0.0f, 0.0f, 0.0f,
		};
		g.add_data(&data[0], data.size());
	}
	float c_r = 0.4f, c_g = 0.4f, c_b = 0.4f;
	for (float i = -l; i <= l; i += dl) {
		//along x
		std::vector<float> data = {	//axis
			//x
			-l, 0.0f, i,		c_r, c_g, c_b,
			l, 0.0f, i,			c_r, c_g, c_b,
			//z
			i, 0.0f, -l,		c_r, c_g, c_b,
			i, 0.0f, l,			c_r, c_g, c_b
		};
		g.add_data(&data[0], data.size());
	}
	g.update();

	//load shader
	shader = AssetManager::get_shader("line_shader");
	surface_shader = AssetManager::get_shader("surface_shader");

	//define the spirograph
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.013f, 0.0f), 0.9f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.0f, 1.0f), 0.5f});
	//steps = 20000;
	//step_delta = 0.1f;

	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.1f, 0.0f), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 1.0f, 1.0f), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 0.0f, 1.0f), 0.5f});
	//steps = 20000;
	//step_delta = 0.1f;

	/*spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, -0.3f, 0.0f), 0.8f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, -0.3f, 1.0f), 0.5f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, -0.1f, -1.1f), 0.5f});
	steps = 20000;
	step_delta = 0.09f;
	point_r = 0.08f;*/

	//pick
	/*spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(-0.3f, 0.0f, 0.0f), 0.5f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, -0.3f, 0.0f), 0.5f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.0f, -0.31f), 0.5f});
	steps = 20000;
	step_delta = 0.09f;
	point_r = 0.08f;*/

	/*//pick - maybe
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(-0.7f, 0.1f, 0.1f), 1.0f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.0f, 1.0f), 1.5f});
	steps = 10000;
	step_delta = 0.09f;
	point_r = 0.02f;
	angle_cos_limit = 30.0f;*/

	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 0.1f, 0.0f), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 1.0f, 1.0f), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 0.0f, 1.0f), 0.5f});
	//steps = 20000;
	//step_delta = 0.1f;

	/*spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.0f, 0.1f), 0.5f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.1f, 0.0f), 1.5f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.1f, 0.1f, 0.1f), 1.5f});
	steps = 100000;
	step_delta = 0.1f;
	//point_r = 0.07f;*/

	/*spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.0f, 0.1f), 1.5f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.11f, 0.0f), 1.5f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.11f, 0.1f, 0.1f), 1.5f});
	//steps = 10000;
	steps = 100000;
	//steps = 100000;
	step_delta = 0.2f;
	//point_r = 0.07f;
	//point_r = 0.07f;*/

	/*//pick
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.1f, 0.0f, 0.1f), 0.7f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.1f, 0.0f, 0.0f), 1.0f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.0f, 0.1f), 1.0f});
	steps = 50000;
	step_delta = 0.1f;
	//point_r = 0.07f;*/

	/*//pick
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.1f, 0.0f, 0.0f), 1.0f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, -0.1f, 0.0f), 1.0f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(-0.1f, 0.0f, 0.0f), 1.0f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.117f, 0.0f), 0.2f});
	steps = 50000;
	step_delta = 0.1f;
	point_r = 0.09f;*/
	

	//pick
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.2f, 0.0f, 0.0f), 1.0f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, -0.3f, 0.0f), 1.0f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(-0.1f, 0.0f, 0.0f), 1.0f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.3f, 0.0f), 0.2f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.0117f, 0.0f), 0.2f});
	steps = 50000;
	step_delta = 0.1f;
	point_r = 0.09f;
	

	//pick
	/*spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.1f, 1.0f, 0.0f), 0.5f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 1.0f, 1.0f), 0.5f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 0.0f, 1.0f), 0.5f});
	steps = 20000;
	step_delta = 0.1f;
	point_r = 0.05f;*/

	/*spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.0f, -0.3f), 0.5f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, -0.3f, 1.0f), 0.5f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(-1.3f, -0.3f, 1.0f), 0.5f});
	steps = 20000;
	step_delta = 0.1f;
	point_r = 0.1f;*/

	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 0.0f, 1.0f).normalize() * 0.11f, 1.0f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.0f, 1.0f).normalize(), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 2.0f, 1.0f).normalize(), 0.5f});
	//steps = 20000;
	//step_delta = 0.1f;

	/*spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.01f, 0.0f), 0.0f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.0f, 1.0f), 1.5f});
	//steps = 20000;
	steps = 20000;
	step_delta = 0.1f;*/

	/*spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 0.0f, 0.0f), 0.5f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 1.0f, 0.0f), 0.5f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 1.0f, 1.0f), 0.5f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, -1.0f, 1.0f), 0.5f});
	//steps = 50000;
	steps = 10000;
	step_delta = 0.1f;
	point_r = 0.07f;*/

	/*spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 1.0f, 0.0f) * sqrt(2), 0.5f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, -1.0f, 0.0f), 0.5f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 1.0f, 1.0f), 0.5f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, -1.0f, 1.0f), 0.5f});
	//steps = 50000;
	steps = 10000;
	step_delta = 0.08f;
	point_r = 0.07f;
	angle_cos_limit = 30.0f;*/

	//create the spirograph
	create_spirograph();
	create_spirograph_surface();
}

void handle_input() {
	mouse_p_frame = mouse_frame;
	mouse_frame = InputManager::mouse_position;

	if (InputManager::keys[InputManager::KEYS::KEY_SPACE] == InputManager::JUST_PRESSED || InputManager::keys[InputManager::KEYS::KEY_SPACE] == InputManager::DOWN) {
		//step forward
		std::vector<Triangle *> new_active;
		for (int i = 0; i < active_triangles.size(); ++i) {
			std::vector<Triangle *> list = active_triangles[i]->build_aoround();
			for (int j = 0; j < list.size(); ++j) {
				new_active.push_back(list[j]);
				spiro_surface_triangles.push_back(list[j]);
			}
		}
		active_triangles = new_active;

		s.clear();
		std::vector<float> surface_data(trimmed_points.size() * 1000 * 3 * (3 + 3 + 3));	//3 vectors per triangle, poisition, normal, color per vector
		Math::Vector3D color(0.1f, 0.7f, 0.3f);
		int count = 0;
		for (int i = 0, m = spiro_surface_triangles.size(); i < m; ++i) {
			const Triangle &t = *spiro_surface_triangles[i];
			Math::Vector3D normal = t.normal();
			for (int j = 0; j < 3; ++j) {
				Math::Vector3D &position = *t.v[j];
				/*Math::Vector3D normal;
				for (Triangle *tr: t.v[j]->connected_triangles)
					normal += tr->normal();*/
				normal = normal.normalize();
				surface_data[count++] = position[0];
				surface_data[count++] = position[1];
				surface_data[count++] = position[2];
				surface_data[count++] = normal[0];
				surface_data[count++] = normal[1];
				surface_data[count++] = normal[2];
				surface_data[count++] = color[0];
				surface_data[count++] = color[1];
				surface_data[count++] = color[2];
			}
		}
		s.add_data(&surface_data[0], count);
		s.update();
	}
}

void update(float dt) {
	using namespace InputManager;

	//mouse_movement
	if (InputManager::mouse_keys[0] == InputManager::KEY_STATE::DOWN) {
		Math::Vector2D s(MainSpace::get_width() / 3.141f, MainSpace::get_height() / 3.141f);
		Math::Vector2D d = (mouse_frame - mouse_p_frame);
		camera_v += d * s * dt * 2.0f;
	}

	if (InputManager::mouse_keys[1] == InputManager::KEY_STATE::DOWN) {
	}

	if (InputManager::keys[InputManager::KEYS::KEY_W] == InputManager::KEY_STATE::DOWN) {
		camera.size *= (1.0f - dt * 0.3f);
	}
	if (InputManager::keys[InputManager::KEYS::KEY_S] == InputManager::KEY_STATE::DOWN) {
		camera.size *= (1.0f + dt * 0.3f);
	}

	camera.position -= camera.right * camera_v[0];
	camera.position -= camera.up * camera_v[1];
	camera_v *= 0.7f;

	camera.look_at(Math::Vector3D(0.0f, 0.0f, 0.0f), 3.0f);
	camera.update();
	//camera.position.print();
}

void render() {
	glClearColor(0.8, 0.8, 0.8, 1.0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shader->use();
	shader->bind_mat("camera_view", camera.view);
	shader->bind_mat("camera_projection", camera.projection);
	
	//g.render();
	//b.render();
	t.render();

	surface_shader->use();
	/*camera.view[0].print();
	camera.view[1].print();
	camera.view[2].print();
	camera.view[3].print();
	std::cout<<std::endl;*/
	surface_shader->bind_mat("camera_view", camera.view);
	surface_shader->bind_mat("camera_projection", camera.projection);
	s.render();
}

void dispose() {
	for (int i = 0; i < spiro_surface_triangles.size(); ++i) 
		delete spiro_surface_triangles[i];
}

int main(int argc, char const *argv[]) {
	CREngine::MainSpace::set_main_function_pointers(
		&init,
		&handle_input,
		&update,
		&render,
		&dispose
	);

	CREngine::MainSpace::Configuration conf("spiro_3d", 800, 800, 60);

	if (CREngine::MainSpace::run(conf) != 0) return -1;

	return 0;
}
