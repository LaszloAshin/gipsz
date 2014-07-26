#include "bsp.h"
#include "ed.h"
#include "gr.h"

#include <vector>
#include <memory>
#include <algorithm>
#include <functional>

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

class Wall {
public:
	Wall(const Vertex& a, const Vertex& b) : a_(a), b_(b) {}

	Vertex a() const { return a_; }
	Vertex b() const { return b_; }

private:
	Vertex a_, b_;
};

class Sector;

struct Renderer {
	void draw(const Vertex& v) { edVertex(v.x(), v.y()); }
	void draw(const Vertex& a, const Vertex& b) { edVector(a.x(), a.y(), b.x(), b.y()); }
	void operator()(const Wall& w) { draw(w.a()); draw(w.b()); draw(w.a(), w.b()); }
	void operator()(const Sector& s);
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

private:
	int id_;
	Walls walls_;
};

void
Renderer::operator()(const Sector& s)
{
    grSetColor((s.id() * 343) | 3);
	std::for_each(s.begin(), s.end(), *this);
}

class Node {
public:
	void build() {}
	void show() const { std::for_each(sectors_.begin(), sectors_.end(), Renderer()); }
	void save(FILE*) const {}
	void add(int sectorId, const Wall& w);

private:
	typedef std::vector<Sector> Sectors;

	Sectors sectors_;
};

void
Node::add(int sectorId, const Wall& w)
{
	if (sectors_.empty() || sectors_.back().id() != sectorId) {
		sectors_.push_back(Sector(sectorId));
	}
	sectors_.back().add(w);
}

class Tree {
public:
	void clear() { root_.reset(new Node); }
	void build() { if(root_.get()) root_->build(); }
	void show() const { if (root_.get()) root_->show(); }
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
