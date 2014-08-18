#ifndef LIB_PLANE_HEADER
#define LIB_PLANE_HEADER

#include <istream>
#include <string>
#include <stdexcept>
#include <limits>

template <typename T>
class Plane2 {
public:
	Plane2() : a_(), b_(), c_() {}

	template <typename U>
	Plane2(const U& v1, const U& v2)
	: a_(v2.y() - v1.y())
	, b_(v1.x() - v2.x())
	, c_(-(a() * v1.x() + b() * v1.y()))
	{}

	T a() const { return a_; }
	T b() const { return b_; }
	T c() const { return c_; }

	template <typename U>
	T determine(const U& v) const { return a() * v.x() + b() * v.y() + c(); }

	template <typename U>
	T dot(const U& v) const { return a() * v.y() - b() * v.x(); }

	static Plane2
	read(std::istream& is)
	{
		std::string name;
		is >> name;
		if (name != "plane2") throw std::runtime_error("plane2 expected");
		Plane2 result;
		is >> result.a_ >> result.b_ >> result.c_;
		return result;
	}

	void
	write(std::ostream& os)
	const
	{
		os << "plane2 " << a() << ' ' << b() << ' ' << c() << std::endl;
	}

	template <class U>
	friend U
	intersect(const Plane2& lhs, const Plane2& rhs)
	{
		const T q = rhs.a() * lhs.b() - lhs.a() * rhs.b();
		const T epsilon = std::numeric_limits<T>::epsilon();
		if (q > -epsilon && q < epsilon) throw std::overflow_error("parallel planes do not intersect each other");
		const T x = (rhs.b() * lhs.c() - lhs.b() * rhs.c()) / q;
		const T y = (lhs.a() * rhs.c() - rhs.a() * lhs.c()) / q;
		return U(x, y);
	}

	Plane2 operator-() const { return Plane2(-a(), -b(), -c()); }

private:
	Plane2(T a, T b, T c) : a_(a), b_(b), c_(c) {}

	T a_, b_, c_;
};

typedef Plane2<double> Plane2d;

#endif // LIB_PLANE_HEADER
