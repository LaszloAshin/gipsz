#ifndef LIB_SURFACE_HEADER
#define LIB_SURFACE_HEADER

#include <ostream>
#include <stdexcept>
#include <string>

template <typename T>
class Surface {
public:
	Surface() : textureId_(0), u1_(), u2_(), v_() {}

	Surface(int textureId, T u1, T u2, T v)
	: textureId_(textureId), u1_(u1), u2_(u2), v_(v)
	{}

	void cropLeft(double a) { u1_ = lerp(u1_, u2_, a); }
	void cropRight(double a) { u2_ = lerp(u2_, u1_, a); }

	T u1() const { return u1_; }
	T u2() const { return u2_; }
	T v() const { return v_; }
	unsigned textureId() const { return textureId_; }

	friend std::ostream&
	operator<<(std::ostream& os, const Surface& s)
	{
		return os << "surface " << s.u1() << ' ' << s.u2() << ' ' << s.v() << ' ' << s.textureId();
	}

	friend std::istream&
	operator>>(std::istream& is, Surface& s)
	{
		std::string name;
		is >> name;
		if (name != "surface") throw std::runtime_error("surface expected");
		T u1, u2, v;
		unsigned textureId;
		is >> u1 >> u2 >> v >> textureId;
		s = Surface(textureId, u1, u2, v);
		return is;
	}

private:
	static double lerp(double x, double y, double a) { return x * (1.0f - a) + y * a; }

	unsigned textureId_;
	T u1_, u2_;
	T v_;
};

typedef Surface<double> Surfaced;

#endif // LIB_SURFACE_HEADER
