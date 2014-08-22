#ifndef LIB_MASSPOINT_HEADER
#define LIB_MASSPOINT_HEADER

#include <lib/vec.hh>

template <typename T, size_t D>
class MassPoint {
public:
	typedef Vec<T, D> V;

	explicit MassPoint(T mass = 1.0f) : mass_(mass) {}

	V pos() const { return pos_; }
	V velo() const { return velo_; }
	T mass() const { return mass_; }

	void pos(const V& p) { pos_ = p; }
	void velo(const V& v) { velo_ = v; } // TODO: remove

	void force(const V& f, T dt) { velo_ += f / mass() * dt; }
	void friction(T c) { velo_ *= 1.0f - c; }
	void move(T dt) { pos_ += velo() * dt; }
	void stop() { velo_ = V(); }

private:
	V pos_;
	V velo_;
	T mass_;
};

typedef MassPoint<double, 3> MassPoint3d;

#endif // LIB_MASSPOINT_HEADER
