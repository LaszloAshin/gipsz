#ifndef LIB_VECTOR_HEADER
#define LIB_VECTOR_HEADER

#include <limits>
#include <cmath>
#include <stddef.h> // size_t

template <class V, typename T>
struct VecBase {
	template <class U>
	V& operator-=(const U& rhs) { return that() += -rhs; }

	template <class U>
	friend V operator+(V lhs, const U& rhs) { return lhs += rhs; }

	template <class U>
	friend V operator-(V lhs, const U& rhs) { return lhs -= rhs; }

	friend V operator*(V lhs, T rhs) { return lhs *= rhs; }
	friend V operator*(T lhs, V rhs) { return rhs *= lhs; }
	friend V operator/(V lhs, T rhs) { return lhs /= rhs; }
	friend V operator/(T lhs, V rhs) { return rhs /= lhs; }

protected:
	V& that() { return *static_cast<V*>(this); }
	const V& that() const { return *static_cast<const V*>(this); }
};

template <typename T, size_t D>
struct Vec {};

template <typename T>
class Vec<T, 2> : public VecBase<Vec<T, 2>, T> {
public:
	Vec() : x_(), y_() {}
	Vec(T x, T y) : x_(x), y_(y) {}

	T x() const { return x_; }
	T y() const { return y_; }

	Vec operator-() const { return Vec(-x(), -y()); }

	template <class U>
	Vec& operator+=(const U& rhs) { x_ += rhs.x(); y_ += rhs.y(); return *this; }

	Vec& operator*=(T rhs) { x_ *= rhs; y_ *= rhs; return *this; }
	Vec& operator/=(T rhs) { x_ /= rhs; y_ /= rhs; return *this; }

	template <class U>
	friend T dot(const Vec& lhs, const U& rhs) { return lhs.x() * rhs.x() + lhs.y() * rhs.y(); }

	template <class U>
	friend T wedge(const Vec& lhs, const U& rhs) { return lhs.x() * rhs.y() - lhs.y() * rhs.x(); }

	friend Vec perpendicular(const Vec& v) { return Vec(v.y(), -v.x()); }

private:
	T x_, y_;
};

typedef Vec<double, 2> Vec2d;

template <typename T>
class Vec<T, 3> : public VecBase<Vec<T, 3>, T> {
public:
	static Vec epsilon() { const T e(std::numeric_limits<T>::epsilon()); return Vec(e, e, e); }

	Vec() : x_(), y_(), z_() {}
	Vec(T x, T y, T z) : x_(x), y_(y), z_(z) {}

	T x() const { return x_; }
	T y() const { return y_; }
	T z() const { return z_; }

	Vec<T, 2> xy() const { return Vec<T, 2>(x(), y()); }
	Vec<T, 2> yz() const { return Vec<T, 2>(y(), z()); }
	Vec<T, 2> xz() const { return Vec<T, 2>(x(), z()); }

	void x(float f) { x_ = f; }
	void y(float f) { y_ = f; }
	void z(float f) { z_ = f; }

	Vec operator-() const { return Vec(-x(), -y(), -z()); }

	template <class U>
	Vec& operator+=(const U& rhs) { x_ += rhs.x(); y_ += rhs.y(); z_ += rhs.z(); return *this; }

	Vec& operator*=(T rhs) { x_ *= rhs; y_ *= rhs; z_ *= rhs; return *this; }
	Vec& operator/=(T rhs) { x_ /= rhs; y_ /= rhs; z_ /= rhs; return *this; }

	template <class U>
	friend T dot(const Vec& lhs, const U& rhs) { return lhs.x() * rhs.x() + lhs.y() * rhs.y() + lhs.z() * rhs.z(); }

	template <class U>
	friend Vec
	cross(const Vec& lhs, const U& rhs)
	{
		return Vec(
			lhs.y() * rhs.z() - lhs.z() * rhs.y(),
			lhs.z() * rhs.x() - lhs.x() * rhs.z(),
			lhs.x() * rhs.y() - lhs.y() * rhs.x()
		);
	}

	template <class U>
	friend Vec
	min(const Vec& lhs, const U& rhs)
	{
		return Vec(
			lhs.x() < rhs.x() ? lhs.x() : rhs.x(),
			lhs.y() < rhs.y() ? lhs.y() : rhs.y(),
			lhs.z() < rhs.z() ? lhs.z() : rhs.z()
		);
	}

	template <class U>
	friend Vec
	max(const Vec& lhs, const U& rhs)
	{
		return Vec(
			rhs.x() < lhs.x() ? lhs.x() : rhs.x(),
			rhs.y() < lhs.y() ? lhs.y() : rhs.y(),
			rhs.z() < lhs.z() ? lhs.z() : rhs.z()
		);
	}

	template <class U>
	friend bool
	lessAll(const Vec& lhs, const U& rhs)
	{
		return lhs.x() < rhs.x() && lhs.y() < rhs.y() && lhs.z() < rhs.z();
	}

private:
	T x_, y_, z_;
};

typedef Vec<double, 3> Vec3d;

template <size_t D>
double len(const Vec<double, D>& v) { return std::sqrt(dot(v, v)); }

template <class T>
T norm(const T& v) { return v / len(v); }

#endif // LIB_VECTOR_HEADER
