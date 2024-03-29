#ifndef GAME_BBOX_HEADER
#define GAME_BBOX_HEADER

#include <lib/vec.hh>

template <typename V>
class Aabb {
public:
	Aabb() : empty_(true) {}

	bool empty() const { return empty_; }
	V low() const { return low_; }
	V high() const { return high_; }

	void
	add(const V& v)
	{
		if (empty()) {
			low_ = high_ = v;
			empty_ = false;
		} else {
			low_ = min(low_, v);
			high_ = max(high_, v);
		}
	}

	void add(const Aabb& other) { add(other.low()); add(other.high()); }

	bool
	inside(const V& v, const V& padding = V())
	const
	{
		return lessAll(low_, v + padding + V::epsilon()) && lessAll(v - padding - V::epsilon(), high_);
	}

private:
	V low_, high_;
	bool empty_;
};

typedef Aabb<Vec3d> Aabb3d;

bool visibleByCamFrustum(const Aabb3d& bb);

#endif // GAME_BBOX_HEADER
