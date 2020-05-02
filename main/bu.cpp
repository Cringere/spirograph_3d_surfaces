
//modified binary serach - return first element >= min
static int binary_search_min(std::vector<Math::Vector3D *> &array, Math::Vector3D target, int (*comparator)(Math::Vector3D*, Math::Vector3D*)) {
	int low = 0;
	int high = array.size() - 1;
	int mid;
	while(low < high) {
		mid = (low + high) / 2;
		if (comparator(array[mid], &target) < 0) {
			low = mid + 1;
		} else {
			high = mid;
		}
	}

	return low;
}
//modified binary serach - return first element <= max
static int binary_search_max(std::vector<Math::Vector3D *> &array, Math::Vector3D target, int (*comparator)(Math::Vector3D*, Math::Vector3D*)) {
	int low = 0;
	int high = array.size() - 1;
	int mid;
	while(low < high) {
		mid = (low + high) / 2;
		if (comparator(array[mid], &target) > 0) {
			high = mid - 1;
		} else {
			low = mid + 1;
		}
	}
	return high;
}



//helpr class for memory management
class MemoryPool {
public:
	struct Tag {
		int size;	//size of data after tag
		bool used;	//if the data after the tag free or not
	};
	const static int TAG_SIZE = sizeof(Tag);

	std::shared_ptr<unsigned char> pool;	//the memory pool
	int pool_size;							//size of memory pool (in bytes)
	unsigned char *ptr;						//useful alias for pool

	void reserve(int items, int item_size) {
		//allocate memory
		pool = std::shared_ptr<unsigned char>(new unsigned char[items * item_size * TAG_SIZE]);
		ptr = pool.get();

		//place the first and last tags
		Tag *first = (Tag *) ptr[0];
		Tag *last = (Tag *) ptr[pool_size - TAG_SIZE];
		first->size = pool_size - 2 * TAG_SIZE;
		first->used = false;
		last->size = 0;
		last->used = false;
	}

	template<class T>
	T* allocate() {
		int size = sizeof(T);
		Tag *current = (Tag *) ptr[0];
		while (true) {
			if (!current->used) {
				if (current->size >= size + TAG_SIZE) {
					int o_size = current->size;
					int new_size = o_size - size - TAG_SIZE;

					Tag *next = (Tag *)((unsigned char)current + size + TAG_SIZE);
					next->size = new_size;
					next->used = true;

					current->size = size;
					current->used = true;

					unsigned char alloc_ptr = (unsigned char)(current);
					return (T *) alloc_ptr;
				}
			}
		}
	}
};



//modified binary serach - return first element >= min
static int binary_search_min(std::vector<Math::Vector3D *> &array, Math::Vector3D target, int (*comparator)(Math::Vector3D*, Math::Vector3D*)) {
	int low = 0;
	int high = array.size() - 1;
	int mid;
	while(low < high) {
		mid = (low + high) / 2;
		if (comparator(array[mid], &target) < 0) {
			low = mid + 1;
		} else {
			high = mid;
		}
	}

	return low;
}
//modified binary serach - return first element <= max
static int binary_search_max(std::vector<Math::Vector3D *> &array, Math::Vector3D target, int (*comparator)(Math::Vector3D*, Math::Vector3D*)) {
	int low = 0;
	int high = array.size() - 1;
	int mid;
	while(low < high) {
		mid = (low + high) / 2;
		if (comparator(array[mid], &target) > 0) {
			high = mid - 1;
		} else {
			low = mid + 1;
		}
	}
	return high;
}




int main(int argc, char const *argv[]) {
	auto X = [](Math::Vector3D *a, Math::Vector3D *b) {
		if ((*a)[0] == (*b)[0]) return 0;
		return (*a)[0] > (*b)[0] ? 1 : -1;
	};

	//sort these vectors (with no regard to duplicates)
	std::vector<Math::Vector3D *> vec;
	vec.push_back(new Math::Vector3D(0.0f, 0.0f, 0.0f));
	vec.push_back(new Math::Vector3D(0.5f, 0.0f, 0.0f));
	vec.push_back(new Math::Vector3D(1.0f, 0.0f, 0.0f));
	vec.push_back(new Math::Vector3D(2.0f, 0.0f, 0.0f));
	vec.push_back(new Math::Vector3D(2.0f, 0.0f, 0.0f));
	vec.push_back(new Math::Vector3D(2.0f, 0.0f, 0.0f));
	vec.push_back(new Math::Vector3D(2.0f, 0.0f, 0.0f));
	vec.push_back(new Math::Vector3D(2.0f, 0.0f, 0.0f));
	vec.push_back(new Math::Vector3D(4.0f, 0.0f, 0.0f));
	vec.push_back(new Math::Vector3D(4.0f, 0.0f, 0.0f));
	vec.push_back(new Math::Vector3D(5.0f, 0.0f, 0.0f));
	vec.push_back(new Math::Vector3D(5.0f, 0.0f, 0.0f));
	vec.push_back(new Math::Vector3D(6.0f, 0.0f, 0.0f));
	vec.push_back(new Math::Vector3D(6.0f, 0.0f, 0.0f));
	vec.push_back(new Math::Vector3D(7.0f, 0.0f, 0.0f));
	vec.push_back(new Math::Vector3D(7.0f, 0.0f, 0.0f));
	vec.push_back(new Math::Vector3D(7.5f, 0.0f, 0.0f));
	vec.push_back(new Math::Vector3D(8.0f, 0.0f, 0.0f));
	vec.push_back(new Math::Vector3D(8.5f, 0.0f, 0.0f));
	vec.push_back(new Math::Vector3D(8.5f, 0.0f, 0.0f));
	vec.push_back(new Math::Vector3D(8.5f, 0.0f, 0.0f));
	vec.push_back(new Math::Vector3D(8.5f, 0.0f, 0.0f));
	vec.push_back(new Math::Vector3D(8.5f, 0.0f, 0.0f));
	vec.push_back(new Math::Vector3D(9.4f, 0.0f, 0.0f));
	vec.push_back(new Math::Vector3D(9.0f, 0.0f, 0.0f));

	//tree.remove(vec[4]);
	//tree.remove(vec[4]);
	//tree.remove(vec[3]);
	//tree.remove(vec[7]);
	//tree.find(vec[1])->ptr->print();
	//tree.find(vec[5])->ptr->print();
	//tree.find(vec[8])->ptr->print();
	//tree.find(tree.head->ptr)->ptr->print();

	quick_sort(&vec[0], 0, vec.size() - 1, X);

	//create binary trees based on these arrays
	BinaryTree tree(vec, X);

	for (int i = vec.size() - 1; i >= 0; --i) {
		tree.remove(vec[i]);
	}

	for (int i = 0; i < vec.size(); ++i) {
		if (tree.find(vec[i]) != nullptr)
			print(i, "not null");
	}


	//print(tree.find(vec[6])->index);
	//print(tree.find(vec[6])->parent->index);
	//print(tree.find(tree.head->ptr)->index);
	//print(tree.find(tree.head->ptr)->right->index);
	//print(tree.find(tree.head->ptr)->left->index);

	//return 0;
	//tree.remove(vec[8]);
	//tree.remove(vec[7]);
	//tree.remove(vec[6]);
	//tree.find(vec[6])->ptr->print();


	//tree.remove(vec[4]);
	//tree.remove(vec[1]);
	//tree.remove(vec[8]);
	//tree.remove(vec[6]);
	//tree.remove(vec[3]);
	//tree.remove(vec[7]);
	//tree.find(vec[5])->ptr->print();
	//tree.find(tree.head->ptr)->ptr->print();

	//print(tree.find(tree.head->ptr)->index);
	//print(tree.find(tree.head->right->ptr)->index);
	//print(tree.find(tree.head->right->right->ptr)->left);
	


	//print(tree.find(vec[4])->index);
	//print(tree.find(vec[5])->index);



	std::cout<<"end"<<std::endl;
	return 0;
}