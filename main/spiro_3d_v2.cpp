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
static RenderUtils::Batcher b("spiro_batcher"), h("handle_batcher"), g("grid"), t("points");
static RenderUtils::Shader *shader;

//spirograph parts
static std::vector<std::tuple<Math::Vector3D, float>> spiro_structure;	//axis (magnitude = anglular frequency), length
static std::vector<Math::Matrix3D> spiro_rotation_matrices;
static std::vector<Math::Vector3D> spiro_handles;

//spirograph trace
static std::vector<float> spiro_points_buffer;
static std::vector<Math::Vector3D> spiro_points;

//program runtime
static float total_time = 0.001f;

//spirograph surfacef
static std::vector<Math::Vector3D> trimmed_points;
int steps;
float step_delta;
float point_r;

//TODO: spiro_structure to have axis, frequency, length

/*
calculates and adds the current position of the spirograph to spiro_points
*/
static void spiro_step() {
	using namespace CREngine::Math;

	/*spiro_handles.clear();
	Math::Vector3D origin;
	for (int i = 0; i < spiro_structure.size(); ++i) {
		spiro_handles.emplace_back(std::get<1>(spiro_structure[i]), 0.0f, 0.0f);
		origin += spiro_handles.back();
	}

	int m = spiro_structure.size();
	for (int i = m - 1; i >= 0; --i) {
		//Matrix3D rotation_matrix = Math::Matrix3D::rotation(std::get<0>(spiro_structure[i]) * total_time);
		Matrix3D rotation_matrix = Math::Matrix3D::rotation(std::get<0>(spiro_structure[i]) * total_time);
		for (int j = i; j < m; ++j) {
			spiro_handles[j] = rotation_matrix * spiro_handles[j];
		}
	}

	Math::Vector3D head;
	for (int i = 0; i < m; ++i)
		head += spiro_handles[i];

	spiro_points_buffer.push_back(head[0]);
	spiro_points_buffer.push_back(head[1]);
	spiro_points_buffer.push_back(head[2]);
	spiro_points_buffer.push_back(0.0f);
	spiro_points_buffer.push_back(0.0f);
	spiro_points_buffer.push_back(0.0f);

	spiro_points.push_back(Math::Vector3D(head[0], head[1], head[2]));*/

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

//quick sort
static int partition(Math::Vector3D **array, int low, int high, int (*comparator)(Math::Vector3D*, Math::Vector3D*)) {
	int i = low;
	int j = high + 1;
	Math::Vector3D *pivot = array[low];
	Math::Vector3D *temp;
	while (true) {
		//propegate forwards
		while(comparator(array[++i], pivot) <= 0) {
			if (i == high) {
				break;
			}
		}

		//propegate backwords
		while(comparator(array[--j], pivot) > 0) {
			if (j == low) {
				break;
			}
		}

		//exit
		if (i >= j)
			break;

		//switch elements
		temp = array[i];
		array[i] = array[j];
		array[j] = temp;
	}
	
	//place the pivot in its final place
	temp = array[low];
	array[low] = array[j];
	array[j] = temp;

	return j;
}

static void exchange(Math::Vector3D **array, int i, int j) {
	Math::Vector3D *temp = array[i];
	array[i] = array[j];
	array[j] = temp;
}

static void quick_sort(Math::Vector3D **array, int low, int high, int (*comparator)(Math::Vector3D*, Math::Vector3D*)) {
	/*for (int i = 0; i < high; ++i)
	{
		int min = i;
		for (int j = 0; j < high + 1; ++j)
		{
			if (comparator(array[min], array[j]) > 0) min = j;
		}
		Math::Vector3D *temp = array[i];
		array[i] = array[min];
		array[min] = temp;
	}
	return;*/

	/*if (low >= high) return;
	int index = partition(array, low, high, comparator);
	quick_sort(array, low, index - 1, comparator);
	quick_sort(array, index + 1, high, comparator);*/

	if (high <= low) return;
	int lt = low, i = low + 1, gt = high;
	Math::Vector3D *v = array[low];
	while (i <= gt) {
		int cmp = comparator(array[i], v);
		if
		(cmp < 0) exchange(array, lt++, i++);
		else if (cmp > 0) exchange(array, i, gt--);
		else
			i++;
	}
	quick_sort(array, low, lt - 1, comparator);
	quick_sort(array, gt + 1, high, comparator);
}



static bool temp_array[20000];



//Binary trees
class BinaryTree {
public:
	class BTNode;

	std::shared_ptr<BTNode> node_pool;
	BTNode *node_pool_alias;
	int node_pool_index = 0;

	BTNode *allocate_node() {
		return &node_pool_alias[node_pool_index++];
	}

	class BTNode {
	public:
		Math::Vector3D *ptr = nullptr;
		int index = -1;
		BTNode *left = nullptr, *right = nullptr, *parent = nullptr;
	};

	static void swap(BTNode *a, BTNode *b) {
		int temp = a->index;
		a->index = b->index;
		b->index = temp;

		Math::Vector3D *temp2 = a->ptr;
		a->ptr = b->ptr;
		b->ptr = temp2;
	}

	BTNode* create_node(Math::Vector3D **array, int low, int high, BTNode *parent) {
		if (low > high) return nullptr;

		int mid = (low + high) / 2;

		int duplicate_low = mid;
		int duplicate_high = mid;
		if (duplicate_low > 0) {
			while(comparator(array[duplicate_low], array[duplicate_low - 1]) == 0) {
				duplicate_low--;
				if (duplicate_low == 0) {
					break;
				}
			}
		}
		
		if (duplicate_high < high) {
			while(comparator(array[duplicate_high], array[duplicate_high + 1]) == 0) {
				duplicate_high++;
				if (duplicate_high == high) {
					break;
				}
			}
		}

		BTNode *node = allocate_node();

		BTNode *current = node;
		current->ptr = array[duplicate_high];
		current->index = duplicate_high;
		current->parent = parent;

		//TESTING
		temp_array[current->index] = true;
		for (int i = duplicate_high - 1; i >= duplicate_low; --i) {
			BTNode *next = allocate_node();
			current->left = next;

			next->parent = current;
			next->ptr = array[i];
			next->index = i;
			temp_array[next->index] = true;

			current = next;
		}

		current->left = create_node(array, low, duplicate_low - 1, current);
		//use node here since larger elements than current cannot be between two duplicates of current
		node->right = create_node(array, duplicate_high + 1, high, node);
		
		return node;
	}

	BTNode* find(Math::Vector3D *vec) {
		BTNode *current = head;
		while(current != nullptr) {
			if (current->ptr == vec) {
				return current;
			}
			if (comparator(current->ptr, vec) >= 0) {	//current->ptr >= vec
				current = current->left;
			} else
				current = current->right;
		}
		return nullptr;
	}


	/*
		returns the first item >= vec
	*/
	BTNode* find_min(Math::Vector3D vec) {
		BTNode *current = head;		//iterator
		BTNode *corner = nullptr;	//the vector closest to <= vec
		while(current != nullptr) {
			if (comparator(current->ptr, &vec) == 1) {	//current > vec
				corner = current;
				current = current->left;
			} else {									//current < vec
				current = current->right;
			}
		}
		return corner;
	}

	/*
		returns the first item <= vec
	*/
	BTNode* find_max(Math::Vector3D vec) {
		BTNode *current = head;		//iterator
		BTNode *corner = nullptr;	//the vector closest to <= vec
		while(current != nullptr) {
			if (comparator(current->ptr, &vec) == 1) {	//current > vec
				current = current->left;
			} else {									//current < vec
				corner = current;
				current = current->right;
			}
		}
		return corner;
	}

	BTNode* get_next_bigger(BTNode* node) {
		if (node->right != nullptr) {
			node = node->right;
			while(node->left != nullptr)
				node = node->left;
			return node;
		}

		BTNode* original = node;
		while(node->parent != nullptr) {
			node = node->parent;
			if (comparator(node->ptr, original->ptr) >= 0) return node;
		}
		return nullptr;
	}

	void remove_node(BTNode *node) {
		//special case
		if (node == head) {
			if (node->left == nullptr && node->right == nullptr) {
				head = nullptr;
				return;
			}
			if (node->left == nullptr && node->right != nullptr) {
				head = node->right;
				head = node->parent = nullptr;
				return;
			}
			if (node->left != nullptr && node->right == nullptr) {
				head = node->left;
				head = node->parent = nullptr;
				return;
			}
		}

		//node has no children
		if (node->left == nullptr && node->right == nullptr) {
			if (node->parent->left == node) node->parent->left = nullptr;
			else node->parent->right = nullptr;
			return;
		}

		//node has one child
		if (node->left == nullptr && node->right != nullptr) {
			//single node on the right

			BTNode *parent = node->parent, *child = node->right;
			if (parent->left == node) {
				parent->left = child;
				child->parent = parent;
			} else {
				parent->right = child;
				child->parent = parent;
			}
			return;
		}
		if (node->left != nullptr && node->right == nullptr) {
			//single node on the left
			BTNode *parent = node->parent, *child = node->left;
			if (parent->left == node) {
				parent->left = child;
				child->parent = parent;
			} else {
				parent->right = child;
				child->parent = parent;
			}
			return;
		}

		//node has two children
		BTNode *leftmost_right = node->right;		//leftmost element from the right
		while (leftmost_right->left != nullptr)
			leftmost_right = leftmost_right->left;
		swap(node, leftmost_right);
		remove_node(leftmost_right);
	}

	void remove(Math::Vector3D *vec) {
		remove_node(find(vec));
	}

	void print(std::string prefix, BTNode *node) {
		if (node == nullptr) return;
		std::cout<<prefix<<node->index<<"  "<<node->ptr<<"  ";
		node->ptr->print();
		this->print(prefix + "l ", node->left);
		this->print(prefix + "r ", node->right);
	}

	void print(std::string prefix) {
		print(prefix, head);
	}


	BTNode *head = nullptr;

	int (*comparator)(Math::Vector3D *, Math::Vector3D *);

	BinaryTree(std::vector<Math::Vector3D *> &array, int (*comparator)(Math::Vector3D *, Math::Vector3D *)) : comparator(comparator) {
		node_pool = std::shared_ptr<BTNode>(new BTNode[array.size()]);
		node_pool_alias = node_pool.get();		

		head = create_node(&array[0], 0, array.size() - 1, nullptr);
	}
};

//Hash maps - modified for counting duplicates
class HashMap {
private:
	int n;
public:
	struct HMNode {
		Math::Vector3D *ptr;
		int count;
	};

	std::vector<std::vector<HMNode>> map;
	std::vector<Math::Vector3D *> duplicates;
	
	HashMap(int n) : n(n) {
		reset();
	}

	void reset() {
		map.clear();		//reset the hash map
		map.resize(n);		//allocate and populate with empty vectors

		duplicates.clear();
	}

	void add(Math::Vector3D *ptr) {
		size_t hash = reinterpret_cast<size_t>(ptr);
		std::vector<HMNode> *htnode_vec = &map[hash % n];
		for (int i = (*htnode_vec).size() - 1; i >= 0; --i) {
			if ((*htnode_vec)[i].ptr == ptr) {
				(*htnode_vec)[i].count++;
				if ((*htnode_vec)[i].count == 2) {
					duplicates.push_back(ptr);
				}
			}
		}
		(*htnode_vec).push_back({ptr, 0});
	}
};

/*
combines points so no two points will be within point_r of each other.
controls trimmed_points
*/
static void trim_points() {
	//create three vectors: sorted spirograph's points by their x, y, z coordinates
	int n = spiro_points.size();
	std::vector<Math::Vector3D *> vec_x(n);
	std::vector<Math::Vector3D *> vec_y(n);
	std::vector<Math::Vector3D *> vec_z(n);
	for (int i = 0; i < n; ++i) {
		Math::Vector3D *vec = &spiro_points[i];
		vec_x[i] = vec;
		vec_y[i] = vec;
		vec_z[i] = vec;
	}

	//create point comparators
	auto x_comparator = [](Math::Vector3D *a, Math::Vector3D *b) {
		if ((*a)[0] == (*b)[0]) return 0;
		return (*a)[0] > (*b)[0] ? 1 : -1;
	};
	auto y_comparator = [](Math::Vector3D *a, Math::Vector3D *b) {
		if ((*a)[1] == (*b)[1]) return 0;
		return (*a)[1] > (*b)[1] ? 1 : -1;
	};
	auto z_comparator = [](Math::Vector3D *a, Math::Vector3D *b) {
		if ((*a)[2] == (*b)[2]) return 0;
		return (*a)[2] > (*b)[2] ? 1 : -1;
	};

	//sort these vectors (with no regard to duplicates)
	//quick_sort(&vec_x[0], 0, n - 1, x_comparator);
	//quick_sort(&vec_y[0], 0, n - 1, y_comparator);
	quick_sort(&vec_z[0], 0, n - 1, z_comparator);

	//create binary trees based on these arrays
	//BinaryTree tree_x(vec_x, x_comparator);
	//BinaryTree tree_y(vec_y, y_comparator);
	BinaryTree tree_z(vec_z, z_comparator);

	//find the binding box the average desnsity of the points
	Math::Vector3D center;
	Math::Vector3D bbox[2] = {Math::Vector3D((*vec_x[0])[0], (*vec_y[0])[1], (*vec_z[0])[2]), Math::Vector3D((*vec_x[n - 1])[0], (*vec_y[n - 1])[1], (*vec_z[n - 1])[2])};
	float bbox_volume = pow((bbox[1] - bbox[0]).length(), 2);
	//float r = 1.5f;
	float r = 0.10f;
	//float r = 0.15f;
	//float r = 0.20f;

	//modified HashMap for finding duplicates
	int hash_map_size = fmax(n / 100, 10);
	HashMap hash_map(hash_map_size);
	
	//combine all points
	int left = n - 1;
	//while(left > 0) {
	while(left > 10) {
		hash_map.reset();

		//pick a point
		//Math::Vector3D *point_ptr = tree_x.head->ptr;
		//Math::Vector3D point = *point_ptr;

		//create bounding box
		//Math::Vector3D point_min = point - Math::Vector3D(r, r, r);
		//Math::Vector3D point_max = point + Math::Vector3D(r, r, r);

		//point_min.print();
		//point_max.print();
		std::cout<<std::endl;

		//helper variables
		//BinaryTree::BTNode *min;
		//BinaryTree::BTNode *max;

		//points in x range
		/*min = tree_x.find_min(point_min);
		max = tree_x.find_max(point_max);
		while(min != max) {
			hash_map.add(min->ptr);
			min = tree_x.get_next_bigger(min);
		}
		hash_map.add(min->ptr);

		//points in y range
		min = tree_y.find_min(point_min);
		max = tree_y.find_max(point_max);
		while(min != max) {
			hash_map.add(min->ptr);
			//min = tree_y.find_min(*min->ptr);
			min = tree_y.get_next_bigger(min);
		}
		hash_map.add(min->ptr);

		//points in z range
		min = tree_z.find_min(point_min);
		max = tree_z.find_max(point_max);
		while(min != max) {
			hash_map.add(min->ptr);
			//min = tree_z.find_min(*min->ptr);
			min = tree_z.get_next_bigger(min);
		}
		hash_map.add(min->ptr);*/
		
		//tree_x.add_range_to_map(point_min, point_max); //TODO
		//tree_y.add_range_to_map(point_min, point_max);
		//tree_z.add_range_to_map(point_min, point_max);

		//filter the points outside the radius
		//std::cout<<tree_x.find(point_ptr)<<std::endl;
		//int removed = 0;
		//std::cout<<hash_map.duplicates.size()<<std::endl;
		/*for (int i = hash_map.duplicates.size() - 1; i >= 0; --i) {
			if ((*hash_map.duplicates[i] - point).length() <= r) {
				hash_map.duplicates[i]->print();
				//remove in range dots from future searches
				//std::cout<<hash_map.duplicates[i]<<std::endl;
				tree_x.remove(hash_map.duplicates[i]);
				tree_y.remove(hash_map.duplicates[i]);
				tree_z.remove(hash_map.duplicates[i]);

				removed++;
			}
		}*/
		std::cout<<"q"<<std::endl;

		for (int i = 0; i < n; ++i){
			std::cout<<i<<std::endl;
			tree_z.remove(&spiro_points[i]);
		}
		std::cout<<"q"<<std::endl;

		return;

		/*return;
		for (int i = 1500; i < spiro_points.size(); ++i) {
			//spiro_points[i].print();
			//tree_x.remove(&spiro_points[i]);
			//tree_y.remove(&spiro_points[i]);
			std::cout<<tree_z.find(&spiro_points[i])<<"  a"<<std::endl;
			std::cout<<i<<std::endl;
			tree_z.remove(&spiro_points[i]);
			std::cout<<tree_z.find(&spiro_points[i])<<std::endl;

			removed++;
		}

		left -= removed;

		return;

		std::cout<<removed<<std::endl;

		if (removed == 0) {
			//tree_x.remove(point_ptr);
			//tree_y.remove(point_ptr);
			tree_z.remove(point_ptr);
		}

		//add the point to the trimmed points and remove it
		//std::cout<<point_ptr<<std::endl;
		//trimmed_points.push_back(point);*/
	}
}

static void create_spirograph_surface() {
	trim_points();
	t.init(trimmed_points.size() * 6, std::vector<int> {3, 3});
	t.mode = GL_POINTS;
	t.clear();

	std::vector<float> data;
	for (int i = 0; i < trimmed_points.size(); ++i) {
		data.push_back(trimmed_points[i][0]);
		data.push_back(trimmed_points[i][1]);
		data.push_back(trimmed_points[i][2]);
		data.push_back(0.0f);
		data.push_back(0.5f);
		data.push_back(0.8f);
	}

	t.add_data(&data[0], data.size());
	t.update();
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
	shader = AssetManager::get_shader("spiro_3d");

	//define the spirograph
	//spiro_structure.emplace_back(std::vector<float> {0.8f, 2.0f});
	//spiro_structure.emplace_back(std::vector<float> {0.2f, 1.0f});
	//spiro_structure.emplace_back(std::vector<float> {0.2f, -3.5f});

	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.013f, 0.0f), 0.9f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.0f, 1.0f), 0.5f});

	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.1f, 0.0f), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 1.0f, 1.0f), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 0.0f, 1.0f), 0.5f});
	
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 0.1f, 0.0f), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 1.0f, 1.0f), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 0.0f, 1.0f), 0.5f});

	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.1f, 1.0f, 0.0f), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 1.0f, 1.0f), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 0.0f, 1.0f), 0.5f});

	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 0.0f, 1.0f).normalize() * 0.11f, 1.0f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.0f, 1.0f).normalize(), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 2.0f, 1.0f).normalize(), 0.5f});
	
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.01f, 0.0f), 0.0f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.01f, 0.0f), 1.0f});
	spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 0.0f, 1.0f), 1.0f});
	//steps = 20000;
	steps = 20000;
	step_delta = 0.1f;

	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 0.0f, 0.0f), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(1.0f, 1.0f, 0.0f), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, 1.0f, 1.0f), 0.5f});
	//spiro_structure.emplace_back(std::tuple<Math::Vector3D, float>{Math::Vector3D(0.0f, -1.0f, 1.0f), 0.5f});
	//steps = 50000;
	//step_delta = 0.1f;

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
	t.render();
	g.render();
}

void dispose() {

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
