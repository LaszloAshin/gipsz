#include <game/aabb.hh>
#include <game/player.h>

bool
visibleByCamFrustum(const Aabb3d& bb)
{
	const Vec3d lo(bb.low() - cam.pos());
	const Vec3d hi(bb.high() - cam.pos());
	const Vec3d p[8] = {
		Vec3d(lo.x(), lo.y(), hi.z()), Vec3d(hi.x(), lo.y(), hi.z()),
		Vec3d(hi.x(), hi.y(), hi.z()), Vec3d(lo.x(), hi.y(), hi.z()),
		Vec3d(lo.x(), lo.y(), lo.z()), Vec3d(hi.x(), lo.y(), lo.z()),
		Vec3d(hi.x(), hi.y(), lo.z()), Vec3d(lo.x(), hi.y(), lo.z())
	};
	for (int i = 0; i < 5; ++i) {
		Vec3d v = -((i == 4) ? cam.forward : cam.cps[i]);
		int k = 8;
		for (int j = 0; j < 8; ++j) if (dot(p[j], v) > 0) --k;
		if (!k) return false;
	}
	return true;
}
