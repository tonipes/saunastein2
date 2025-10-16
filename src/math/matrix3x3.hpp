// Copyright (c) 2025 Inan Evin
#pragma once

#include "vector3.hpp"

namespace SFG
{
	class quat;
	class ostream;
	class istream;

	// Column-major 3x3: m[col * 3 + row]
	class matrix3x3
	{
	public:
		float m[9];

		matrix3x3() = default;

		// Construct by columns (column-major):
		// (m00 m10 m20) is column 0, etc.
		matrix3x3(float m00,
				  float m10,
				  float m20, // Col 0
				  float m01,
				  float m11,
				  float m21, // Col 1
				  float m02,
				  float m12,
				  float m22); // Col 2

		static const matrix3x3 identity;

		// Factories
		static matrix3x3 scale(const vector3& s);										  // diag(s.x, s.y, s.z)
		static matrix3x3 rotation(const quat& q);										  // pure rotation (no translation)
		static matrix3x3 from_axes(const vector3& x, const vector3& y, const vector3& z); // columns

		// Access
		inline float operator[](int index) const
		{
			return m[index];
		}
		inline float& operator[](int index)
		{
			return m[index];
		}

		inline vector3 get_column(int i) const
		{
			return vector3(m[i * 3 + 0], m[i * 3 + 1], m[i * 3 + 2]);
		}
		inline void set_column(int i, const vector3& c)
		{
			m[i * 3 + 0] = c.x;
			m[i * 3 + 1] = c.y;
			m[i * 3 + 2] = c.z;
		}
		inline vector3 get_row(int i) const
		{
			return vector3(m[0 * 3 + i], m[1 * 3 + i], m[2 * 3 + i]);
		}
		inline void set_row(int i, const vector3& r)
		{
			m[0 * 3 + i] = r.x;
			m[1 * 3 + i] = r.y;
			m[2 * 3 + i] = r.z;
		}

		// Algebra
		matrix3x3 transposed() const;
		matrix3x3 inversed() const; // general 3x3 (handles shear / non-uniform scale if invertible)
		float	  determinant() const;

		// Element-wise abs (useful for conservative extent transforms)
		static matrix3x3 abs(const matrix3x3& A);

		// Mul
		inline vector3 operator*(const vector3& v) const
		{
			// Column-major affine linear part
			return vector3(m[0] * v.x + m[3] * v.y + m[6] * v.z, m[1] * v.x + m[4] * v.y + m[7] * v.z, m[2] * v.x + m[5] * v.y + m[8] * v.z);
		}

		inline matrix3x3 operator*(const matrix3x3& other) const
		{
			matrix3x3 r;
			for (int i = 0; i < 3; ++i) // row
			{
				for (int j = 0; j < 3; ++j) // col
				{
					r.m[j * 3 + i] = m[0 * 3 + i] * other.m[j * 3 + 0] + m[1 * 3 + i] * other.m[j * 3 + 1] + m[2 * 3 + i] * other.m[j * 3 + 2];
				}
			}
			return r;
		}

		inline matrix3x3 operator*(float s) const
		{
			matrix3x3 r;
			for (int i = 0; i < 9; ++i)
				r.m[i] = m[i] * s;
			return r;
		}
		inline matrix3x3& operator*=(float s)
		{
			for (int i = 0; i < 9; ++i)
				m[i] *= s;
			return *this;
		}

		// Utilities
		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};
}
