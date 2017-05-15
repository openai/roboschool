#include "household.h"
#include "assets.h"
#include "render-simple.h"

namespace Household {

#if 0
static const float TEST_GAP = 0.00; // see what walls and floor are made of, set 0.03
static inline float sign(float x)  { return fabs(x)<0.0001 ? 0 : (x>0 ? +1:-1); }
static inline btVector3 sign(btVector3 v)  { return btVector3(sign(v.x()), sign(v.y()), sign(v.z())); }

shared_ptr<Material> World::texture_from_file_cached(const std::string& texid)
{
	if (texid.empty()) return shared_ptr<Material>();
	if (!textures_cache) textures_cache.reset(new MaterialNamespace);
	auto f = textures_cache->name2mtl.find(texid);
	if (f!=textures_cache->name2mtl.end())
		return f->second;
	shared_ptr<Material> mat(new Material(texid));
	mat->diffuse_color = 0x0000FF; // default color for failed-to-load materials
	if (texid[0]=='#') { // color, like "#FF0000" for red
		assert("color should be #rrggbb" && texid.size()==7);
		int parsed = sscanf(texid.substr(1).c_str(), "%x", &mat->diffuse_color);
		assert(parsed==1);
	} else try {
		std::string mtl_fn;
		std::string mtl_name;
		std::string::size_type f = texid.find(":");
		if (f==std::string::npos) {
			mtl_fn = texid;
		} else {
			mtl_fn = texid.substr(0, f);
			mtl_name = texid.substr(f+1);
		}
		shared_ptr<ShapeDetailLevels> dummy(new ShapeDetailLevels);
		load_model(dummy, mtl_fn, 1.0, false);

		assert(!dummy->materials->name2mtl.empty());
		if (mtl_name.empty()) {
			mat = dummy->materials->name2mtl.begin()->second;
		} else {
			auto i = dummy->materials->name2mtl.find(mtl_name);
			if (i==dummy->materials->name2mtl.end())
				throw std::runtime_error("no '" + mtl_name + "' inside '" + mtl_fn + "'");
			mat = i->second;
		}

	} catch (const std::exception& e) {
		fprintf(stderr, "material file '%s' cannot be loaded: %s\n", texid.c_str(), e.what());
	}

	textures_cache->name2mtl[texid] = mat;
	return mat;
}

shared_ptr<Thingy> World::tool_wall(
	btScalar grid_size, float tex1, float tex_v_zero,
	const std::vector<btScalar>& path, bool closed,
	const std::vector<btScalar>& low,
	const std::vector<btScalar>& hgh,
	const std::string& side_tex_,   float side_tex_scale,
	const std::string& top_tex_,    float top_tex_scale,
	const std::string& bottom_tex_, float bott_tex_scale,
	const std::string& butt_tex_,   float butt_tex_scale)
{
	shared_ptr<Household::Shape> side_shape;
	shared_ptr<Household::Shape> side_cshape;
	if (!side_tex_.empty()) {
		side_shape.reset(new Household::Shape);
		side_shape->material = texture_from_file_cached(side_tex_);
		side_cshape.reset(new Household::Shape);
		side_cshape->primitive_type = Household::Shape::STATIC_MESH;
	}

	shared_ptr<Household::Shape> top_shape;
	shared_ptr<Household::Shape> top_cshape;
	if (!top_tex_.empty()) {
		top_shape.reset(new Household::Shape);
		top_shape->material = texture_from_file_cached(top_tex_);
		top_cshape.reset(new Household::Shape);
		top_cshape->primitive_type = Household::Shape::STATIC_MESH;
	}

	shared_ptr<Household::Shape> bot_shape;
	shared_ptr<Household::Shape> bot_cshape;
	if (!bottom_tex_.empty()) {
		bot_shape.reset(new Household::Shape);
		bot_shape->material = texture_from_file_cached(bottom_tex_);
		bot_cshape.reset(new Household::Shape);
		bot_cshape->primitive_type = Household::Shape::STATIC_MESH;
	}

	shared_ptr<Household::Shape> but_shape;
	shared_ptr<Household::Shape> but_cshape;
	if (!butt_tex_.empty()) {
		but_shape.reset(new Household::Shape);
		but_shape->material = texture_from_file_cached(butt_tex_);
		but_cshape.reset(new Household::Shape);
		but_cshape->primitive_type = Household::Shape::STATIC_MESH;
	}

	int cnt = low.size();
	assert(cnt==(int)hgh.size());
	assert((cnt+(closed?0:1))*2==(int)path.size());

	std::vector<btScalar> path_outer;
	std::vector<btScalar> path_inner;
	std::vector<btScalar> low_expanded;
	std::vector<btScalar> hgh_expanded;
	std::vector<bool> skip_top_bottom;
	std::vector<btVector3> normals;
	for (int c=0; c<cnt; c++) {
		int c0 = (c+cnt-1) % cnt;
		int c1 = (c+0) % cnt;
		int c2 = (c+1) % cnt;
		btVector3 hor0(path[2*c1+0] - path[2*c0+0], path[2*c1+1] - path[2*c0+1], 0);
		btVector3 hor1(path[2*c2+0] - path[2*c1+0], path[2*c2+1] - path[2*c1+1], 0);
		btVector3 ver(0, 0, -1);
		btVector3 norm0 = hor0.cross(ver);
		btVector3 norm1 = hor1.cross(ver);
		norm0.normalize();
		bool norm0_45deg = fabs(fabs(norm0.x()) - fabs(norm0.y())) < 0.01;
		norm1.normalize();
		bool norm1_45deg = fabs(fabs(norm1.x()) - fabs(norm1.y())) < 0.01;
		if (!closed && c==0) norm0 = norm1;
		if (!closed && c==cnt-1) norm1 = norm0;
		btVector3 inwards(0, 0, 0);
		btVector3 outwards;

		if (!norm0_45deg && !norm1_45deg) {
			if (hor0==-hor1)
				outwards = hor0;
			else
				outwards = norm0 + norm1;
			if (!outwards.fuzzyZero()) {
				outwards.normalize();
				btScalar correction = 1 / std::max( fabs(outwards.x()), fabs(outwards.y()) );
				outwards *= correction;
			}
			if (!(norm0 - norm1).fuzzyZero() && norm0.dot(hor1) > 0) // 90 degree turn, paint top/bottom less
				inwards = btVector3(outwards);

		} else if (norm0_45deg && norm1_45deg) {
			outwards = btVector3(0,0,0);

		} else if (norm0_45deg) {
			btScalar reverse = ((norm1.dot(hor0)) > 0) ? +1 : -1;
			outwards = hor0*reverse;
			outwards.normalize();
			btScalar correction = 1 / std::max( fabs(outwards.x()), fabs(outwards.y()) );
			outwards *= correction;

		} else if (norm1_45deg) {
			btScalar reverse = ((norm0.dot(hor1)) > 0) ? +1 : -1;
			outwards = hor1*reverse;
			outwards.normalize();
			btScalar correction = 1 / std::max( fabs(outwards.x()), fabs(outwards.y()) );
			outwards *= correction;
		}

		path_outer.push_back(path[2*c1+0] + grid_size*0.5*outwards.x());
		path_outer.push_back(path[2*c1+1] + grid_size*0.5*outwards.y());
		path_inner.push_back(path[2*c1+0] + grid_size*0.5*inwards.x());
		path_inner.push_back(path[2*c1+1] + grid_size*0.5*inwards.y());
		low_expanded.push_back(low[c1]);
		hgh_expanded.push_back(hgh[c1]);
		skip_top_bottom.push_back(norm1_45deg);
		normals.push_back(norm1);

		bool height_change = hgh[c1] != hgh[c2] || low[c1] != low[c2];
		if (height_change) {
			//assert( (norm0 - norm1).fuzzyZero() && "cannot change height and turn simultaniously");
			path_outer.push_back( (path[2*c1+0] + path[2*c2+0])*0.5 + grid_size*0.5*norm1.x());
			path_outer.push_back( (path[2*c1+1] + path[2*c2+1])*0.5 + grid_size*0.5*norm1.y());
			path_inner.push_back( (path[2*c1+0] + path[2*c2+0])*0.5 );
			path_inner.push_back( (path[2*c1+1] + path[2*c2+1])*0.5 );
			low_expanded.push_back(low[c1]);
			hgh_expanded.push_back(hgh[c1]);
			skip_top_bottom.push_back(norm1_45deg);
			normals.push_back(norm1);

			path_outer.push_back( (path[2*c1+0] + path[2*c2+0])*0.5 + grid_size*0.5*norm1.x());
			path_outer.push_back( (path[2*c1+1] + path[2*c2+1])*0.5 + grid_size*0.5*norm1.y());
			path_inner.push_back( (path[2*c1+0] + path[2*c2+0])*0.5 );
			path_inner.push_back( (path[2*c1+1] + path[2*c2+1])*0.5 );
			low_expanded.push_back(low[c2]);
			hgh_expanded.push_back(hgh[c2]);
			skip_top_bottom.push_back(norm1_45deg);
			normals.push_back(norm1);
		}
	}

	float side_tex_spent = 0;
	cnt = low_expanded.size();
	for (int c=0; c<cnt; c++) {
		int c1 = (c+0) % cnt;
		int c2 = (c+1) % cnt;
		btVector3 norm = normals[c1];

		bool height_only = (btVector3(path_outer[2*c1+0], path_outer[2*c1+1], 0) - btVector3(path_outer[2*c2+0], path_outer[2*c2+1], 0)).fuzzyZero();

		if (side_shape && !height_only) {
			btVector3 p1(path_outer[2*c1+0], path_outer[2*c1+1], low_expanded[c1]);
			btVector3 p2(path_outer[2*c1+0], path_outer[2*c1+1], hgh_expanded[c1]);
			btVector3 p3(path_outer[2*c2+0], path_outer[2*c2+1], hgh_expanded[c1]);
			btVector3 p4(path_outer[2*c2+0], path_outer[2*c2+1], low_expanded[c1]);
			btVector3 center = (p1+p2+p3+p4)/4;
			if (!(p1-p2).fuzzyZero()) { // if zero, wall has no height, that's normal (for doorways)
				if (TEST_GAP) {
					p1 -= sign(p1-center)*TEST_GAP;
					p2 -= sign(p2-center)*TEST_GAP;
					p3 -= sign(p3-center)*TEST_GAP;
					p4 -= sign(p4-center)*TEST_GAP;
				}
				side_shape->push_vertex(p1.x(), p1.y(), p1.z());
				side_shape->push_vertex(p2.x(), p2.y(), p2.z());
				side_shape->push_vertex(p3.x(), p3.y(), p3.z());
				side_shape->push_vertex(p4.x(), p4.y(), p4.z());
				side_cshape->push_vertex(p1.x(), p1.y(), p1.z());
				side_cshape->push_vertex(p2.x(), p2.y(), p2.z());
				side_cshape->push_vertex(p3.x(), p3.y(), p3.z());
				side_cshape->push_vertex(p1.x(), p1.y(), p1.z());
				side_cshape->push_vertex(p3.x(), p3.y(), p3.z());
				side_cshape->push_vertex(p4.x(), p4.y(), p4.z());
				for (int c=0; c<4; c++)
					side_shape->push_normal(norm.x(), norm.y(), norm.z());
				for (int c=0; c<6; c++)
					side_cshape->push_normal(norm.x(), norm.y(), norm.z());
				float next_side_tex_spent = side_tex_spent + (p4-p1).norm();
				side_shape->push_tex(side_tex_spent/side_tex_scale/tex1,      (low_expanded[c1]-tex_v_zero)/side_tex_scale/tex1);
				side_shape->push_tex(side_tex_spent/side_tex_scale/tex1,      (hgh_expanded[c1]-tex_v_zero)/side_tex_scale/tex1);
				side_shape->push_tex(next_side_tex_spent/side_tex_scale/tex1, (hgh_expanded[c1]-tex_v_zero)/side_tex_scale/tex1);
				side_shape->push_tex(next_side_tex_spent/side_tex_scale/tex1, (low_expanded[c1]-tex_v_zero)/side_tex_scale/tex1);
				side_tex_spent = next_side_tex_spent;
			}
		}

		if (top_shape && !skip_top_bottom[c] && hgh_expanded[c1]==hgh_expanded[c2] && !height_only) {
			btVector3 p1(path_outer[2*c2+0], path_outer[2*c2+1], hgh_expanded[c1]);
			btVector3 p2(path_outer[2*c1+0], path_outer[2*c1+1], hgh_expanded[c1]);
			btVector3 p3(path_inner[2*c1+0], path_inner[2*c1+1], hgh_expanded[c1]);
			btVector3 p4(path_inner[2*c2+0], path_inner[2*c2+1], hgh_expanded[c1]);
			btVector3 center = (p1+p2+p3+p4)/4;
			if (TEST_GAP) {
				p1 -= sign(p1-center)*TEST_GAP;
				p2 -= sign(p2-center)*TEST_GAP;
				p3 -= sign(p3-center)*TEST_GAP;
				p4 -= sign(p4-center)*TEST_GAP;
			}
			top_shape->push_vertex(p1.x(), p1.y(), p1.z());
			top_shape->push_vertex(p2.x(), p2.y(), p2.z());
			top_shape->push_vertex(p3.x(), p3.y(), p3.z());
			top_shape->push_vertex(p4.x(), p4.y(), p4.z());
			top_cshape->push_vertex(p1.x(), p1.y(), p1.z());
			top_cshape->push_vertex(p2.x(), p2.y(), p2.z());
			top_cshape->push_vertex(p3.x(), p3.y(), p3.z());
			top_cshape->push_vertex(p1.x(), p1.y(), p1.z());
			top_cshape->push_vertex(p3.x(), p3.y(), p3.z());
			top_cshape->push_vertex(p4.x(), p4.y(), p4.z());
			for (int c=0; c<4; c++)
				top_shape->push_normal(0,0,1);
			for (int c=0; c<6; c++)
				top_cshape->push_normal(0,0,1);
			float next_side_tex_spent = side_tex_spent - (p2-p1).norm();
			btScalar depth = (p2-p3).norm();
			top_shape->push_tex( side_tex_spent/top_tex_scale/tex1,      hgh_expanded[c1]/top_tex_scale/tex1 );
			top_shape->push_tex( next_side_tex_spent/top_tex_scale/tex1, hgh_expanded[c1]/top_tex_scale/tex1 );
			top_shape->push_tex( next_side_tex_spent/top_tex_scale/tex1, (hgh_expanded[c1] + depth)/top_tex_scale/tex1 );
			top_shape->push_tex( side_tex_spent/top_tex_scale/tex1,      (hgh_expanded[c1] + depth)/top_tex_scale/tex1 );
		}

		if (bot_shape && !skip_top_bottom[c] && low_expanded[c1]==low_expanded[c2] && !height_only) {
			btVector3 p1(path_outer[2*c2+0], path_outer[2*c2+1], low_expanded[c1]);
			btVector3 p2(path_inner[2*c2+0], path_inner[2*c2+1], low_expanded[c1]);
			btVector3 p3(path_inner[2*c1+0], path_inner[2*c1+1], low_expanded[c1]);
			btVector3 p4(path_outer[2*c1+0], path_outer[2*c1+1], low_expanded[c1]);
			btVector3 center = (p1+p2+p3+p4)/4;
			if (TEST_GAP) {
				p1 -= sign(p1-center)*TEST_GAP;
				p2 -= sign(p2-center)*TEST_GAP;
				p3 -= sign(p3-center)*TEST_GAP;
				p4 -= sign(p4-center)*TEST_GAP;
			}
			bot_shape->push_vertex(p1.x(), p1.y(), p1.z());
			bot_shape->push_vertex(p2.x(), p2.y(), p2.z());
			bot_shape->push_vertex(p3.x(), p3.y(), p3.z());
			bot_shape->push_vertex(p4.x(), p4.y(), p4.z());
			bot_cshape->push_vertex(p1.x(), p1.y(), p1.z());
			bot_cshape->push_vertex(p2.x(), p2.y(), p2.z());
			bot_cshape->push_vertex(p3.x(), p3.y(), p3.z());
			bot_cshape->push_vertex(p1.x(), p1.y(), p1.z());
			bot_cshape->push_vertex(p3.x(), p3.y(), p3.z());
			bot_cshape->push_vertex(p4.x(), p4.y(), p4.z());
			for (int c=0; c<4; c++)
				bot_shape->push_normal(0,0,-1);
			for (int c=0; c<6; c++)
				bot_cshape->push_normal(0,0,-1);
			float next_side_tex_spent = side_tex_spent - (p4-p1).norm();
			btScalar depth = (p1-p2).norm();
			bot_shape->push_tex( side_tex_spent/bott_tex_scale/tex1,      low_expanded[c1]/bott_tex_scale/tex1 );
			bot_shape->push_tex( side_tex_spent/bott_tex_scale/tex1,      (low_expanded[c1] - depth)/bott_tex_scale/tex1 );
			bot_shape->push_tex( next_side_tex_spent/bott_tex_scale/tex1, (low_expanded[c1] - depth)/bott_tex_scale/tex1 );
			bot_shape->push_tex( next_side_tex_spent/bott_tex_scale/tex1, low_expanded[c1]/bott_tex_scale/tex1 );
		}

		// butt (facing door or window when height change)
		if (hgh_expanded[c1] != hgh_expanded[c2] && but_shape) {
			btVector3 p1(path_outer[2*c1+0], path_outer[2*c1+1], hgh_expanded[c1]);
			btVector3 p2(path_inner[2*c1+0], path_inner[2*c1+1], hgh_expanded[c1]);
			btVector3 p3(path_inner[2*c1+0], path_inner[2*c1+1], hgh_expanded[c2]);
			btVector3 p4(path_outer[2*c1+0], path_outer[2*c1+1], hgh_expanded[c2]);
			btVector3 v1 = p2-p1;
			btVector3 v2 = p4-p1;
			btVector3 n = v1.cross(v2);
			n.normalize();
			if (TEST_GAP) {
				btVector3 center = (p1+p2+p3+p4)/4;
				p1 -= sign(p1-center)*TEST_GAP;
				p2 -= sign(p2-center)*TEST_GAP;
				p3 -= sign(p3-center)*TEST_GAP;
				p4 -= sign(p4-center)*TEST_GAP;
			}
			but_shape->push_vertex(p1.x(), p1.y(), p1.z());
			but_shape->push_vertex(p2.x(), p2.y(), p2.z());
			but_shape->push_vertex(p3.x(), p3.y(), p3.z());
			but_shape->push_vertex(p4.x(), p4.y(), p4.z());
			but_cshape->push_vertex(p1.x(), p1.y(), p1.z());
			but_cshape->push_vertex(p2.x(), p2.y(), p2.z());
			but_cshape->push_vertex(p3.x(), p3.y(), p3.z());
			but_cshape->push_vertex(p1.x(), p1.y(), p1.z());
			but_cshape->push_vertex(p3.x(), p3.y(), p3.z());
			but_cshape->push_vertex(p4.x(), p4.y(), p4.z());
			for (int c=0; c<4; c++)
				but_shape->push_normal(n.x(), n.y(), n.z()); // n.z is zero
			for (int c=0; c<6; c++)
				but_cshape->push_normal(n.x(), n.y(), n.z());
			float next_side_tex_spent = side_tex_spent + (p1-p2).norm() * (hgh_expanded[c1] > hgh_expanded[c2] ? +1:-1);
			but_shape->push_tex(side_tex_spent/butt_tex_scale/tex1,      (hgh_expanded[c1]-tex_v_zero)/butt_tex_scale/tex1);
			but_shape->push_tex(next_side_tex_spent/butt_tex_scale/tex1, (hgh_expanded[c1]-tex_v_zero)/butt_tex_scale/tex1);
			but_shape->push_tex(next_side_tex_spent/butt_tex_scale/tex1, (hgh_expanded[c2]-tex_v_zero)/butt_tex_scale/tex1);
			but_shape->push_tex(side_tex_spent/butt_tex_scale/tex1,      (hgh_expanded[c2]-tex_v_zero)/butt_tex_scale/tex1);
		}

		if (low_expanded[c1] != low_expanded[c2] && but_shape) {
			btVector3 p1(path_outer[2*c1+0], path_outer[2*c1+1], low_expanded[c1]);
			btVector3 p2(path_outer[2*c1+0], path_outer[2*c1+1], low_expanded[c2]);
			btVector3 p3(path_inner[2*c1+0], path_inner[2*c1+1], low_expanded[c2]);
			btVector3 p4(path_inner[2*c1+0], path_inner[2*c1+1], low_expanded[c1]);
			btVector3 v1 = p2-p1;
			btVector3 v2 = p4-p1;
			btVector3 n = v1.cross(v2);
			n.normalize();
			if (TEST_GAP) {
				btVector3 center = (p1+p2+p3+p4)/4;
				p1 -= sign(p1-center)*TEST_GAP;
				p2 -= sign(p2-center)*TEST_GAP;
				p3 -= sign(p3-center)*TEST_GAP;
				p4 -= sign(p4-center)*TEST_GAP;
			}
			but_shape->push_vertex(p1.x(), p1.y(), p1.z());
			but_shape->push_vertex(p2.x(), p2.y(), p2.z());
			but_shape->push_vertex(p3.x(), p3.y(), p3.z());
			but_shape->push_vertex(p4.x(), p4.y(), p4.z());
			but_cshape->push_vertex(p1.x(), p1.y(), p1.z());
			but_cshape->push_vertex(p2.x(), p2.y(), p2.z());
			but_cshape->push_vertex(p3.x(), p3.y(), p3.z());
			but_cshape->push_vertex(p1.x(), p1.y(), p1.z());
			but_cshape->push_vertex(p3.x(), p3.y(), p3.z());
			but_cshape->push_vertex(p4.x(), p4.y(), p4.z());
			for (int c=0; c<4; c++)
				but_shape->push_normal(n.x(), n.y(), n.z()); // n.z is zero
			for (int c=0; c<6; c++)
				but_cshape->push_normal(n.x(), n.y(), n.z());
			float next_side_tex_spent = side_tex_spent + (p2-p3).norm() * (low_expanded[c1] < low_expanded[c2] ? +1:-1);
			but_shape->push_tex(side_tex_spent/tex1,      (low_expanded[c1]-tex_v_zero)/butt_tex_scale/tex1);
			but_shape->push_tex(side_tex_spent/tex1,      (low_expanded[c2]-tex_v_zero)/butt_tex_scale/tex1);
			but_shape->push_tex(next_side_tex_spent/tex1, (low_expanded[c2]-tex_v_zero)/butt_tex_scale/tex1);
			but_shape->push_tex(next_side_tex_spent/tex1, (low_expanded[c1]-tex_v_zero)/butt_tex_scale/tex1);
		}
	}

	shared_ptr<Household::ShapeDetailLevels> shapeset_visual(new Household::ShapeDetailLevels);
	shared_ptr<Household::ShapeDetailLevels> shapeset_collision(new Household::ShapeDetailLevels);
	if (side_shape) {
		shapeset_visual->detail_levels[Household::DETAIL_BEST].push_back(side_shape);
		side_cshape->quads_from = side_cshape->v.size();
		shapeset_collision->detail_levels[Household::DETAIL_BEST].push_back(side_cshape);
	}
	if (top_shape) {
		shapeset_visual->detail_levels[Household::DETAIL_BEST].push_back(top_shape);
		top_cshape->quads_from = top_cshape->v.size();
		shapeset_collision->detail_levels[Household::DETAIL_BEST].push_back(top_cshape);
	}
	if (bot_shape) {
		shapeset_visual->detail_levels[Household::DETAIL_BEST].push_back(bot_shape);
		bot_cshape->quads_from = bot_cshape->v.size();
		shapeset_collision->detail_levels[Household::DETAIL_BEST].push_back(bot_cshape);
	}
	if (but_shape) {
		shapeset_visual->detail_levels[Household::DETAIL_BEST].push_back(but_shape);
		but_cshape->quads_from = but_cshape->v.size();
		shapeset_collision->detail_levels[Household::DETAIL_BEST].push_back(but_cshape);
	}
	shapeset_visual->materials = textures_cache;

	shared_ptr<ThingyClass> klass(new ThingyClass);
	klass->class_name = "WallTool";
	klass->metaclass = METACLASS_WALL;
	klass->shapedet_visual = shapeset_visual;
	klass->shapedet_collision = shapeset_collision;

	klass->make_compound_from_collision_shapes();

	shared_ptr<Thingy> th(new Thingy(klass));
	th->self_collision_ptr = th.get();
	thingy_add(th);
	return th;
}

shared_ptr<Thingy> World::tool_quad_prism(
	float tex1, float tex_v_zero,
	std::vector<btScalar>& path,
	btScalar low,
	btScalar high,
	const std::string& side_tex_,
	const std::string& top_tex_,
	const std::string& bottom_tex_)
{
	shared_ptr<ThingyClass> klass(new ThingyClass);
	klass->class_name = "PrismTool";
	klass->metaclass = METACLASS_FLOOR;

	shared_ptr<Household::ShapeDetailLevels> shapeset(new Household::ShapeDetailLevels);

	shared_ptr<Household::Shape> side_shape;
	shared_ptr<Household::Shape> side_cshape;
	if (!side_tex_.empty()) {
		side_shape.reset(new Household::Shape);
		side_shape->material = texture_from_file_cached(side_tex_);
		side_cshape.reset(new Household::Shape);
		side_cshape->primitive_type = Household::Shape::STATIC_MESH;
	}

	shared_ptr<Household::Shape> top_shape;
	shared_ptr<Household::Shape> top_cshape;
	if (!top_tex_.empty()) {
		top_shape.reset(new Household::Shape);
		top_shape->material = texture_from_file_cached(top_tex_);
		top_cshape.reset(new Household::Shape);
		top_cshape->primitive_type = Household::Shape::STATIC_MESH;
	}

	shared_ptr<Household::Shape> bot_shape;
	shared_ptr<Household::Shape> bot_cshape;
	if (!bottom_tex_.empty()) {
		bot_shape.reset(new Household::Shape);
		bot_shape->material = texture_from_file_cached(bottom_tex_);
		bot_cshape.reset(new Household::Shape);
		bot_cshape->primitive_type = Household::Shape::STATIC_MESH;
	}

	int cnt = path.size()/2;
	assert((cnt==4 || cnt==3) && "should be quad or triangle");
	std::vector<btVector3> normals;
	btVector3 center(0,0,0);
	for (int c=0; c<cnt; c++) {
		int c1 = (c+0) % cnt;
		int c2 = (c+1) % cnt;
		btVector3 hor1(path[2*c2+0] - path[2*c1+0], path[2*c2+1] - path[2*c1+1], 0);
		btVector3 ver(0, 0, -1);
		btVector3 norm1 = hor1.cross(ver);
		norm1.normalize();
		normals.push_back(norm1);
		center += btVector3(path[2*c+0], path[2*c+1], 0);
	}
	center /= cnt;
	center.setZ(0.5*(low+high));
	for (int c=0; c<cnt; c++) {
		path[2*c+0] -= center.x();
		path[2*c+1] -= center.y();
	}
	low  -= center.z();
	high -= center.z();

	float side_tex_spent = 0;
	if (side_shape)
	for (int c=0; c<cnt; c++) {
		int c1 = (c+0) % cnt;
		int c2 = (c+1) % cnt;
		btVector3 norm = normals[c1];
		btVector3 p1(path[2*c1+0], path[2*c1+1], low);
		btVector3 p2(path[2*c2+0], path[2*c2+1], low);
		btVector3 p3(path[2*c2+0], path[2*c2+1], high);
		btVector3 p4(path[2*c1+0], path[2*c1+1], high);
		side_shape->push_vertex(p1.x(), p1.y(), p1.z());
		side_shape->push_vertex(p2.x(), p2.y(), p2.z());
		side_shape->push_vertex(p3.x(), p3.y(), p3.z());
		side_shape->push_vertex(p4.x(), p4.y(), p4.z());
		side_cshape->push_vertex(p1.x(), p1.y(), p1.z());
		side_cshape->push_vertex(p2.x(), p2.y(), p2.z());
		side_cshape->push_vertex(p3.x(), p3.y(), p3.z());
		side_cshape->push_vertex(p1.x(), p1.y(), p1.z());
		side_cshape->push_vertex(p3.x(), p3.y(), p3.z());
		side_cshape->push_vertex(p4.x(), p4.y(), p4.z());
		for (int c=0; c<4; c++)
			side_shape->push_normal(norm.x(), norm.y(), norm.z());
		for (int c=0; c<6; c++)
			side_cshape->push_normal(norm.x(), norm.y(), norm.z());
		float next_side_tex_spent = side_tex_spent + (p2-p1).norm();
		side_shape->push_tex(side_tex_spent/tex1,      (low-tex_v_zero)/tex1 );
		side_shape->push_tex(side_tex_spent/tex1,      (high-tex_v_zero)/tex1);
		side_shape->push_tex(next_side_tex_spent/tex1, (high-tex_v_zero)/tex1);
		side_shape->push_tex(next_side_tex_spent/tex1, (low-tex_v_zero)/tex1);
		side_tex_spent = next_side_tex_spent;
	}

	if (top_shape) {
		if (cnt==4) top_shape->quads_from = 0;
		else top_shape->quads_from = 9;
		btVector3 p1(path[0] + sign(center.x()-path[0])*TEST_GAP, path[1] + sign(center.y()-path[1])*TEST_GAP, high);
		btVector3 p2(path[2] + sign(center.x()-path[2])*TEST_GAP, path[3] + sign(center.y()-path[3])*TEST_GAP, high);
		btVector3 p3(path[4] + sign(center.x()-path[4])*TEST_GAP, path[5] + sign(center.y()-path[5])*TEST_GAP, high);
		btVector3 p4(0,0,0);
		top_shape->push_vertex(p1.x(), p1.y(), p1.z());
		top_shape->push_vertex(p2.x(), p2.y(), p2.z());
		top_shape->push_vertex(p3.x(), p3.y(), p3.z());
		top_cshape->push_vertex(p1.x(), p1.y(), p1.z());
		top_cshape->push_vertex(p2.x(), p2.y(), p2.z());
		top_cshape->push_vertex(p3.x(), p3.y(), p3.z());
		if (cnt==4) {
			p4 = btVector3(path[6] + sign(center.x()-path[6])*TEST_GAP, path[7] + sign(center.y()-path[7])*TEST_GAP, high);
			top_shape->push_vertex(p4.x(), p4.y(), p4.z());
			top_cshape->push_vertex(p1.x(), p1.y(), p1.z());
			top_cshape->push_vertex(p3.x(), p3.y(), p3.z());
			top_cshape->push_vertex(p4.x(), p4.y(), p4.z());
		}
		for (int c=0; c<cnt; c++)
			top_shape->push_normal(0,0,1);
		for (int c=0; c<(cnt==4 ? 6 : 3); c++)
			top_cshape->push_normal(0,0,1);
		top_shape->push_tex( (p1.x() + center.x())/tex1, (p1.y() + center.y())/tex1 );
		top_shape->push_tex( (p2.x() + center.x())/tex1, (p2.y() + center.y())/tex1 );
		top_shape->push_tex( (p3.x() + center.x())/tex1, (p3.y() + center.y())/tex1 );
		if (cnt==4)
		top_shape->push_tex( (p4.x() + center.x())/tex1, (p4.y() + center.y())/tex1 );
	}

	shared_ptr<Household::ShapeDetailLevels> shapeset_visual(new Household::ShapeDetailLevels);
	shared_ptr<Household::ShapeDetailLevels> shapeset_collision(new Household::ShapeDetailLevels);
	if (side_shape) {
		shapeset_visual->detail_levels[Household::DETAIL_BEST].push_back(side_shape);
		side_cshape->quads_from = side_cshape->v.size();
		shapeset_collision->detail_levels[Household::DETAIL_BEST].push_back(side_cshape);
	}
	if (top_shape) {
		shapeset_visual->detail_levels[Household::DETAIL_BEST].push_back(top_shape);
		top_cshape->quads_from = top_cshape->v.size();
		shapeset_collision->detail_levels[Household::DETAIL_BEST].push_back(top_cshape);
	}
	if (bot_shape) {
		shapeset_visual->detail_levels[Household::DETAIL_BEST].push_back(bot_shape);
		bot_cshape->quads_from = bot_cshape->v.size();
		shapeset_collision->detail_levels[Household::DETAIL_BEST].push_back(bot_cshape);
	}
	shapeset_visual->materials = textures_cache;
	klass->shapedet_visual = shapeset_visual;
	klass->shapedet_collision = shapeset_collision;

	klass->make_compound_from_collision_shapes();

	shared_ptr<Thingy> th(new Thingy(klass));
	th->self_collision_ptr = th.get();
	th->bullet_position = btTransform(btTransform(btQuaternion(0,0,0,1), center));
	thingy_add(th);
	return th;
}
#endif

shared_ptr<Thingy> World::debug_rect(btScalar x1, btScalar y1, btScalar x2, btScalar y2, btScalar h, uint32_t color)
{
	return shared_ptr<Thingy>();
	if (0) { // broken
		shared_ptr<Household::Shape> l(new Household::Shape);
		l->primitive_type = Shape::DEBUG_LINES;
		l->lines_color = color;
		l->push_lines(x1, y1, h);
		l->push_lines(x2, y1, h);
		l->push_lines(x2, y1, h);
		l->push_lines(x2, y2, h);
		l->push_lines(x2, y2, h);
		l->push_lines(x1, y2, h);
		l->push_lines(x1, y2, h);
		l->push_lines(x1, y1, h);
		shared_ptr<ShapeDetailLevels> visual(new ShapeDetailLevels);
		visual->detail_levels[DETAIL_BEST].push_back(l);
		shared_ptr<Household::ThingyClass> klass(new Household::ThingyClass);
		klass->class_name = "debug_rect";
		klass->shapedet_visual = visual;
		shared_ptr<Household::Thingy> t(new Household::Thingy());
		t->klass = klass;
		t->bullet_ignore = true;
		t->bullet_position.setIdentity();
		thingy_add_to_drawlist(t);
		return t;
	}
}

shared_ptr<Thingy> World::debug_line(btScalar x1, btScalar y1, btScalar z1, btScalar x2, btScalar y2, btScalar z2, uint32_t color)
{
	return shared_ptr<Thingy>();
	if (0) { // broken
		shared_ptr<Household::Shape> l(new Household::Shape);
		l->primitive_type = Shape::DEBUG_LINES;
		l->lines_color = color;
		btVector3 center( (x1+x2)*0.5, (y1+y2)*0.5, (z1+z2)*0.5 );
		l->push_lines(x1 - center.x(), y1 - center.y(), z1 - center.z());
		l->push_lines(x2 - center.x(), y2 - center.y(), z2 - center.z());
		shared_ptr<ShapeDetailLevels> visual(new ShapeDetailLevels);
		visual->detail_levels[DETAIL_BEST].push_back(l);
		shared_ptr<Household::ThingyClass> klass(new Household::ThingyClass);
		klass->class_name = "debug_line";
		klass->shapedet_visual = visual;
		shared_ptr<Household::Thingy> t(new Household::Thingy());
		t->klass = klass;
		t->bullet_ignore = true;
		t->bullet_position = btTransform(btTransform(btQuaternion(0,0,0,1), center));
		thingy_add_to_drawlist(t);
		return t;
	}
}

shared_ptr<Thingy> World::debug_sphere(btScalar x, btScalar y, btScalar z, btScalar rad, uint32_t color)
{
	char buf[1024];
	snprintf(buf, sizeof(buf), "debug_sphere_%lf_%x", (double)rad, color);
	std::string class_name = buf;
	shared_ptr<Household::ThingyClass> klass = klass_cache_find_or_create(class_name);
	if (!klass->frozen) {
		shared_ptr<Material> mat(new Material(class_name));
		mat->diffuse_color = color;
		shared_ptr<Household::Shape> l(new Household::Shape);
		l->primitive_type = Shape::SPHERE;
		l->sphere.reset(new Sphere({ rad }));
		l->material = mat;
		klass->shapedet_visual->detail_levels[DETAIL_BEST].push_back(l);
		klass->frozen = true;
	}
	shared_ptr<Household::Thingy> t(new Household::Thingy());
	t->klass = klass;
	t->bullet_ignore = true;
	t->bullet_position = btTransform(btTransform(btQuaternion(0,0,0,1), btVector3(x,y,z)));
	thingy_add_to_drawlist(t);
	return t;
}

} // namespace Household
