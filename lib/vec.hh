#ifndef LIB_VECTOR_HEADER
#define LIB_VECTOR_HEADER

#include <limits>
#include <cmath>

template <typename T>
class Vec2 {
public:
	Vec2() : x_(), y_() {}
	Vec2(T x, T y) : x_(x), y_(y) {}

	T x() const { return x_; }
	T y() const { return y_; }

private:
	T x_, y_;
};

typedef Vec2<double> Vec2d;

template <typename T>
class Vec3 {
public:
	static Vec3 epsilon() { const T e(std::numeric_limits<T>::epsilon()); return Vec3(e, e, e); }

	Vec3() : x_(), y_() {}
	Vec3(T x, T y, T z) : x_(x), y_(y), z_(z) {}

	T x() const { return x_; }
	T y() const { return y_; }
	T z() const { return z_; }

	void x(float f) { x_ = f; }
	void y(float f) { y_ = f; }
	void z(float f) { z_ = f; }

	Vec3 operator-() const { return Vec3(-x(), -y(), -z()); }
	Vec3& operator+=(const Vec3& rhs) { x_ += rhs.x(); y_ += rhs.y(); z_ += rhs.z(); return *this; }
	Vec3& operator-=(const Vec3& rhs) { return *this += -rhs; }

	friend Vec3 operator+(Vec3 lhs, const Vec3& rhs) { return lhs += rhs; }
	friend Vec3 operator-(Vec3 lhs, const Vec3& rhs) { return lhs -= rhs; }

	Vec3& operator*=(T rhs) { x_ *= rhs; y_ *= rhs; z_ *= rhs; return *this; }
	Vec3& operator/=(T rhs) { x_ /= rhs; y_ /= rhs; z_ /= rhs; return *this; }

	friend Vec3 operator*(Vec3 lhs, T rhs) { return lhs *= rhs; }
	friend Vec3 operator*(T lhs, Vec3 rhs) { return rhs *= lhs; }
	friend Vec3 operator/(Vec3 lhs, T rhs) { return lhs /= rhs; }
	friend Vec3 operator/(T lhs, Vec3 rhs) { return rhs /= lhs; }

	friend T dot(const Vec3& lhs, const Vec3& rhs) { return lhs.x() * rhs.x() + lhs.y() * rhs.y() + lhs.z() * rhs.z(); }
	T len() const;

	friend Vec3
	min(const Vec3& lhs, const Vec3& rhs)
	{
		return Vec3(
			lhs.x() < rhs.x() ? lhs.x() : rhs.x(),
			lhs.y() < rhs.y() ? lhs.y() : rhs.y(),
			lhs.z() < rhs.z() ? lhs.z() : rhs.z()
		);
	}

	friend Vec3
	max(const Vec3& lhs, const Vec3& rhs)
	{
		return Vec3(
			rhs.x() < lhs.x() ? lhs.x() : rhs.x(),
			rhs.y() < lhs.y() ? lhs.y() : rhs.y(),
			rhs.z() < lhs.z() ? lhs.z() : rhs.z()
		);
	}

	friend bool
	lessAll(const Vec3& lhs, const Vec3& rhs)
	{
		return lhs.x() < rhs.x() && lhs.y() < rhs.y() && lhs.z() < rhs.z();
	}

	friend Vec3
	operator%(const Vec3& lhs, const Vec3& rhs)
	{
		return Vec3(
			lhs.y() * rhs.z() - lhs.z() * rhs.y(),
			lhs.z() * rhs.x() - lhs.x() * rhs.z(),
			lhs.x() * rhs.y() - lhs.y() * rhs.x()
		);
	}

	Vec3 norm() const { return *this / len(); }

private:
	T x_, y_, z_;
};

typedef Vec3<double> Vec3d;

template <>
inline double Vec3d::len() const { return std::sqrt(dot(*this, *this)); }

#endif // LIB_VECTOR_HEADER
