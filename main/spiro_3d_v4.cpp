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
static RenderUtils::Batcher b("spiro_batcher"), h("handle_batcher"), g("grid"), t("points"), s("surface"), ep("extra");
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
float point_r = 0.05f;		//minimum spacing between elements

//debug herlpers
static std::vector<Math::Vector3D> extra_points;

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
			extra_points.push_back(line_center);

			float max_angle_cos = 0.2f;
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
							
							extra_points.push_back(*v);
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

//herlper comparator fucntions
static int x_comparator(const Math::Vector3D *a, const Math::Vector3D *b) {
	if ((*a)[0] == (*b)[0]) return 0;
	return (*a)[0] > (*b)[0] ? 1 : -1;
}
static int y_comparator(const Math::Vector3D *a, const Math::Vector3D *b) {
	if ((*a)[1] == (*b)[1]) return 0;
	return (*a)[1] > (*b)[1] ? 1 : -1;
}
static int z_comparator(const Math::Vector3D *a, const Math::Vector3D *b) {
	if ((*a)[2] == (*b)[2]) return 0;
	return (*a)[2] > (*b)[2] ? 1 : -1;
}

typedef int (*Comparator)(const Math::Vector3D *, const Math::Vector3D *);

//array functions
static void exchange(const Math::Vector3D **array, int i, int j) {
	const Math::Vector3D *temp = array[i];
	array[i] = array[j];
	array[j] = temp;
}

static void quick_sort(const Math::Vector3D **array, Comparator c, int low, int high) {
	if (high <= low) return;
	int lt = low, i = low + 1, gt = high;
	const Math::Vector3D *v = array[low];
	while (i <= gt) {
		int cmp = c(array[i], v);
		if (cmp < 0) exchange(array, lt++, i++);
		else if (cmp > 0) exchange(array, i, gt--);
		else i++;
	}
	quick_sort(array, c, low, lt - 1);
	quick_sort(array, c, gt + 1, high);
}

static void quick_sort(std::vector<const Math::Vector3D *> &array, Comparator c) {
	quick_sort(&array[0], c, 0, array.size() - 1);
}

//modiefied hashmap for finding duplicates
class Duplicates {
private:
	struct Node {
		const Math::Vector3D *value = nullptr;
		int count = 1;
		Node *next = nullptr;
	};

	std::vector<Node> nodes_resource;
	std::vector<Node *> head_nodes;
	//Node **head_nodes;
	int nodes_index = 0, n, max;
	
	Node *allocate_node(const Math::Vector3D *v) {
		if (nodes_index > max) {
			std::cout<<"ERROR: not enough space in hash map"<<std::endl;
		}
		Node *node = &nodes_resource[nodes_index++];
		node->value = v;
		node->count = 1;
		node->next = nullptr;
		return node;
	}

public:
	std::vector<const Math::Vector3D *> duplicates;

	Duplicates(int n, int max) : n(n), max(max), nodes_resource(max), head_nodes(n, nullptr) {
		nodes_index = 0;
		clear();
	}

	void clear() {
		nodes_index = 0;
		for (int i = 0; i < n; ++i)
			head_nodes[i] = nullptr;
		duplicates.clear();
	}

	void add(const Math::Vector3D *v) {
		int hash = ((unsigned int) (reinterpret_cast<size_t>(v))) % n;

		if (head_nodes[hash] == nullptr) {
			head_nodes[hash] = allocate_node(v);
			return;
		}

		Node *parent;
		Node *current = head_nodes[hash];
		while (current != nullptr) {
			if (current->value == v) {
				current->count++;
				if (current->count == 3)
					duplicates.push_back(v);
				return;
			}
			parent = current;
			current = parent->next;
		}
		parent->next = allocate_node(v);
	}
};

//binary search tree
class BinaryTree {
private:
	struct Node {
		int index = -1;
		Node *left = nullptr, *right = nullptr;
		Node *duplicate = nullptr;
	};

	Node *head = nullptr;
	std::vector<const Math::Vector3D *> array;
	std::vector<Node> nodes_resource;
	int node_allocation_current = 0;
	Comparator comparator;

	Node* allocate_node(int index) {
		Node *node = &nodes_resource[node_allocation_current++];
		node->index = index;
		node->left = node->right = node->duplicate = nullptr;
		return node;
	}

	void swap_values(Node *a, Node *b) {
		int temp = a->index;
		a->index = b->index;
		b->index = temp;

		Node *temp2 = a->duplicate;
		a->duplicate = b->duplicate;
		b->duplicate = temp2;
	}

	Node *create_sub_tree(int low, int high) {
		if (low > high) return nullptr;
		int duplicate_low, duplicate_high;
		duplicate_low = duplicate_high = (low + high) / 2;

		//find duplicate with lowest index
		while (true) {
			if (duplicate_low == low) break;
			if (comparator(array[duplicate_low], array[duplicate_low - 1]) == 0)
				duplicate_low--;
			else
				break;
		}

		//find duplicate with highest index
		while (true) {
			if (duplicate_high == high) break;
			if (comparator(array[duplicate_high], array[duplicate_high + 1]) == 0)
				duplicate_high++;
			else
				break;
		}

		Node *current = allocate_node(duplicate_high);
		Node *origin = current;
		for (int i = duplicate_high - 1; i >= duplicate_low; i--) {
			current->duplicate = allocate_node(i);
			current = current->duplicate;
		}
		origin->left = create_sub_tree(low, duplicate_low - 1);
		origin->right = create_sub_tree(duplicate_high + 1, high);

		return origin;
	}

	Node *find_and_delete_rec(Node *current, const Math::Vector3D *v) {
		if (current == nullptr) return nullptr;

		int c = comparator(array[current->index], v);
		if (c > 0)
			current->left = find_and_delete_rec(current->left, v);
		else if (c < 0)
			current->right = find_and_delete_rec(current->right, v);
		else {
			if (array[current->index] != v) {				//the target is a duplicate
				Node *origin = current;
				while (current->duplicate != nullptr) {
					if (array[current->duplicate->index] == v) {
						current->duplicate = current->duplicate->duplicate;
						return origin;
					}
					current = current->duplicate;
				}
				return origin;
			} else {										//the target is not a duplicate
				if (current->duplicate != nullptr) {			//the target is the head of the duplicates
					current->duplicate->left = current->left;
					current->duplicate->right = current->right;
					return current->duplicate;
				} else {									//the target has no duplicates
					//no children
					if (current->left == nullptr && current->right == nullptr)
						return nullptr;

					//single child or no children
					if (current->left == nullptr) {				//if there is a child, its on the right
						return current->right;
					} else if (current->right == nullptr) {		//if there is a child, its on the left
						return current->left;
					} else {									//both children exist
						//find next smallest element
						Node *min = current->right;
						while (min->left != nullptr) min = min->left;
						swap_values(current, min);
						current->right = find_and_delete_rec(current->right, v);
						return current;
					}
				}
			}
		}

		return current;
	}

	void get_in_range_rec(Duplicates &duplicates, Node *current, const Math::Vector3D &min, const Math::Vector3D &max) {
		if (current == nullptr) return;

		//calculate positioning
		int c1 = comparator(array[current->index], &min);
		int c2 = comparator(array[current->index], &max);

		if (c1 >= 0 && c2 <= 0) {    //node is in range
			//add kids to list
			get_in_range_rec(duplicates, current->left, min, max);
			get_in_range_rec(duplicates, current->right, min, max);

			//add this and all duplicates to duplicates
			do {
				duplicates.add(array[current->index]);
				current = current->duplicate;
			} while (current != nullptr);
			return;
		}

		if (c1 < 0) {    //node is left of range
			get_in_range_rec(duplicates, current->right, min, max);
		}

		if (c2 > 0) {    //node is right of range
			get_in_range_rec(duplicates, current->left, min, max);
		}
	}

public:
	BinaryTree(const std::vector<Math::Vector3D> &source_array, Comparator comparator) : comparator(comparator), array(source_array.size()), nodes_resource(source_array.size()) {
		//create a copy of the original array - pointer copy
		for (int i = 0; i < source_array.size(); i++) {
			//array.push_back(&source_array[i]);
			array[i] = &source_array[i];
		}

		//sort the array
		quick_sort(array, comparator);

		//create the binary tree
		head = create_sub_tree(0, this->array.size() - 1);
	}

	Node *find(const Math::Vector3D *v) {
		Node *current = head;
		while (current != nullptr) {
			int c = comparator(array[current->index], v);
			if (c > 0)										//current > v
				current = current->left;
			else if (c < 0)									//current < v
				current = current->right;
			else {											//current = v
				while (current != nullptr) {
					if (array[current->index] == v) {
						return current;
					}
					current = current->duplicate;
				}
				return nullptr;
			}

		}
		return nullptr;
	}

	void find_and_delete(const Math::Vector3D *v) {
		head = find_and_delete_rec(head, v);
	}

	/*
	adds to list all the vectors whose value is between min and max
	*/
	void get_in_range(Duplicates &duplicates, const Math::Vector3D &min, Math::Vector3D &max) {
		if (comparator(&max, &min) < 0) {
			std::cout<<"max has to be >= min"<<std::endl;
			return;
		}
		get_in_range_rec(duplicates, head, min, max);
	}

	const Math::Vector3D *get_head() {
		return array[head->index];
	}
};

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

	return ;
	
	//create the binary search trees
	int n = spiro_points.size();

	//create binary trees based on these arrays
	BinaryTree tree_x(spiro_points, &x_comparator);
	BinaryTree tree_y(spiro_points, &y_comparator);
	BinaryTree tree_z(spiro_points, &z_comparator);

	//Creatae a map for finding duplicates
	Duplicates duplicates((int) (n * 0.75) * 2, n * 2);
	
	//combine all points
	int left = n;
	while(left > 0) {
		//pick a random point, and add it to the new points array
		Math::Vector3D point(*tree_x.get_head());	//we only need the value, not the pointer, so we make a copy
		trimmed_points.push_back(point);

		//create the search limits
		Math::Vector3D vector_min = Math::Vector3D(point[0] - point_r, point[1] - point_r, point[2] - point_r);
		Math::Vector3D vector_max = Math::Vector3D(point[0] + point_r, point[1] + point_r, point[2] + point_r);

		//reset the duplicates map
		duplicates.clear();

		//filter the points based on the search limit and add them to the duplicates map
		tree_x.get_in_range(duplicates, vector_min, vector_max);
		tree_y.get_in_range(duplicates, vector_min, vector_max);
		tree_z.get_in_range(duplicates, vector_min, vector_max);

		//final filter - radius checking
		for (const Math::Vector3D *v: duplicates.duplicates) {
			if ((*v - point).length() <= point_r) {
				tree_x.find_and_delete(v);
				tree_y.find_and_delete(v);
				tree_z.find_and_delete(v);
				left--;
			}
		}
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

	//pick the poin furthest away from the center
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
	int min_angle_index = -1;
	float min_angle = -1.0f;
	for (int i = 0; i < vertices_list.size(); ++i) {
		if (i == max_distance_index) continue;
		float c = (vertices_list[i] - max_distance_vector).normalize().dot(normal);
		if (c > min_angle) {	//cosine bigger for smaller angles
			min_angle = c;
			min_angle_index = i;
		}
	}
	const Math::Vector3D &min_angle_vector = vertices_list[min_angle_index];

	//adjust the normal
	Math::Vector3D line_center = (max_distance_vector + min_angle_vector) * 0.5f;
	normal = (line_center - center).normalize();

	//complete the triangle with a final point that will have minimum angle from the normal
	float min_normal = -1.0f;
	int min_normal_index = -1;
	for (int i = 0; i < vertices_list.size(); ++i) {
		if (i == max_distance_index || i == min_angle_index) continue;
		float c = (vertices_list[i] - center).angle_cos(normal);
		if (c > min_normal) {	//cosine bigger for smaller angles
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
		t->flip_vertex_order();
	}

	//spiro_surface_triangles
	spiro_surface_triangles.push_back(t);

	active_triangles.push_back(t);

	//recursively build the rest of the surface
	//spiro_surface_triangles[0]->recursively_build();

	//expand recursively
	/*Triangle *current = &spiro_surface_triangles[0];
	for (int n = 0; n < 2; ++n) {
		const Math::Vector3D &a = *current->v[0];
		const Math::Vector3D &b = *current->v[1];
		Math::Vector3D line_center = (a + b) * 0.5f;
		Math::Vector3D line_dir = (line_center - current->center()).normalize();
		Math::Vector3D normal = current->normal();

		int index = -1;
		float min = -1.0f;
		for (int i = 0; i < trimmed_points.size(); ++i) {
			if (i == max_distance_index || i == min_angle_index || i == min_normal_index) continue;
			if (line_center.distance_from(trimmed_points[i]) > 2.0f * point_r) continue;
			Math::Vector3D &p = trimmed_points[i];
			if (line_dir.dot(p - a) < 0) continue;
			//float c = ((p - a).cross(line_dir)).angle_cos(normal);
			float c = ((p - a).dot(line_dir));
			if (c > min) {
				min = c;
				index = i;
				std::cout<<c<<std::endl;
			}
		}
		std::cout<<index<<std::endl;
		spiro_surface_triangles.emplace_back(
			current->v[0],
			current->v[1],
			&trimmed_points[index]
		);
	}*/
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
	ep.init(100000 * 6, std::vector<int> {3, 3});
	ep.mode = GL_POINTS;

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
	shader = AssetManager::get_shader("spiro_3d");
	surface_shader = AssetManager::get_shader("spiro_3d_surface");

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

	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 0.1f, 0.0f), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 1.0f, 1.0f), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 0.0f, 1.0f), 0.5f});
	//steps = 20000;
	//step_delta = 0.1f;

	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.1f, 1.0f, 0.0f), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 1.0f, 1.0f), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 0.0f, 1.0f), 0.5f});
	//steps = 20000;
	//step_delta = 0.1f;

	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 0.0f, 1.0f).normalize() * 0.11f, 1.0f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.0f, 1.0f).normalize(), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 2.0f, 1.0f).normalize(), 0.5f});
	//steps = 20000;
	//step_delta = 0.1f;

	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.01f, 0.0f), 0.0f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.0f, 1.0f), 1.5f});
	////steps = 20000;
	//steps = 20000;
	//step_delta = 0.1f;

	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 0.0f, 0.0f), 0.5f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 1.0f, 0.0f), 0.5f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 1.0f, 1.0f), 0.5f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, -1.0f, 1.0f), 0.5f});
	//steps = 50000;
	steps = 10000;
	step_delta = 0.1f;

	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 1.0f, 0.0f), 0.5f});

	//create the spirograph's parts
	/*
	for (int i = 0; i < spiro_structure.size(); ++i) {
		spiro_handles.emplace_back(Math::Vector2D(spiro_structure[i][0], 0.0f));
		spiro_rotation_matrices.push_back(Math::Matrix2D::rotation(spiro_structure[i][1] * 0.01f));
	}
	*/

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

		//extra points buffer
		ep.clear();
		std::vector<float> data(extra_points.size() * 6);
		for (int i = 0; i < extra_points.size(); ++i) {
			data[i * 6 + 0] = extra_points[i][0];
			data[i * 6 + 1] = extra_points[i][1];
			data[i * 6 + 2] = extra_points[i][2];
			data[i * 6 + 3] = 0.8f;
			data[i * 6 + 4] = 0.5f;
			data[i * 6 + 5] = 0.0f;
		}
		ep.add_data(&data[0], data.size());
		ep.update();
	}
}

void update(float dt) {
	//update the spirograph
	/*int n = 10;
	float s = 8.0f;
	for (int i = 0; i < n; ++i) {
		total_time += s * dt;
		spiro_step();
	}
	

	//update the spirograph's visual data
	b.clear();
	b.add_data(&spiro_points[0], spiro_points.size());
	b.update();*/




	/*//update the handles
	h.clear();
	Math::Vector3D trace;
	std::vector<float> data = {trace[0], trace[1], trace[2], 0.0f, 0.0f, 0.0f};
	h.add_data(&data[0], data.size());
	for (int i = 0; i < spiro_structure.size(); ++i) {
		trace += spiro_handles[i];
		std::vector<float> data = {trace[0], trace[1], trace[2], 0.0f, 0.0f, 0.0f};
		h.add_data(&data[0], data.size());
	}
	h.update();*/

	
	using namespace InputManager;
	/*
	//keybaord movement
	float r = 0.0f, u = 0.0f;
	if (keys[KEYS::KEY_D] == KEY_STATE::DOWN) {
		r += 1.0f;
	}
	if (keys[KEYS::KEY_A] == KEY_STATE::DOWN) {
		r -= 1.0f;
	}
	if (keys[KEYS::KEY_W] == KEY_STATE::DOWN) {
		u += 1.0f;
	}
	if (keys[KEYS::KEY_S] == KEY_STATE::DOWN) {
		u -= 1.0f;
	}
	float s = 5.0f;
	camera.position += camera.right * dt * s * r;
	camera.position += camera.up * dt * s * u;*/

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
	
	b.render();
	//ep.render();
	g.render();
	t.render();

	surface_shader->use();
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

	CREngine::MainSpace::Configuration conf("spiro_2d", 800, 800, 60);

	if (CREngine::MainSpace::run(conf) != 0) return -1;

	return 0;
}
