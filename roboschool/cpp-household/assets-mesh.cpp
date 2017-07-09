#include "render-simple.h"

#include <QtCore/QFileInfo>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

namespace Household {

static
void mesh_push_vertex(const shared_ptr<Shape>& m, const aiMesh* aimesh, int vind, btScalar scale)
{
	m->v.push_back(aimesh->mVertices[vind][0]*scale);
	m->v.push_back(aimesh->mVertices[vind][1]*scale);
	m->v.push_back(aimesh->mVertices[vind][2]*scale);
	if (aimesh->mTextureCoords[0]) {
		m->t.push_back(aimesh->mTextureCoords[0][vind][0]);
		m->t.push_back(1-aimesh->mTextureCoords[0][vind][1]);
	}
	if (aimesh->mNormals) {
		m->norm.push_back(aimesh->mNormals[vind][0]);
		m->norm.push_back(aimesh->mNormals[vind][1]);
		m->norm.push_back(aimesh->mNormals[vind][2]);
	}
}

void Shape::push_vertex(btScalar vx, btScalar vy, btScalar vz)
{
	v.push_back(float(vx));
	v.push_back(float(vy));
	v.push_back(float(vz));
}

void Shape::push_normal(btScalar nx, btScalar ny, btScalar nz)
{
	norm.push_back(float(nx));
	norm.push_back(float(ny));
	norm.push_back(float(nz));
}

void Shape::push_tex(btScalar u, btScalar v)
{
	t.push_back(float(u));
	t.push_back(1-float(v));
}

void Shape::push_lines(btScalar x, btScalar y, btScalar z)
{
	lines.push_back(float(x));
	lines.push_back(float(y));
	lines.push_back(float(z));
}

static
void mesh_push_line(const shared_ptr<Shape>& m, const aiMesh* aimesh, int vind1, int vind2, btScalar scale)
{
	m->lines.push_back(aimesh->mVertices[vind1][0]*scale);
	m->lines.push_back(aimesh->mVertices[vind1][1]*scale);
	m->lines.push_back(aimesh->mVertices[vind1][2]*scale);
	m->lines.push_back(aimesh->mVertices[vind2][0]*scale);
	m->lines.push_back(aimesh->mVertices[vind2][1]*scale);
	m->lines.push_back(aimesh->mVertices[vind2][2]*scale);
}

bool load_collision_shape_from_OFF_files(const shared_ptr<ShapeDetailLevels>& result, const std::string& fn_template, btScalar scale, const btTransform& viz_frame)
{
	for (int c=0; c<50; c++) {
		QString fn = QString(fn_template.c_str()) . arg(c, 2, 10, QChar('0'));
		if (!QFileInfo(fn).exists()) {
			if (c==0) return false;
			return true;
		}
		load_model(result, fn.toUtf8().data(), scale, viz_frame);
	}
	return false;
}

void load_model(const shared_ptr<ShapeDetailLevels>& result, const std::string& fn, btScalar scale, const btTransform& transform)
{
	//fprintf(stderr, "Loading model '%s' (takes some time, should not happen when learning without rendering)\n", fn.c_str());
	Assimp::Importer importer1;
	std::string ext = fn.substr(fn.size()-3,  3);
	aiMatrix4x4 root_trans;
	importer1.SetPropertyInteger(AI_CONFIG_PP_PTV_ADD_ROOT_TRANSFORMATION, 1);
	importer1.SetPropertyMatrix(AI_CONFIG_PP_PTV_ROOT_TRANSFORMATION, root_trans); // setting identity matrix helps to load .dae, resulting model turned on the side without it
	const aiScene* scene1 = importer1.ReadFile(fn,
		aiProcess_JoinIdenticalVertices | aiProcess_GenNormals | aiProcess_ImproveCacheLocality | aiProcess_PreTransformVertices | aiProcess_Triangulate
		);

	if (!scene1) throw std::runtime_error("cannot load '" + fn + "': " + std::string(importer1.GetErrorString()) );
	if (scene1->mNumMeshes==0 && scene1->mMaterials==0) throw std::runtime_error("cannot load '" + fn + "': model empty");

	Assimp::Importer importer2;
	importer2.SetPropertyInteger(AI_CONFIG_PP_PTV_ADD_ROOT_TRANSFORMATION, 1);
	importer2.SetPropertyMatrix(AI_CONFIG_PP_PTV_ROOT_TRANSFORMATION, root_trans);
	const aiScene* scene2 = importer2.ReadFile(fn, aiProcess_JoinIdenticalVertices | aiProcess_PreTransformVertices);
	assert(scene2->mNumMaterials==scene1->mNumMaterials);
	assert(scene2->mNumMeshes==scene1->mNumMeshes);

	std::vector<shared_ptr<Material>> materials;
	if (!result->materials || ext=="dae")  // Texture names tend to repeat in .dae, for example "Material_001", cannot be made common for the whole robot
		result->materials.reset(new MaterialNamespace);
	for (int c=0; c<(int)scene1->mNumMaterials; c++) {
		const aiMaterial* aim = scene1->mMaterials[c];
		aiString name;
		aim->Get(AI_MATKEY_NAME, name);

		shared_ptr<Material> m;
		auto find = result->materials->name2mtl.find(name.C_Str());

		if (find!=result->materials->name2mtl.end()) {
			m = find->second;
		} else {
			m.reset(new Material(name.C_Str()));
			aiString aipath;
			if (aim->GetTexture(aiTextureType_DIFFUSE, 0, &aipath, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
				QFileInfo finfo(QString::fromUtf8(fn.c_str()));
				QString path = finfo.absolutePath();
				path += "/";
				path += QString::fromUtf8(aipath.data);
				m->diffuse_texture_image_fn = path.toUtf8().constData();
			}
			aiColor4D diffuse;
			if (aiGetMaterialColor(aim, AI_MATKEY_COLOR_DIFFUSE, &diffuse) == AI_SUCCESS) {
				m->diffuse_color =
					(uint32_t(255*diffuse[0]) << 16) |
					(uint32_t(255*diffuse[1]) << 8) |
					(uint32_t(255*diffuse[2]) << 0);
			}
			result->materials->name2mtl[m->name] = m;
		}
		materials.push_back(m);
	}

	for (int c=0; c<(int)scene1->mNumMeshes; c++) {
		const aiMesh* aimesh1 = scene1->mMeshes[c];
		const aiMesh* aimesh2 = scene2->mMeshes[c];
		shared_ptr<Shape> mesh(new Shape);
		mesh->origin = transform;
		for (int v=0; v<(int)aimesh2->mNumVertices; v++) {
			mesh->raw_vertexes.push_back(aimesh2->mVertices[v][0]*scale);
			mesh->raw_vertexes.push_back(aimesh2->mVertices[v][1]*scale);
			mesh->raw_vertexes.push_back(aimesh2->mVertices[v][2]*scale);
		}
		mesh->material = materials[aimesh1->mMaterialIndex];
		for (int f=0; f<(int)aimesh1->mNumFaces; f++) {
			const aiFace& face = aimesh1->mFaces[f];
			if (face.mNumIndices==3) {
				mesh_push_vertex(mesh, aimesh1, face.mIndices[0], scale);
				mesh_push_vertex(mesh, aimesh1, face.mIndices[1], scale);
				mesh_push_vertex(mesh, aimesh1, face.mIndices[2], scale);
				mesh_push_line(mesh, aimesh1, face.mIndices[0], face.mIndices[1], scale);
				mesh_push_line(mesh, aimesh1, face.mIndices[1], face.mIndices[2], scale);
				mesh_push_line(mesh, aimesh1, face.mIndices[2], face.mIndices[0], scale);
			} else {
				fprintf(stderr, "%s mesh face with %i verts\n", fn.c_str(), face.mNumIndices);
			}
		}
		if (mesh->v.size())
			result->detail_levels[DETAIL_BEST].push_back(mesh);
		//printf("%s mesh %i\n%5i raw vertexes, %5i vertexes in triangles, %5i vertexes in quads\n",
		//	fn.c_str(), c, (int)mesh->raw_vertexes.size()/3, (int)mesh->v.size()/3, (int)mesh->v.size()/3);
	}
}

//void Robot::replace_texture(const std::string& material_name, const std::string& texid)
//{
//	shared_ptr<MaterialNamespace> materials = root_part->klass->shapedet_visual->materials; // all parts share materials pointer
//	shared_ptr<MaterialNamespace> alt_materials(new MaterialNamespace);
//	std::map<shared_ptr<Material>, shared_ptr<Material>> existing2alt;
//	for (auto pair: materials->name2mtl) {
//		shared_ptr<Material> mat = pair.second;
//		if (mat->name==material_name) {
//			shared_ptr<Material> alt_mat(new Material(material_name));
//			*alt_mat = *mat;
//			if (texid[0]=='#') {
//				assert("color should be #rrggbb" && texid.size()==7);
//				int parsed = sscanf(texid.substr(1).c_str(), "%x", &alt_mat->diffuse_color);
//				assert(parsed==1);
//			} else {
//				//assert(!mat->texture_loaded && "please call replace_texture() before first render");
//				alt_mat->texture_loaded = false;
//				alt_mat->diffuse_texture_image_fn = texid;
//			}
//			alt_materials->name2mtl[pair.first] = alt_mat;
//			existing2alt[mat] = alt_mat;
//		} else {
//			alt_materials->name2mtl[pair.first] = mat;
//		}
//	}

//	for (auto pair: robot_parts) {
//		shared_ptr<Thingy> t = pair.second;
//		shared_ptr<ThingyClass> alt_class;
//		std::string alt_class_name = t->klass->class_name + ":" + material_name + "->" + texid;
//		int stat_replaced = 0;
//		int stat_left_alone = 0;
//		auto f = robot_links_cache.find(alt_class_name);
//		if (f!=robot_links_cache.end()) {
//			alt_class = f->second;
//		} else {
//			alt_class.reset(new ThingyClass);
//			*alt_class = *t->klass; // copy by value
//			alt_class->shapedet_visual.reset(new ShapeDetailLevels);
//			alt_class->shapedet_visual->materials = alt_materials;
//			for (int lev=0; lev<DETAIL_LEVELS; lev++) {
//				const std::vector<shared_ptr<Shape>>& shapes = t->klass->shapedet_visual->detail_levels[lev];
//				std::vector<shared_ptr<Shape>>& alt_shapes = alt_class->shapedet_visual->detail_levels[lev];
//				for (int c=0; c<(int)shapes.size(); ++c) {
//					auto f = existing2alt.find(shapes[c]->material);
//					if (f != existing2alt.end()) {
//						shared_ptr<Shape> alt_shape(new Shape);
//						*alt_shape = *shapes[c]; // copy by value
//						alt_shape->material = f->second;
//						alt_shapes.push_back(alt_shape);
//						stat_replaced++;
//					} else {
//						alt_shapes.push_back(shapes[c]);
//						stat_left_alone++;
//					}
//				}
//			}
//			//fprintf(stderr, "replace_texture() class %s: %i materials replaced, %i left alone\n", alt_class_name.c_str(), stat_replaced, stat_left_alone);
//			if (stat_replaced==0)
//				alt_class = t->klass;
//			robot_links_cache[alt_class_name] = alt_class;
//		}
//		t->klass = alt_class;
//	}
//}

void Thingy::set_multiply_color(const std::string& tex, uint32_t* color, std::string* replace_texture)  // not urdf at all, but does very similar thing to the function above
{
	shared_ptr<Material> replace_me, modified;
	shared_ptr<ThingyClass> alt_class(new ThingyClass);
	*alt_class = *klass; // copy by value
	alt_class->shapedet_visual.reset(new ShapeDetailLevels);
	alt_class->shapedet_visual->materials.reset(new MaterialNamespace);
	for (auto pair: klass->shapedet_visual->materials->name2mtl) {
		shared_ptr<Material> mat = pair.second;
		if (pair.first==tex) {
			replace_me = mat;
			modified.reset(new Material(replace_me->name + ":colorized"));
			*modified = *replace_me; // copy by value
			if (color) modified->multiply_color = *color;
			if (replace_texture) modified->diffuse_texture_image_fn = *replace_texture;
			alt_class->shapedet_visual->materials->name2mtl[pair.first] = modified;
		} else {
			alt_class->shapedet_visual->materials->name2mtl[pair.first] = mat;
		}
	}
	for (int lev=0; lev<DETAIL_LEVELS; lev++) {
		const std::vector<shared_ptr<Shape>>& shapes = klass->shapedet_visual->detail_levels[lev];
		std::vector<shared_ptr<Shape>>& alt_shapes = alt_class->shapedet_visual->detail_levels[lev];
		for (int c=0; c<(int)shapes.size(); ++c) {
			if (shapes[c]->material==replace_me) {
				shared_ptr<Shape> alt_shape(new Shape);
				*alt_shape = *shapes[c];
				alt_shape->material = modified;
				alt_shapes.push_back(alt_shape);
			} else {
				alt_shapes.push_back(shapes[c]);
			}
		}
	}
	alt_class->modified_from_class = klass;
	klass = alt_class;
}

} // namespace
