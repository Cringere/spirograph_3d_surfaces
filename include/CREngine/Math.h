#ifndef CRENGINE_PUBLIC_HEADER_MATH_H
#define CRENGINE_PUBLIC_HEADER_MATH_H

#include <cmath>
#include <string>

namespace CREngine {
	namespace Math {
		//definitions
		class Vector2D;
		class Vector3D;
		class Matrix2D;
		class Matrix3D;

		class Vector2D {
			public:
				//vars
				float v[2];

				//constructors
				Vector2D();
				Vector2D(float x, float y);
				Vector2D(const Vector2D &vec);

				//setters and getters
				inline void set(float x, float y) {v[0] = x; v[1] = y;}
				inline float operator[](unsigned int i) const {return v[i];}
				inline float &operator[](unsigned int i) {return v[i];}
				Vector2D operator=(const Vector2D &vec);

				//overloaded operators
				inline Vector2D operator+(const Vector2D &s) const {return Vector2D(v[0] + s.v[0], v[1] + s.v[1]);}
				inline void operator+=(const Vector2D &s) {v[0] += s.v[0]; v[1] += s.v[1];}
				
				inline Vector2D operator-(const Vector2D &s) const {return Vector2D(v[0] - s.v[0], v[1] - s.v[1]);}
				inline void operator-=(const Vector2D &s) {v[0] -= s.v[0]; v[1] -= s.v[1];}
				
				inline Vector2D operator*(float s) const {return Vector2D(v[0] * s, v[1] * s);}
				inline void operator*=(float s) {v[0] *= s; v[1] *= s;}
				
				inline Vector2D operator*(const Vector2D &s) const {return Vector2D(v[0] * s.v[0], v[1] * s.v[1]);}
				inline void operator*=(const Vector2D &s) {v[0] *= s.v[0]; v[1] *= s.v[1];}

				inline Vector2D operator/(float s) const {return Vector2D(v[0] / s, v[1] / s);}
				inline void operator/=(float s) {v[0] /= s; v[1] /= s;}
				
				inline Vector2D operator/(const Vector2D &s) const {return Vector2D(v[0] / s.v[0], v[1] / s.v[1]);}
				inline void operator/=(const Vector2D &s) {v[0] /= s.v[0]; v[1] /= s.v[1];}

				//products
				inline float dot(const Vector2D &s) const {return v[0] * s[0] + v[1] * s[1];}

				//special operations
				inline float length() const {return sqrt(v[0] * v[0] + v[1] * v[1]);}
				inline Vector2D normalize() const {float s = length(); return Vector2D(v[0] / s, v[1] / s);}
				inline float distance_from(const Vector2D &v) const { return (*this - v).length();}

				/*
				return the angle [-pi, pi] with respect to the x axis (counter-clockwise)
				*/
				float get_angle();

				//debug
				std::string to_string();
				void print();
		};

		class Vector3D {
			public:
				float v[3];
				
				//constructors
				Vector3D();
				Vector3D(float x, float y, float z);
				Vector3D(const Vector3D &vec);
				
				//setters and getters
				inline void set(float x, float y, float z) {v[0] = x; v[1] = y; v[2] = z;}
				Vector3D operator=(const Vector3D &vec);
				inline float operator[](unsigned int i) const {return v[i];}
				inline float &operator[](unsigned int i) {return v[i];}

				//overloaded operators
				inline Vector3D operator+(const Vector3D &s) const {return Vector3D(v[0] + s.v[0], v[1] + s.v[1], v[2] + s.v[2]);}
				inline void operator+=(const Vector3D &s) {v[0] += s.v[0]; v[1] += s.v[1]; v[2] += s.v[2];}
				
				inline Vector3D operator-(const Vector3D &s) const {return Vector3D(v[0] - s.v[0], v[1] - s.v[1], v[2] - s.v[2]);}
				inline void operator-=(const Vector3D &s) {v[0] -= s.v[0]; v[1] -= s.v[1]; v[2] -= s.v[2];}
				
				inline Vector3D operator*(float s) const {return Vector3D(v[0] * s, v[1] * s, v[2] * s);}
				inline void operator*=(float s) {v[0] *= s; v[1] *= s; v[2] *= s;}
				
				inline Vector3D operator*(const Vector3D &s) const {return Vector3D(v[0] * s.v[0], v[1] * s.v[1], v[2] * s.v[2]);}
				inline void operator*=(const Vector3D &s) {v[0] *= s.v[0]; v[1] *= s.v[1]; v[2] *= s.v[2];}

				inline Vector3D operator/(float s) const {return Vector3D(v[0] / s, v[1] / s, v[2] / s);}
				inline void operator/=(float s) {v[0] /= s; v[1] /= s; v[2] /= s;}
				
				inline Vector3D operator/(const Vector3D &s) const {return Vector3D(v[0] / s.v[0], v[1] / s.v[1], v[2] / s.v[2]);}
				inline void operator/=(const Vector3D &s) {v[0] /= s.v[0]; v[1] /= s.v[1]; v[2] /= s.v[2];}

				//products
				inline float dot(const Vector3D &s) const {return v[0] * s[0] + v[1] * s[1] + v[2] * s[2];}
				inline Vector3D cross(const Vector3D &s) const {return Vector3D(v[1] * s[2] - v[2] * s[1], v[2] * s[0] - v[0] * s[2], v[0] * s[1] - v[1] * s[0]);}

				//special operations
				inline float length() const {return sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);}
				inline Vector3D normalize() const {float l = length(); return Vector3D(v[0] / l, v[1] / l, v[2] / l);}
				inline float distance_from(const Vector3D &v) const { return (*this - v).length(); }

				inline float angle_cos(const Vector3D &v) const { return (this->dot(v))/ (length() * v.length()); }

				//debug
				std::string to_string();
				void print();
		};

		class Vector4D {
		public:
			float v[4];

			//constructors
			Vector4D();
			Vector4D(float x, float y, float z, float w);
			Vector4D(const Vector4D &vec);

			//setters and getters
			inline void set(float x, float y, float z, float w) {v[0] = x; v[1] = y; v[2] = z; v[3] = w;}
			Vector4D operator=(const Vector4D &vec);
			inline float operator[](unsigned int i) const {return v[i];}
			inline float &operator[](unsigned int i) {return v[i];}

			//overloaded operators
			inline Vector4D operator+(const Vector4D &s) const {return Vector4D(v[0] + s.v[0], v[1] + s.v[1], v[2] + s.v[2], v[3] + s.v[3]);}
			inline void operator+=(const Vector4D &s) {v[0] += s.v[0]; v[1] += s.v[1]; v[2] += s.v[2]; v[3] += s.v[3];}
			
			inline Vector4D operator-(const Vector4D &s) const {return Vector4D(v[0] - s.v[0], v[1] - s.v[1], v[2] - s.v[2], v[3] - s.v[3]);}
			inline void operator-=(const Vector4D &s) {v[0] -= s.v[0]; v[1] -= s.v[1]; v[2] -= s.v[2]; v[3] -= s.v[3];}
			
			inline Vector4D operator*(float s) const {return Vector4D(v[0] * s, v[1] * s, v[2] * s, v[3] * s);}
			inline void operator*=(float s) {v[0] *= s; v[1] *= s; v[2] *= s; v[3] *= s;}
			
			inline Vector4D operator*(const Vector4D &s) const {return Vector4D(v[0] * s.v[0], v[1] * s.v[1], v[2] * s.v[2], v[3] * s.v[3]);}
			inline void operator*=(const Vector4D &s) {v[0] *= s.v[0]; v[1] *= s.v[1]; v[2] *= s.v[2]; v[3] *= s.v[3];}

			inline Vector4D operator/(float s) const {return Vector4D(v[0] / s, v[1] / s, v[2] / s, v[3] / s);}
			inline void operator/=(float s) {v[0] /= s; v[1] /= s; v[2] /= s; v[3] /= s;}
			
			inline Vector4D operator/(const Vector4D &s) const {return Vector4D(v[0] / s.v[0], v[1] / s.v[1], v[2] / s.v[2], v[3] / s.v[3]);}
			inline void operator/=(const Vector4D &s) {v[0] /= s.v[0]; v[1] /= s.v[1]; v[2] /= s.v[2]; v[3] /= s.v[3];}

			//products
			inline float dot(const Vector4D &s) const {return v[0] * s[0] + v[1] * s[1] + v[2] * s[2] + v[3] * s[3];}
			
			//special operations
			inline float length() const {return sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3]);}
			inline Vector4D normalize() {float l = length(); return Vector4D(v[0] / l, v[1] / l, v[2] / l, v[3] / l);}
			inline float distance_from(const Vector4D &v) const { return (*this - v).length(); }

			//debug
			std::string to_string();
			void print();
		};

		class Matrix2D {
		public:
			Vector2D rows[2];

			//constructors
			Matrix2D(float a, float b, float c, float d);
			Matrix2D(const Vector2D &row_a, const Vector2D &row_b);

			//setters and getters
			//void set(float x, float y, float z, float w) {v[0] = x; v[1] = y; v[2] = z; v[3] = w;}
			Matrix2D operator=(const Matrix2D &mat);
			inline const Vector2D &operator[](unsigned int i) const {return rows[i];}
			inline Vector2D &operator[](unsigned int i) {return rows[i];}

			//overloaded operators
			Vector2D operator*(const Vector2D &vec);
			
			Matrix2D operator*(const Matrix2D &mat);
			void operator*=(const Matrix2D &mat);

			//static creators
			static Matrix2D identity();
			static Matrix2D translation(float delta_x, float delta_y);
			static Matrix2D rotation(float theta);
		};

		class Matrix3D {
		public:
			Vector3D rows[4];

			Matrix3D();

			Matrix3D(const Vector3D &row_a, const Vector3D &row_b, const Vector3D &row_c);

			//void set(float x, float y, float z, float w) {v[0] = x; v[1] = y; v[2] = z; v[3] = w;}

			Vector3D operator[](unsigned int i) const {return rows[i];}
			Vector3D &operator[](unsigned int i) {return rows[i];}

			//matrix - vector operations
			Matrix3D operator*(float s) const;
			Vector3D operator*(const Vector3D &vec) const;

			Matrix3D operator*(const Matrix3D &mat) const;
			Matrix3D operator+(const Matrix3D &mat) const;

			//static creators
			static Matrix3D identity();
			static Matrix3D rotation(const Vector3D &axis);
		};

		class Matrix4D {
		public:
			Vector4D rows[4];

			Matrix4D();

			Matrix4D(const Vector4D &row_a, const Vector4D &row_b, const Vector4D &row_c, const Vector4D &row_d);

			//void set(float x, float y, float z, float w) {v[0] = x; v[1] = y; v[2] = z; v[3] = w;}

			inline void operator=(const Matrix4D &mat) {
				rows[0] = mat.rows[0];
				rows[1] = mat.rows[1];
				rows[2] = mat.rows[2];
				rows[3] = mat.rows[3];
			}
			
			Vector4D operator[](unsigned int i) const {return rows[i];}
			Vector4D &operator[](unsigned int i) {return rows[i];}

			//matrix - vector operations
			Vector4D operator*(const Vector4D &vec);

			Matrix4D operator*(const Matrix4D &mat);

			//static creators
			static Matrix4D identity();
			static Matrix4D translate(const Vector3D &position);
			static Matrix4D view(const Vector3D &right, const Vector3D &up, const Vector3D &forward);
			static Matrix4D orthographic_projection(const Vector2D &size, float n, float f);
		};
	}
}

#endif
