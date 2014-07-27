#include "bsp.h"
#include "ed.h"
#include "gr.h"

#include <vector>
#include <memory>
#include <algorithm>
#include <functional>
#include <ostream>
#include <limits>
#include <stdexcept>
#include <numeric>
#include <iostream>

namespace bsp {

class Vertex {
public:
	Vertex() : x_(0.0f), y_(0.0f) {}
	Vertex(float x, float y) : x_(x), y_(y) {}

	float x() const { return x_; }
	float y() const { return y_; }

private:
	float x_, y_;
};

std::ostream& operator<<(std::ostream& os, const Vertex& v) { return os << "(" << v.x() << ", " << v.y() << ")"; }

class Plane2d {
public:
	Plane2d() : a_(), b_(), c_() {}
	Plane2d(const Vertex& v1, const Vertex& v2)
	: a_(v2.y() - v1.y())
	, b_(v1.x() - v2.x())
	, c_(-(a_ * v1.x() + b_ * v1.y()))
	{}

	float determine(const Vertex& v) const { return a_ * v.x() + b_ * v.y() + c_; }
	float dot(const Vertex& v) const { return a_ * v.y() - b_ * v.x(); }
	void save(FILE* fp) const { fwrite(&a_, sizeof(a_), 1, fp); fwrite(&b_, sizeof(b_), 1, fp); fwrite(&c_, sizeof(c_), 1, fp); }
	void print(std::ostream& os) const { os << "Plane2d(" << a_ << ", " << b_ << ", " << c_ << ")"; }
	friend Vertex intersect(const Plane2d& lhs, const Plane2d& rhs);

private:
	float a_, b_, c_;
};

Vertex
intersect(const Plane2d& lhs, const Plane2d& rhs)
{
	const float e = std::numeric_limits<float>::epsilon();
	const float q = rhs.a_ * lhs.b_ - lhs.a_ * rhs.b_;
	if (q > -e && q < e) throw std::overflow_error("parallel planes do not intersect each other");
	const float p = 1.0f / q;
	const float x = (rhs.b_ * lhs.c_ - lhs.b_ * rhs.c_) * p;
	const float y = (lhs.a_ * rhs.c_ - rhs.a_ * lhs.c_) * p;
	return Vertex(x, y);
}

class Wall {
public:
	Wall(const Vertex& a, const Vertex& b) : a_(a), b_(b) {}

	Vertex a() const { return a_; }
	Vertex b() const { return b_; }

private:
	Vertex a_, b_;
};

class Sector;
class Node;

struct Renderer {
	void draw(const Vertex& v) { edVertex(v.x(), v.y()); }
	void draw(const Vertex& a, const Vertex& b) { edVector(a.x(), a.y(), b.x(), b.y()); }
	void operator()(const Wall& w) { draw(w.a()); draw(w.b()); draw(w.a(), w.b()); }
	void operator()(const Sector& s);
	void draw(const Plane2d& p);
	void draw(const Node& n);
};

void
Renderer::draw(const Plane2d& p)
{
	int x1, y1, x2, y2;
	edGetViewPort(&x1, &y1, &x2, &y2);
	const Vertex lu(x1, y1);
	const Vertex ru(x2, y1);
	const Vertex rd(x2, y2);
	const Vertex ld(x1, y2);
	const Plane2d up(ru, lu);
	const Plane2d right(rd, ru);
	const Plane2d down(ld, rd);
	const Plane2d left(lu, ld);
	std::vector<Vertex> v;
	try {
		const Vertex mpup(intersect(p, up));
		if (right.determine(mpup) * left.determine(mpup) > 0) v.push_back(mpup);
	} catch (...) {}
	try {
		const Vertex mpright(intersect(p, right));
		if (up.determine(mpright) * down.determine(mpright) > 0) v.push_back(mpright);
	} catch (...) {}
	try {
		const Vertex mpdown(intersect(p, down));
		if (right.determine(mpdown) * left.determine(mpdown) > 0) v.push_back(mpdown);
	} catch (...) {}
	try {
		const Vertex mpleft(intersect(p, left));
		if (up.determine(mpleft) * down.determine(mpleft) > 0) v.push_back(mpleft);
	} catch (...) {}
	if (v.size() < 2) return;
	edLine(v[0].x(), v[0].y(), v[1].x(), v[1].y());
}

class SplitStats {
public:
	SplitStats() : balance_(), splitCount_() {}

	void wallIsFront() { ++balance_; }
	void wallIsBack() { --balance_; }
	void wallIsSplit() { ++splitCount_; }

	int balance() const { return balance_; }
	size_t splitCount() const { return splitCount_; }
	int score() const { return abs(balance()) + splitCount() * 10; }

	SplitStats& operator+=(const SplitStats& rhs) { balance_ += rhs.balance(); splitCount_ += rhs.splitCount(); return *this; }
	void print(std::ostream& os) const { os << "SplitStats(" << balance() << ", " << splitCount() << ", " << score() << ")"; }

private:
	int balance_;
	size_t splitCount_;
};

class Sector {
	typedef std::vector<Wall> Walls;

public:
	Sector() : id_() {}
	Sector(int id) : id_(id) {}

	typedef Walls::const_iterator const_iterator;

	const_iterator begin() const { return walls_.begin(); }
	const_iterator end() const { return walls_.end(); }

	int id() const { return id_; }
	void add(const Wall& w) { walls_.push_back(w); }
	SplitStats computeStats(const Plane2d& plane) const;

private:
	int id_;
	Walls walls_;
};

SplitStats Sector::computeStats(const Plane2d& plane) const {
	SplitStats result;
	for (const_iterator i(begin()); i != end(); ++i) {
		const float da = plane.determine(i->a());
		const float db = plane.determine(i->b());
		if (da * db < 0) {
			result.wallIsSplit();
		} else if (da + db > std::numeric_limits<float>::epsilon()) {
			result.wallIsFront();
		} else if (da + db < -std::numeric_limits<float>::epsilon()) {
			result.wallIsBack();
		}
	}
	return result;
}

void
Renderer::operator()(const Sector& s)
{
	grSetColor((s.id() * 343) | 3);
	std::for_each(s.begin(), s.end(), *this);
}

class Node {
	typedef std::vector<Sector> Sectors;

public:
	typedef Sectors::const_iterator const_iterator;

	const_iterator begin() const { return sectors_.begin(); }
	const_iterator end() const { return sectors_.end(); }

	void build();
	void save(FILE*) const {}
	void add(int sectorId, const Wall& w);
	Plane2d plane() const { return plane_; }

private:
	SplitStats computeStats(const Plane2d& plane) const;
	void findBestPlane();

	Sectors sectors_;
	Plane2d plane_;
};

void
Renderer::draw(const Node& n)
{
	std::for_each(n.begin(), n.end(), *this);
	grSetColor(0xe0);
	draw(n.plane());
}

SplitStats Node::computeStats(const Plane2d& plane) const {
	SplitStats result;
	for (Sectors::const_iterator i(sectors_.begin()); i != sectors_.end(); ++i) {
		result += i->computeStats(plane);
	}
	return result;
}

void
Node::add(int sectorId, const Wall& w)
{
	if (sectors_.empty() || sectors_.back().id() != sectorId) {
		sectors_.push_back(Sector(sectorId));
	}
	sectors_.back().add(w);
}

void
Node::findBestPlane()
{
	bool bestFound = false;
	SplitStats bestStat;

	for (Sectors::const_iterator i(sectors_.begin()); i != sectors_.end(); ++i) {
		for (Sector::const_iterator j(i->begin()); j != i->end(); ++j) {
			const Plane2d p(j->a(), j->b());
			const SplitStats s(computeStats(p));
			if (!bestFound || s.score() < bestStat.score()) {
				bestFound = true;
				plane_ = p;
				bestStat = s;
			}
		}
	}
}

void
Node::build()
{
	findBestPlane();
}

class Tree {
public:
	void clear() { root_.reset(new Node); }
	void build() { if(root_.get()) root_->build(); }
	void show() const { if (root_.get()) Renderer().draw(*root_); }
	void save(FILE* fp) const { if (root_.get()) root_->save(fp); }
	void add(int sectorId, const Wall& w) { if (root_.get()) root_->add(sectorId, w); }

private:
	std::auto_ptr<Node> root_;
};

namespace { Tree tree; }

int bspInit() { tree.clear(); return 0; }
void bspDone() { tree.clear(); }
void bspBuildTree() { tree.build(); }
void bspShow() { tree.show(); }
int bspSave(FILE* f) { tree.save(f); return 0; }

int
bspAddLine(int s, int x1, int y1, int x2, int y2, int, int, int, int, int)
{
	tree.add(s, Wall(Vertex(x1, y1), Vertex(x2, y2)));
	return 0;
}

} // namespace bsp