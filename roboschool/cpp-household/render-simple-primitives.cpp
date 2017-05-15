#include "render-simple.h"
#include <assimp/scene.h>           // for aiVector3D structure

namespace SimpleRender {

using namespace Household;

void primitives_to_mesh(const shared_ptr<ShapeDetailLevels>& m, int want_detail, int s)
{
	std::vector<shared_ptr<SimpleRender::Shape>>& shapes = m->detail_levels[want_detail];
	const std::vector<shared_ptr<SimpleRender::Shape>>& best_detail = m->detail_levels[DETAIL_BEST];

	int shapes_count = best_detail.size();
	shapes.resize(shapes_count);

	{
		shared_ptr<SimpleRender::Shape> lower_detail;
		if (want_detail==DETAIL_BEST) {
			lower_detail = best_detail[s];
		} else {
			lower_detail.reset(new SimpleRender::Shape);
			*lower_detail = *(best_detail[s]); // copy by value
		}

		if (lower_detail->primitive_type==SimpleRender::Shape::MESH) {
			// TODO: simplify
			// now leave copied best_detail

		} else if (lower_detail->primitive_type==SimpleRender::Shape::STATIC_MESH) {
			// visualizing collision shape, leave it as it is

		} else if (lower_detail->primitive_type==SimpleRender::Shape::BOX) {
			assert(lower_detail->v.empty());
			double n[] = {
			+1, 0, 0,
			-1, 0, 0,
			0, +1, 0,
			0, -1, 0,
			0, 0, +1,
			0, 0, -1 };
			for (int f=0; f<6; f++) {
				double side[] = {
				+1, +1,
				-1, +1,
				-1, -1,
				+1, -1 };
				int zero1 = n[3*f + 0]==0 ? 0 : 1;
				int zero2 = n[3*f + 2]==0 ? 2 : 1;
				int sign = n[3*f + 0] + n[3*f + 1] + n[3*f + 2];
				if (f==2 || f==3) sign *= -1;
				int ind_reloc[] = { 0,1,3, 3,1,2 }; // Triangles that together make up rect 0123
				for (int i=0; i<6; ++i) {
					int idx = ind_reloc[i];
					lower_detail->push_normal(n[3*f + 0], n[3*f + 1], n[3*f + 2]);
					float v[3];
					v[0] = n[3*f+0];
					v[1] = n[3*f+1];
					v[2] = n[3*f+2];
					if (sign > 0) {
						v[zero1] = side[2*idx + 0];
						v[zero2] = side[2*idx + 1];
					} else {
						v[zero1] = side[6 - 2*idx];
						v[zero2] = side[7 - 2*idx];
					}
					lower_detail->push_vertex(v[0]*0.5*lower_detail->box->size_x, v[1]*0.5*lower_detail->box->size_y, v[2]*0.5*lower_detail->box->size_z);
				}
			}

		} else if (lower_detail->primitive_type==SimpleRender::Shape::CYLINDER) {
			assert(lower_detail->v.empty());
			int side_faces;
			switch (want_detail) {
			case 0: side_faces = 16; break;
			case 1: side_faces =  8; break;
			default: side_faces = 3; break;
			}
			float l = lower_detail->cylinder->length;
			float r = lower_detail->cylinder->radius;
			for (int c=0; c<side_faces; c++) {
				float angle1 = float(c)   / side_faces * 2 * M_PI;
				float angle2 = float(c+1) / side_faces * 2 * M_PI;
				float n1[3], n2[3];
				n1[0] = cos(angle1); n1[1] = sin(angle1); n1[2] = 0;
				n2[0] = cos(angle2); n2[1] = sin(angle2); n2[2] = 0;
				lower_detail->push_vertex(n1[0]*r, n1[1]*r, +l*0.5);
				lower_detail->push_vertex(n2[0]*r, n2[1]*r, +l*0.5);
				lower_detail->push_vertex(   0.0f,       0, +l*0.5);
				lower_detail->push_normal(0.0f, 0, 1);
				lower_detail->push_normal(0.0f, 0, 1);
				lower_detail->push_normal(0.0f, 0, 1);
				lower_detail->push_vertex(n1[0]*r, n1[1]*r, -l*0.5);
				lower_detail->push_vertex(   0.0f,       0, -l*0.5);
				lower_detail->push_vertex(n2[0]*r, n2[1]*r, -l*0.5);
				lower_detail->push_normal(0.0f, 0, -1);
				lower_detail->push_normal(0.0f, 0, -1);
				lower_detail->push_normal(0.0f, 0, -1);
			}
			for (int c=0; c<side_faces; c++) {
				float angle1 = float(c)   / side_faces * 2 * M_PI;
				float angle2 = float(c+1) / side_faces * 2 * M_PI;
				float n1[3], n2[3];
				n1[0] = cos(angle1); n1[1] = sin(angle1); n1[2] = 0;
				n2[0] = cos(angle2); n2[1] = sin(angle2); n2[2] = 0;
				lower_detail->push_vertex(n1[0]*r, n1[1]*r, -l*0.5);
				lower_detail->push_vertex(n2[0]*r, n2[1]*r, -l*0.5);
				lower_detail->push_vertex(n2[0]*r, n2[1]*r, +l*0.5);
				lower_detail->push_normal(n1[0], n1[1], n1[2]);
				lower_detail->push_normal(n2[0], n2[1], n2[2]);
				lower_detail->push_normal(n2[0], n2[1], n2[2]);

				lower_detail->push_vertex(n1[0]*r, n1[1]*r, -l*0.5);
				lower_detail->push_vertex(n2[0]*r, n2[1]*r, +l*0.5);
				lower_detail->push_vertex(n1[0]*r, n1[1]*r, +l*0.5);
				lower_detail->push_normal(n1[0], n1[1], n1[2]);
				lower_detail->push_normal(n2[0], n2[1], n2[2]);
				lower_detail->push_normal(n1[0], n1[1], n1[2]);
			}

		} else if (lower_detail->primitive_type==SimpleRender::Shape::SPHERE || lower_detail->primitive_type==SimpleRender::Shape::CAPSULE) {
			assert(lower_detail->v.empty());
			std::vector<aiVector3D> v(12, aiVector3D());
			// Icosahedron
			double theta = 26.56505117707799 * M_PI / 180.0;
			v[0] = aiVector3D(0,0,-1);
			double phi = M_PI/5;
			for (int i=1; i<6; ++i) {
				v[i] = aiVector3D(cos(theta)*cos(phi), cos(theta)*sin(phi), -sin(theta));
				phi += 2*M_PI / 5;
			}
			phi = 0.0;
			for (int i=6; i<11; ++i) {
				v[i] = aiVector3D(cos(theta)*cos(phi), cos(theta)*sin(phi), sin(theta));
				phi += 2*M_PI / 5;
			}
			v[11] = aiVector3D(0,0,+1);
			int idx[] = {
			0,2,1,
			0,3,2,
			0,4,3,
			0,5,4,
			0,1,5,
			1,2,7,
			2,3,8,
			3,4,9,
			4,5,10,
			5,1,6,
			1,7,6,
			2,8,7,
			3,9,8,
			4,10,9,
			5,6,10,
			6,7,11,
			7,8,11,
			8,9,11,
			9,10,11,
			10,6,11,
			};
			for (int i=0; i<20*3; i++)
				lower_detail->push_vertex(v[idx[i]].x, v[idx[i]].y, v[idx[i]].z);

			int repeat;
			switch (want_detail) {
			case DETAIL_BEST:  repeat = 2; break;
			case DETAIL_LOWER: repeat = 1; break;
			default: repeat = 0;
			}

			for (int c=0; c<repeat; c++) { // improve detail
				std::vector<float> v;
				v.swap(lower_detail->v);
				for (int i=0; i<(int)v.size(); i+=9) {
					aiVector3D e0(v[i+0], v[i+1], v[i+2]);
					aiVector3D e1(v[i+3], v[i+4], v[i+5]);
					aiVector3D e2(v[i+6], v[i+7], v[i+8]);
					aiVector3D mid01(e0.x+e1.x, e0.y+e1.y, e0.z+e1.z);
					aiVector3D mid12(e1.x+e2.x, e1.y+e2.y, e1.z+e2.z);
					aiVector3D mid20(e2.x+e0.x, e2.y+e0.y, e2.z+e0.z);
					mid01.Normalize();
					mid12.Normalize();
					mid20.Normalize();
					lower_detail->push_vertex(mid01.x, mid01.y, mid01.z);
					lower_detail->push_vertex(mid12.x, mid12.y, mid12.z);
					lower_detail->push_vertex(mid20.x, mid20.y, mid20.z);
					lower_detail->push_vertex(e0.x, e0.y, e0.z);
					lower_detail->push_vertex(mid01.x, mid01.y, mid01.z);
					lower_detail->push_vertex(mid20.x, mid20.y, mid20.z);
					lower_detail->push_vertex(e1.x, e1.y, e1.z);
					lower_detail->push_vertex(mid12.x, mid12.y, mid12.z);
					lower_detail->push_vertex(mid01.x, mid01.y, mid01.z);
					lower_detail->push_vertex(e2.x, e2.y, e2.z);
					lower_detail->push_vertex(mid20.x, mid20.y, mid20.z);
					lower_detail->push_vertex(mid12.x, mid12.y, mid12.z);
				}
			}

			bool capsule = lower_detail->primitive_type==SimpleRender::Shape::CAPSULE;
			float rad = capsule ? lower_detail->cylinder->radius : lower_detail->sphere->radius;
			float len = capsule ? lower_detail->cylinder->length/2 : 0;
			for (int i=0; i<(int)lower_detail->v.size()/3; i++) {
				lower_detail->norm.push_back(lower_detail->v[3*i+0]);
				lower_detail->norm.push_back(lower_detail->v[3*i+1]);
				lower_detail->norm.push_back(lower_detail->v[3*i+2]);
				lower_detail->v[3*i+0] *= rad;
				lower_detail->v[3*i+1] *= rad;
				lower_detail->v[3*i+2] *= rad;
				if (capsule) {
					if (lower_detail->v[3*i+2] > 0) {
						lower_detail->v[3*i+2] += len;
					} else if (lower_detail->v[3*i+2] < 0) {
						lower_detail->v[3*i+2] -= len;
					}
				}
			}

		} else {
			assert(!"unknown shape");
		}

		shapes[s] = lower_detail;
	}
}

} // namespace
