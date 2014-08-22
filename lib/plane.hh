#ifndef LIB_PLANE_HEADER
#define LIB_PLANE_HEADER

#include <lib/vec.hh>

#include <istream>
#include <string>
#include <stdexcept>
#include <limits>

template <typename T, size_t D>
class Plane {
public:
	typedef Vec<T, D + 1> V;
	typedef Vec<T, D> N;

	Plane() {}

	template <class U, class W>
	Plane(const U& p1, const W& p2) : v_(computeFromTwoPoints(p1, p2)) {}

	V v() const { return v_; }
	N n() const { return v().xy(); }
	T c() const { return v().z(); }

	template <class U>
	T determine(const U& v) const { return dot(n(), v) + c(); }

	template <typename U>
	friend T dot(const Plane& p, const U& v) { return wedge(p.n(), v); }

	static Plane
	read(std::istream& is)
	{
		std::string name;
		is >> name;
		if (name != "plane2") throw std::runtime_error("plane2 expected");
		T a, b, c;
		is >> a >> b >> c;
		return Plane(a, b, c);
	}

	void
	write(std::ostream& os)
	const
	{
		os << "plane2 " << v().x() << ' ' << v().y() << ' ' << v().z() << std::endl;
	}

	template <class U>
	friend U
	intersect(const Plane& lhs, const Plane& rhs)
	{
		const T q = wedge(rhs.n(), lhs.n());
		const T epsilon = std::numeric_limits<T>::epsilon();
		if (q > -epsilon && q < epsilon) throw std::overflow_error("parallel planes do not intersect each other");
		const Vec<T, 2> p(wedge(rhs.v().yz(), lhs.v().yz()), wedge(lhs.v().xz(), rhs.v().xz()));
		const Vec<T, 2> result(p / q);
		return U(result.x(), result.y());
	}

	Plane operator-() const { return Plane(-n().x(), -n().y(), -c()); }

private:
	Plane(T a, T b, T c) : v_(a, b, c) {}

	template <class U, class W>
	static V computeFromTwoPoints(const U& p1, const W& p2)
	{
		N n(p2.y() - p1.y(), p1.x() - p2.x());
		return V(n.x(), n.y(), -dot(n, p1));
	}

	V v_;
};

typedef Plane<double, 2> Plane2d;

#endif // LIB_PLANE_HEADER
