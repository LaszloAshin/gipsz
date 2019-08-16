#include "bsp.h"
#include "ed.h"
#include "gr.h"
#include "line.h"

#include <lib/persistency.hh>
#include <lib/plane.hh>
#include <lib/surface.hh>

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
#include <cmath>
#include <cassert>

namespace bsp {

static const double epsilon = std::numeric_limits<double>::epsilon();
static const double thickness = 0.1f;
//static const double epsilon = 0.00001f;

class Vertex;
double dot(const Vertex& lhs, const Vertex& rhs);

class Vertex {
public:
	Vertex() : x_(0.0f), y_(0.0f) {}
	Vertex(double x, double y) : x_(x), y_(y) {}
	Vertex(const Vec2d& v) : x_(v.x()), y_(v.y()) {}

	double x() const { return x_; }
	double y() const { return y_; }

	Vertex operator-() const { return Vertex(-x(), -y()); }
	Vertex& operator+=(const Vertex& rhs) { x_ += rhs.x(); y_ += rhs.y(); return *this; }
	Vertex& operator-=(const Vertex& rhs) {  return operator+=(-rhs); }
	double length() const { return sqrtf(dot(*this, *this)); }

private:
	double x_, y_;
};

std::ostream& operator<<(std::ostream& os, const Vertex& v) { return os << v.x() << ' ' << v.y(); }
Vertex operator+(Vertex lhs, const Vertex& rhs) { return lhs += rhs; }
Vertex operator-(Vertex lhs, const Vertex& rhs) { return lhs -= rhs; }

double dot(const Vertex& lhs, const Vertex& rhs) { return lhs.x() * rhs.x() + lhs.y() * rhs.y(); }

inline bool operator==(Vertex d, const Vertex& rhs) {
	d -= rhs;
	return d.x() > -epsilon && d.x() < epsilon && d.y() > -epsilon && d.y() < epsilon;
}

inline bool operator<(const Vertex& lhs, Vertex d) {
	d -= lhs;
	if (d.y() < -epsilon) return true;
	if (d.y() > epsilon) return false;
	return d.x() < -epsilon;
}

inline bool operator!=(const Vertex& lhs, const Vertex& rhs) { return !(lhs == rhs); }

class Wall {
public:
	Wall(const Vertex& a, const Vertex& b, int backSectorId = 0, const Surfaced& surface = Surfaced(), int flags = Line::Flag::TWOSIDED)
	: a_(a), b_(b), backSectorId_(backSectorId), surface_(surface), flags_(flags)
	{}

	Vertex a() const { return a_; }
	Vertex b() const { return b_; }
	int backSectorId() const { return backSectorId_; }
	Surfaced surface() const { return surface_; }
	int flags() const { return flags_; }

	void save(std::ostream& os) const;

private:
	Vertex a_, b_;
	int backSectorId_;
	Surfaced surface_;
	int flags_;
};

std::ostream& operator<<(std::ostream& os, const Wall& w) { return os << "Wall(" << w.a() << ", " << w.b() << ")"; }

void Wall::save(std::ostream& os) const {
	os << "wall ";
	os << a() << ' ';
	os << b() << ' ';
	os << flags() << ' ';
	os << backSectorId() << std::endl;
	os << surface() << std::endl;
}

class Sector;
class Node;

struct Renderer {
	void draw(const Vertex& v) { edVertex(v.x(), v.y()); }
	void draw(const Vertex& a, const Vertex& b) { edVector(a.x(), a.y(), b.x(), b.y()); }
	void operator()(const Wall& w) { draw(w.a()); draw(w.b()); draw(w.a(), w.b()); }
	void operator()(const Sector& s);
	void draw(const Node& n);
};

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
		return abs(static_cast<ptrdiff_t>(front()) - static_cast<ptrdiff_t>(back())) + splitted() * 10;
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

	Sector partition(const Plane2d& plane) const;
	void checkWalls() const;
	void print(std::ostream& os) const;
	size_t countWalls() const { return walls_.size(); }
	bool empty() const { return walls_.empty(); }
	void save(std::ostream& os) const;

private:
	void checkWallsSub() const;
	void saveSorted(std::ostream& os) const;

	int id_;
	Walls walls_;
};

template <class T>
T pick(std::vector<T>& v, typename std::vector<T>::iterator i) {
	T result(*i);
	*i = v.back();
	v.pop_back();
	return result;
}

void Sector::saveSorted(std::ostream& os) const {
	if (walls_.size() < 3) {
		std::cerr << walls_.size() << std::endl;
		throw std::runtime_error("too few walls in a sector");
	}
	Walls ws(walls_);
	Vertex first, last;
	bool open = false;
	while (!ws.empty()) {
		assert(ws.front().a() != ws.front().b());
		if (!open) {
			assert(ws.size() >= 3);
			Wall w(pick(ws, ws.begin()));
			w.save(os);
			first = w.a();
			last = w.b();
			open = true;
			continue;
		}
		bool found = false;
		for (Walls::iterator i(ws.begin()); i != ws.end(); ++i) {
			if (i->a() == last) {
				Wall w(pick(ws, i));
				w.save(os);
				last = w.b();
				if (last == first) open = false;
				found = true;
				break;
			}
		}
		if (!found) throw std::runtime_error("open sector");
	}
	if (open) throw std::runtime_error("open sector at the end");
}

void Sector::save(std::ostream& os) const {
	os << "subsector " << id() << ' ' << countWalls() << std::endl;
	saveSorted(os);
}

SplitStats Sector::computeStats(const Plane2d& plane) const {
	SplitStats result;
	for (const_iterator i(begin()); i != end(); ++i) {
		const double da = plane.determine(i->a());
		const double db = plane.determine(i->b());
		const int ia = (da < -0.5f * thickness) ? -1 : ((da > 0.5f * thickness) ? 1 : 0);
		const int ib = (db < -0.5f * thickness) ? -1 : ((db > 0.5f * thickness) ? 1 : 0);
		if (ia * ib < 0) {
			result.wallIsSplit();
		} else if (ia + ib > 0) {
			result.wallIsFront();
		} else if (ia + ib < 0) {
			result.wallIsBack();
		}
	}
	return result;
}

Sector Sector::partition(const Plane2d& plane) const {
	typedef std::map<Vertex, size_t> Degs;
	Degs degs;
	Sector result(id());
	for (const_iterator i(begin()); i != end(); ++i) {
		const double da = plane.determine(i->a());
		const double db = plane.determine(i->b());
		const int ia = (da < -0.5f * thickness) ? -1 : ((da > 0.5f * thickness) ? 1 : 0);
		const int ib = (db < -0.5f * thickness) ? -1 : ((db > 0.5f * thickness) ? 1 : 0);
		if (ia * ib < 0) {
			const Vertex isp(intersect(plane, Plane2d(i->a(), i->b())));
			const double di = plane.determine(isp);
			std::cerr << "di = " << di << " epsilon = " << epsilon << std::endl;
			if (di > thickness / 2.0f || di < -thickness / 2.0f) throw std::runtime_error("intersection point is not on the intersection plane");
			Vertex a(i->a());
			Vertex b(i->b());
			Surfaced sface(i->surface());
			if (ia < 0) {
				a = isp;
				sface.cropLeft(da / (da - db));
			} else {
				b = isp;
				sface.cropRight(db / (db - da));
			}
			result.add(Wall(a, b, i->backSectorId(), sface, i->flags()));
			++degs[a], ++degs[b];
		} else if (ia + ib > 0) {
			result.add(*i);
			++degs[i->a()], ++degs[i->b()];
		} else if (ia + ib >= 0 && dot(plane, i->b() - i->a()) < 0) {
			result.add(*i);
			++degs[i->a()], ++degs[i->b()];
		}
	}
	Vertex last;
	bool open = false;
	for (Degs::const_iterator i(degs.begin()); i != degs.end(); ++i) {
		if (!(i->second & 1)) continue;
		if (open) {
			result.add((dot(plane, last - i->first) < 0) ? Wall(i->first, last, id()) : Wall(last, i->first, id()));
		} else {
			last = i->first;
		}
		open = !open;
	}
	if (open) throw std::runtime_error("open sector after split");
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
	void save(std::ostream& os) const;
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
//	draw(n.plane());
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
/*	static int k = 0;
	++k;
	if (k >= 299) return;
	std::cerr << "k=" << k << std::endl;*/
	checkWalls();
	print(std::cerr);
	if (!findBestPlane()) return;
	std::cerr << "best plane is: ";
	plane_.write(std::cerr);
	std::cerr << std::endl;
	{
		std::auto_ptr<Node> front(new Node);
		std::auto_ptr<Node> back(new Node);
		for (Sectors::const_iterator i(sectors_.begin()); i != sectors_.end(); ++i) {
			const Sector frontSlice(i->partition(plane_));
			if (!frontSlice.empty()) front->sectors_.push_back(frontSlice);
			const Sector backSlice(i->partition(-plane_));
			if (!backSlice.empty()) back->sectors_.push_back(backSlice);
		}
		if (!front->countWalls() || !back->countWalls()) {
			throw std::runtime_error("ough");
			return;
		}
		front_.reset(front.release());
		back_.reset(back.release());
	}
	sectors_.clear();
/*	grBegin();
	Renderer().draw(*front_);
	grEnd();*/
	front_->build();
	back_->build();
}

size_t Node::countWalls() const {
	size_t result = 0;
	for (Sectors::const_iterator i(sectors_.begin()); i != sectors_.end(); ++i) {
		result += i->countWalls();
	}
	if (front_.get()) result += front_->countWalls();
	if (back_.get()) result += back_->countWalls();
	return result;
}

void Node::print(std::ostream& os) const {
	os << "Node" << std::endl;
	for (Sectors::const_iterator i(sectors_.begin()); i != sectors_.end(); ++i) {
		i->print(os);
	}
}

void Node::save(std::ostream& os) const {
	const int isLeaf = !sectors_.empty();
	if (isLeaf) {
		os << "node leaf" << std::endl;
		assert(!front_.get());
		assert(!back_.get());
		if (sectors_.size() != 1) throw std::runtime_error("no single sector in node");
		sectors_.front().save(os);
	} else {
		os << "node branch" << std::endl;
		assert(front_.get());
		assert(back_.get());
		assert(sectors_.empty());
		plane_.write(os);
		front_->save(os);
		back_->save(os);
	}
}

class Tree {
public:
	void clear() { root_.reset(new Node); }
	void build() { try{if (root_.get()) root_->build();}catch(const std::exception& e){std::cerr << e.what() << std::endl;} }
//	void build() { if (root_.get()) root_->build(); }
	void show() const { if (root_.get()) Renderer().draw(*root_); }
	void save(std::ostream& os) const { try{trySave(os);}catch(const std::exception& e){std::cerr << e.what() << std::endl;} }
	void add(int sectorId, const Wall& w) { if (root_.get()) root_->add(sectorId, w); }

private:
	void trySave(std::ostream& os) const;

	std::auto_ptr<Node> root_;
};

void Tree::trySave(std::ostream& os) const {
	assert(root_.get());
	root_->save(os);
}

namespace { Tree tree; }

int bspInit() { tree.clear(); return 0; }
void bspDone() { tree.clear(); }
void bspBuildTree() { tree.build(); }
void bspShow() { tree.show(); }
int bspSave(std::ostream& os) { tree.save(os); return 0; }

int
bspAddLine(int sf, int sb, int x1, int y1, int x2, int y2, int u, int v, int flags, int tex, int du)
{
	const Vertex a(x1, y1);
	const Vertex b(x2, y2);
	tree.add(sf, Wall(a, b, sb, Surfaced(tex, u / 64.0f, (u + (b - a).length() + du) / 64.0f, v / 64.0f), flags));
	return 0;
}

} // namespace bsp
