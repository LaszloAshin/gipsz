#ifndef LIB_VECTOR_HEADER
#define LIB_VECTOR_HEADER

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

#endif // LIB_VECTOR_HEADER
