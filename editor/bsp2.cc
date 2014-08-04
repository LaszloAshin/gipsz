#include "bsp.h"
#include "ed.h"
#include "gr.h"
#include "line.h"

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

typedef std::vector<Vertex> Vertexes;

void save(FILE* fp, const Vertex& v) {
	short x = round(v.x());
	short y = round(v.y());
	fwrite(&x, sizeof(x), 1, fp);
	fwrite(&y, sizeof(y), 1, fp);
}

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
	Wall(const Vertex& a, const Vertex& b, int tex = 0, int flags = Line::Flag::TWOSIDED)
	: a_(a), b_(b), tex_(tex), flags_(flags)
	{}

	Vertex a() const { return a_; }
	Vertex b() const { return b_; }
	int tex() const { return tex_; }
	int flags() const { return flags_; }

	void save(FILE* fp, const Vertexes& vs) const;

private:
	Vertex a_, b_;
	int tex_;
	int flags_;
};

std::ostream& operator<<(std::ostream& os, const Wall& w) { return os << "Wall(" << w.a() << ", " << w.b() << ")"; }

int getVertexIndex(const Vertexes& vs, const Vertex& v) {
	Vertexes::const_iterator i(std::find(vs.begin(), vs.end(), v));
	assert(i != vs.end());
	return std::distance(vs.begin(), i);
}

void Wall::save(FILE* fp, const Vertexes& vs) const {
	unsigned char buf[2 * 2 + 1 + 3 * 2 + 4], *p = buf;
	*(short *)p = getVertexIndex(vs, a());
	*(short *)(p + 2) = getVertexIndex(vs, b());
	p[4] = flags();
	*(unsigned short *)(p + 5) = 0;
	*(unsigned short *)(p + 7) = 64;
	*(unsigned short *)(p + 9) = 0;
	*(unsigned *)(p + 11) = tex();
	fwrite(buf, 1, sizeof(buf), fp);
}

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

	Sector partition(const Plane2d& plane) const;
	void checkWalls() const;
	void print(std::ostream& os) const;
	size_t countWalls() const { return walls_.size(); }
	bool empty() const { return walls_.empty(); }
	void collect(Vertexes& vs) const;
	void save(FILE* fp, const Vertexes& vs) const;

private:
	void checkWallsSub() const;
	void saveSorted(FILE* fp, const Vertexes& vs) const;

	int id_;
	Walls walls_;
};

void add(Vertexes& vs, const Vertex& v) {
	if (std::find(vs.begin(), vs.end(), v) == vs.end()) vs.push_back(v);
}

void Sector::collect(Vertexes& vs) const {
	for (Walls::const_iterator i(walls_.begin()); i != walls_.end(); ++i) {
		::bsp::add(vs, i->a());
		::bsp::add(vs, i->b());
	}
}

template <class T>
T pick(std::vector<T>& v, typename std::vector<T>::iterator i) {
	T result(*i);
	*i = v.back();
	v.pop_back();
	return result;
}

void Sector::saveSorted(FILE* fp, const Vertexes& vs) const {
	assert(walls_.size() >= 3);
	Walls ws(walls_);
	Vertex first, last;
	bool open = false;
	while (!ws.empty()) {
		assert(ws.front().a() != ws.front().b());
		if (!open) {
			assert(ws.size() >= 3);
			Wall w(pick(ws, ws.begin()));
			w.save(fp, vs);
			first = w.a();
			last = w.b();
			open = true;
			continue;
		}
		bool found = false;
		for (Walls::iterator i(ws.begin()); i != ws.end(); ++i) {
			if (i->a() == last) {
				Wall w(pick(ws, i));
				w.save(fp, vs);
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

void Sector::save(FILE* fp, const Vertexes& vs) const {
	std::cerr << "Sector::save()" << std::endl;
	{
		int wallCount = countWalls();
		fwrite(&wallCount, sizeof(int), 1, fp);
	}
	{
		int sectorId = id();
		fwrite(&sectorId, sizeof(int), 1, fp);
	}
	saveSorted(fp, vs);
}

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

Sector Sector::partition(const Plane2d& plane) const {
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
				result.add(Wall(isp, i->b(), i->tex(), i->flags()));
				++degs[i->b()];
			} else {
				result.add(Wall(i->a(), isp, i->tex(), i->flags()));
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
	void save(FILE*, const Vertexes& vs) const;
	void add(int sectorId, const Wall& w);
	Plane2d plane() const { return plane_; }
	size_t countWalls() const;
	void print(std::ostream& os) const;
	void collect(Vertexes& vs) const;
	size_t countNodes() const { return 1 + (front_.get() ? front_->countNodes() : 0) + (back_.get() ? back_->countNodes() : 0); }

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

void Node::collect(Vertexes& vs) const {
	for (Sectors::const_iterator i(sectors_.begin()); i != sectors_.end(); ++i) {
		i->collect(vs);
	}
	if (front_.get()) front_->collect(vs);
	if (back_.get()) back_->collect(vs);
}

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
/*	static int k = 0;
	++k;
	if (k >= 299) return;
	std::cerr << "k=" << k << std::endl;*/
	checkWalls();
	print(std::cerr);
	if (!findBestPlane()) return;
	std::cerr << "best plane is:";
	plane_.print(std::cerr);
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

void Node::save(FILE* fp, const Vertexes& vs) const {
	std::cerr << "Node::save()" << std::endl;
	const int isLeaf = !sectors_.empty();
	fwrite(&isLeaf, sizeof(int), 1, fp);
	if (isLeaf) {
		assert(!front_.get());
		assert(!back_.get());
		assert(sectors_.size() == 1);
		sectors_.front().save(fp, vs);
	} else {
		assert(front_.get());
		assert(back_.get());
		assert(sectors_.empty());
		plane_.save(fp);
		front_->save(fp, vs);
		back_->save(fp, vs);
	}
}

class Tree {
public:
	void clear() { root_.reset(new Node); }
	void build() { try{if (root_.get()) root_->build();}catch(...){} }
//	void build() { if (root_.get()) root_->build(); }
	void show() const { if (root_.get()) Renderer().draw(*root_); }
	void save(FILE* fp) const;
	void add(int sectorId, const Wall& w) { if (root_.get()) root_->add(sectorId, w); }

private:
	std::auto_ptr<Node> root_;
};

void Tree::save(FILE* fp) const {
	assert(root_.get());
	Vertexes vertexes;
	root_->collect(vertexes);
	{
		unsigned vertexCount = vertexes.size();
		std::cerr << vertexCount << " vertexes" << std::endl;
		fwrite(&vertexCount, sizeof(unsigned), 1, fp);
		for (Vertexes::const_iterator i(vertexes.begin()); i != vertexes.end(); ++i) {
			::bsp::save(fp, *i);
		}
	}
	unsigned nodeCount = root_->countNodes();
	std::cerr << nodeCount << " nodes" << std::endl;
	unsigned lineCount = root_->countWalls();
	std::cerr << lineCount << " lines" << std::endl;
	fwrite(&nodeCount, sizeof(unsigned), 1, fp);
	fwrite(&lineCount, sizeof(unsigned), 1, fp);
	root_->save(fp, vertexes);
}

namespace { Tree tree; }

int bspInit() { tree.clear(); return 0; }
void bspDone() { tree.clear(); }
void bspBuildTree() { tree.build(); }
void bspShow() { tree.show(); }
int bspSave(FILE* f) { tree.save(f); return 0; }

int
bspAddLine(int s, int x1, int y1, int x2, int y2, int, int, int flags, int tex, int)
{
	tree.add(s, Wall(Vertex(x1, y1), Vertex(x2, y2), tex, flags));
	return 0;
}

} // namespace bsp
