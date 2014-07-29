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
#include <iterator>
#include <map>

namespace bsp {

class Vertex {
public:
	Vertex() : x_(0.0f), y_(0.0f) {}
	Vertex(float x, float y) : x_(x), y_(y) {}

	float x() const { return x_; }
	float y() const { return y_; }

	Vertex operator-() const { return Vertex(-x(), -y()); }
	Vertex& operator+=(const Vertex& rhs) { x_ += rhs.x(); y_ += rhs.y(); return *this; }
	Vertex& operator-=(const Vertex& rhs) {  return operator+=(-rhs); }

private:
	float x_, y_;
};

std::ostream& operator<<(std::ostream& os, const Vertex& v) { return os << "(" << v.x() << ", " << v.y() << ")"; }
Vertex operator+(Vertex lhs, const Vertex& rhs) { return lhs += rhs; }
Vertex operator-(Vertex lhs, const Vertex& rhs) { return lhs -= rhs; }

inline bool operator==(Vertex d, const Vertex& rhs) {
	d -= rhs;
	const float e = std::numeric_limits<float>::epsilon();
	return d.x() > -e && d.x() < e && d.y() > -e && d.y() < e;
}

inline bool operator<(const Vertex& lhs, Vertex d) {
	d -= lhs;
	const float e = std::numeric_limits<float>::epsilon();
	if (d.y() < -e) return true;
	if (d.y() > e) return false;
	return d.x() < -e;
}

inline bool operator!=(const Vertex& lhs, const Vertex& rhs) { return !(lhs == rhs); }

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
	Plane2d operator-() const { return Plane2d(-a_, -b_, -c_); }

private:
	Plane2d(float a, float b, float c) : a_(a), b_(b), c_(c) {}

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

std::ostream& operator<<(std::ostream& os, const Wall& w) { return os << "Wall(" << w.a() << ", " << w.b() << ")"; }

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
	SplitStats() : front_(), back_(), splitted_() {}
	SplitStats(size_t front, size_t back, size_t splitted) : front_(front), back_(back), splitted_(splitted) {}

	void wallIsFront() { ++front_; }
	void wallIsBack() { ++back_; }
	void wallIsSplit() { ++splitted_; wallIsFront(); wallIsBack(); }

	size_t front() const { return front_; }
	size_t back() const { return back_; }
	size_t splitted() const { return splitted_; }

	int
	score()
	const
	{
		if (!front() || !back()) return -1;
		return abs(front() - back()) + splitted() * 10;
	}

	SplitStats&
	operator+=(const SplitStats& rhs)
	{
		front_ += rhs.front();
		back_ += rhs.back();
		splitted_ += rhs.splitted();
		return *this;
	}

	void
	print(std::ostream& os)
	const
	{
		os << "SplitStats(f=" << front() << " b=" << back() << " sp=" << splitted() << " sc=" << score() << ")";
	}

private:
	size_t front_;
	size_t back_;
	size_t splitted_;
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

	Sector slice(const Plane2d& plane) const;
	void checkWalls() const;
	void print(std::ostream& os) const;
	size_t countWalls() const { return walls_.size(); }
	bool empty() const { return walls_.empty(); }

private:
	void checkWallsSub() const;

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

Sector Sector::slice(const Plane2d& plane) const {
	typedef std::map<Vertex, size_t> Degs;
	Degs degs;
	Sector result(id());
	for (const_iterator i(begin()); i != end(); ++i) {
		const float da = plane.determine(i->a());
		const float db = plane.determine(i->b());
		if (da * db < 0) {
			const Vertex isp(intersect(plane, Plane2d(i->a(), i->b())));
			++degs[isp];
			if (da < 0) {
				result.add(Wall(isp, i->b()));
				++degs[i->b()];
			} else {
				result.add(Wall(i->a(), isp));
				++degs[i->a()];
			}
		} else if (da + db > std::numeric_limits<float>::epsilon()) {
			result.add(*i);
			++degs[i->a()], ++degs[i->b()];
		} else if (da + db > -std::numeric_limits<float>::epsilon()) {
			if (plane.dot(i->b() - i->a()) < 0) {
				result.add(*i);
				++degs[i->a()], ++degs[i->b()];
			}
		}
	}
	Vertex last;
	bool open = false;
	for (Degs::const_iterator i(degs.begin()); i != degs.end(); ++i) {
		if (!(i->second & 1)) continue;
		if (open) {
			result.add((plane.dot(last - i->first) < 0) ? Wall(i->first, last) : Wall(last, i->first));
		} else {
			last = i->first;
		}
		open = !open;
	}
	if (open) throw std::runtime_error("open sector after split");
	return result;
}

template <class T>
T pick(std::vector<T>& v, typename std::vector<T>::iterator i) {
	T result(*i);
	*i = v.back();
	v.pop_back();
	return result;
}

void Sector::checkWallsSub() const {
	const bool debug = false;
	Walls w(walls_);
	Vertex first, last;
	bool open = false;
	if (debug) std::cerr << "sector #" << id() << std::endl;
	while (!w.empty()) {
		if (w.front().a() == w.front().b()) throw std::runtime_error("zero-length wall detected");
		if (!open) {
			if (w.size() < 3) throw std::runtime_error("every sector must have three walls at least");
			Wall wall(pick(w, w.begin()));
			first = wall.a();
			last = wall.b();
			if (debug) std::cerr << "new first: " << first << std::endl;
			if (debug) std::cerr << "add: " << wall << std::endl;
			open = true;
			continue;
		}
		bool found = false;
		for (Walls::iterator i(w.begin()); i != w.end(); ++i) {
			if (i->a() == last) {
				Wall wall(pick(w, i));
				last = wall.b();
				if (last == first) open = false;
				if (debug) std::cerr << "add: " << wall << std::endl;
				found = true;
				break;
			}
		}
		if (!found) throw std::runtime_error("open sector");
	}
	if (open) throw std::runtime_error("open sector at the end");
	if (debug) std::cerr << "sector done" << std::endl;
}

void Sector::checkWalls() const {
	try {
		checkWallsSub();
	} catch (const std::exception& e) {
		std::cerr << "checking walls failed for a sector: " << e.what() << std::endl;
		std::cerr << "Sector details:" << std::endl;
		print(std::cerr);
		throw;
	}
}

void Sector::print(std::ostream& os) const {
	os << "Sector(id=" << id() << ")" << std::endl;
	std::copy(walls_.begin(), walls_.end(), std::ostream_iterator<Wall>(os, "\n"));
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

	Node() {}

	const_iterator begin() const { return sectors_.begin(); }
	const_iterator end() const { return sectors_.end(); }
	bool hasFront() const { return front_.get(); }
	bool hasBack() const { return back_.get(); }
	Node& front() { return *front_; }
	Node& back() { return *back_; }
	const Node& front() const { return *front_; }
	const Node& back() const{ return *back_; }

	void build();
	void save(FILE*) const {}
	void add(int sectorId, const Wall& w);
	Plane2d plane() const { return plane_; }
	size_t countWalls() const;
	void print(std::ostream& os) const;

private:
	Node(const Node&);
	Node& operator=(const Node&);

	SplitStats computeStats(const Plane2d& plane) const;
	bool findBestPlane();
	void checkWalls() const { std::for_each(sectors_.begin(), sectors_.end(), std::mem_fun_ref(&Sector::checkWalls)); }

	Sectors sectors_;
	Plane2d plane_;
	std::auto_ptr<Node> front_, back_;
};

void
Renderer::draw(const Node& n)
{
	std::for_each(n.begin(), n.end(), *this);
	if (n.hasFront()) draw(n.front());
	if (n.hasBack()) draw(n.back());
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

bool
Node::findBestPlane()
{
	bool bestFound = false;
	SplitStats bestStat;

	for (Sectors::const_iterator i(sectors_.begin()); i != sectors_.end(); ++i) {
		for (Sector::const_iterator j(i->begin()); j != i->end(); ++j) {
			const Plane2d p(j->a(), j->b());
			const SplitStats s(computeStats(p));
			p.print(std::cerr);
			s.print(std::cerr);
			std::cerr << std::endl;
			if (s.score() < 0) continue;
			if (!bestFound || s.score() < bestStat.score()) {
				bestFound = true;
				plane_ = p;
				bestStat = s;
			}
		}
	}
	return bestFound;
}

void
Node::build()
{
//	static int k = 0;
	checkWalls();
//	std::cerr << "k=" << k << std::endl;
	print(std::cerr);
	if (!findBestPlane()) return;
	std::cerr << "best plane is:";
	plane_.print(std::cerr);
	std::cerr << std::endl;
	{
		std::auto_ptr<Node> front(new Node);
		std::auto_ptr<Node> back(new Node);
		for (Sectors::const_iterator i(sectors_.begin()); i != sectors_.end(); ++i) {
			const Sector frontSlice(i->slice(plane_));
			if (!frontSlice.empty()) front->sectors_.push_back(frontSlice);
			const Sector backSlice(i->slice(-plane_));
			if (!backSlice.empty()) back->sectors_.push_back(backSlice);
		}
		if (!front->countWalls() || !back->countWalls()) {
			throw std::runtime_error("ough");
			return;
		}
//		if (k++ >= 9) return;
		front_.reset(front.release());
//		back_.reset(back.release());
	}
	sectors_.clear();
/*	grBegin();
	Renderer().draw(*front_);
	grEnd();*/
	front_->build();
//	back_->build();
}

size_t Node::countWalls() const {
	size_t result = 0;
	for (Sectors::const_iterator i(sectors_.begin()); i != sectors_.end(); ++i) {
		result += i->countWalls();
	}
	return result;
}

void Node::print(std::ostream& os) const {
	os << "Node" << std::endl;
	for (Sectors::const_iterator i(sectors_.begin()); i != sectors_.end(); ++i) {
		i->print(os);
	}
}

class Tree {
public:
	void clear() { root_.reset(new Node); }
	void build() { try{if (root_.get()) root_->build();}catch(...){} }
//	void build() { if (root_.get()) root_->build(); }
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
