#define GL_GLEXT_PROTOTYPES
#include "render-simple.h"
#include <QtOpenGL/QtOpenGL>
#include <QtOpenGL/QGLFramebufferObject>

#ifdef __APPLE__
#include <gl3.h>
#include <gl3ext.h>
#endif

// OpenGL compatibility:
// https://en.wikipedia.org/wiki/Intel_HD_and_Iris_Graphics#Capabilities
// https://developer.apple.com/opengl/capabilities/

// Mac Intel GPU:
// sysctl -n machdep.cpu.brand_string
// google for processor id

std::string glsl_path = "roboschool/cpp-household/glsl"; // outside namespace

namespace SimpleRender {

using namespace Household;

static float line_vertex[] = {
+1,+1,0, -1,+1,0,
-1,+1,0, -1,-1,0,
-1,-1,0, +1,-1,0,
+1,-1,0, +1,+1,0,
-2,0,0, +2,0,0,
0,-2,0, 0,+2,0,

+1,+1,-1, +1,+1,+1,
-1,+1,-1, -1,+1,+1,
-1,-1,-1, -1,-1,+1,
+1,-1,-1, +1,-1,+1,
};

static float hud_vertex[] = {
0,0,0, 0,1,0, 1,0,0,
0,1,0, 1,1,0, 1,0,0
};

enum {
	ATTR_N_VERTEX,
	ATTR_N_NORMAL,
	ATTR_N_TEXCOORD,
};

static
std::string read_file(const std::string& fn)
{
	FILE* f = fopen( fn.c_str(), "rt");
	if (!f) throw std::runtime_error("cannot open '" + fn + "' with mode 'rt': " + strerror(errno) );
	off_t ret = fseek(f, 0, SEEK_END);
	if (ret==(off_t)-1) {
		fclose(f);
		throw std::runtime_error("cannot stat '" + fn + "': " + strerror(errno) );
	}
	uint32_t file_size = (uint32_t) ftell(f); // ftell returns long int
	fseek(f, 0, SEEK_SET);
	std::string str;
	if (file_size==0) {
		fclose(f);
		return str;
	}
	str.resize(file_size);
	try {
		int r = (int) fread(&str[0], file_size, 1, f);
		if (r==0) throw std::runtime_error("cannot read from '" + fn + "', eof");
		if (r!=1) throw std::runtime_error("cannot read from '" + fn + "': " + strerror(errno) );
		fclose(f);
	} catch (...) {
		fclose(f);
		throw;
	}
	return str;
}

shared_ptr<QGLShaderProgram> load_program(
	const std::string& vert_fn, const std::string& geom_fn, const std::string& frag_fn,
	const char* vert_defines, const char* geom_defines, const char* frag_defines)
{
	shared_ptr<QGLShaderProgram> prog(new QGLShaderProgram);
	std::string common_header;

	if (!vert_fn.empty()) {
		std::string vert_prog_text = read_file(glsl_path + "/" + vert_fn);
		common_header = "";
		if (vert_defines) common_header += vert_defines;
		prog->addShaderFromSourceCode(QGLShader::Vertex, (common_header + vert_prog_text).c_str());
		if (!prog->log().isEmpty())
			fprintf(stderr, "%s LOG: '%s'\n", vert_fn.c_str(), prog->log().toUtf8().data());
	}

	if (!geom_fn.empty()) {
		std::string geom_prog_text = read_file(glsl_path + "/" + geom_fn);
		common_header = "";
		if (geom_defines) common_header += geom_defines;
		prog->addShaderFromSourceCode(QGLShader::Geometry, (common_header + geom_prog_text).c_str());
		if (!prog->log().isEmpty())
			fprintf(stderr, "%s LOG: '%s'\n", geom_fn.c_str(), prog->log().toUtf8().data());
	}

	if (!frag_fn.empty()) {
		std::string frag_prog_text = read_file(glsl_path + "/" + frag_fn);
		common_header = "";
		if (frag_defines) common_header += frag_defines;
		prog->addShaderFromSourceCode(QGLShader::Fragment, (common_header + frag_prog_text).c_str());
		if (!prog->log().isEmpty())
			fprintf(stderr, "%s LOG: '%s'\n", frag_fn.c_str(), prog->log().toUtf8().data());
	}

	return prog;
}

Context::Context(const shared_ptr<Household::World>& world):
	weak_world(world)
{
}

Context::~Context()
{
	//glcx->makeCurrent(surf);
	// destructors here
}

static void glMultMatrix(const float* m)  { glMultMatrixf(m); }  // this helps with btScalar
static void glMultMatrix(const double* m) { glMultMatrixd(m); }

int Context::cached_bind_texture(const std::string& image_fn)
{
	auto f = bind_cache.find(image_fn);
	if (f!=bind_cache.end())
		return f->second;
	int b = 0;
	QImage img(QString::fromUtf8(image_fn.c_str()));
	if (img.isNull()) {
		fprintf(stderr, "cannot read image '%s'\n", image_fn.c_str());
	} else {
		//printf("image %ix%i <- %s\n", (int)img.width(), (int)img.height(), image_fn.c_str());
		glActiveTexture(GL_TEXTURE0);
		shared_ptr<Texture> t(new Texture());
		glBindTexture(GL_TEXTURE_2D, t->handle);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width(), img.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img.scanLine(0));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		textures.push_back(t);
		//assert(glGetError() == GL_NO_ERROR);
		b = t->handle;
		if (b==0) fprintf(stderr, "cannot bind texture '%s'\n", image_fn.c_str());
	}
	bind_cache[image_fn] = b; // we go on, even if texture isn't there -- better than crash
	return b;
}

void Context::load_missing_textures()
{
	shared_ptr<Household::World> world = weak_world.lock();
	if (!world) return;

	while (1) {
		bool did_anything = false;
		for (auto i=world->klass_cache.begin(); i!=world->klass_cache.end(); ++i) {
			shared_ptr<ThingyClass> klass = i->second.lock();
			if (!klass) {
				world->klass_cache.erase(i);
				did_anything = true;
				break;
			}
			shared_ptr<ShapeDetailLevels> v = klass->shapedet_visual;
			if (v->load_later_on) {
				v->load_later_on = false;
				load_model(v, v->load_later_fn, 1, v->load_later_transform);
			}
		}
		if (!did_anything) break;
	}
	//fprintf(stderr, "world now has %i classes\n", (int)world->classes.size());
	for (const weak_ptr<Thingy>& w: world->drawlist) {
		shared_ptr<Thingy> t = w.lock();
		if (!t) continue;
		if (!t->klass) continue;
		const shared_ptr<MaterialNamespace>& mats = t->klass->shapedet_visual->materials;
		if (!mats) continue;
		for (const std::pair<std::string, shared_ptr<Material>>& pair: mats->name2mtl) {
			shared_ptr<Material> mat = pair.second;
			if (mat->texture_loaded) continue;
			if (mat->diffuse_texture_image_fn.empty()) continue;
			mat->texture = cached_bind_texture(mat->diffuse_texture_image_fn);
			mat->texture_loaded = true;
		}
	}

	for (auto i=allocated_vaos.begin(); i!=allocated_vaos.end(); ) {
		if (i->unique()) {
			i = allocated_vaos.erase(i);
			continue;
		}
		++i;
	}

	for (auto i=allocated_buffers.begin(); i!=allocated_buffers.end(); ) {
		if (i->unique()) {
			i = allocated_buffers.erase(i);
			continue;
		}
		++i;
	}

	//fprintf(stderr, "allocated_vaos %i\n", (int)allocated_vaos.size());
	//fprintf(stderr, "allocated_buffers %i\n", (int)allocated_buffers.size());
}

static
void render_lines_overlay(const shared_ptr<Shape>& t)
{
	float r = float(1/256.0) * ((t->lines_color >> 16) & 255);
	float g = float(1/256.0) * ((t->lines_color >>  8) & 255);
	float b = float(1/256.0) * ((t->lines_color >>  0) & 255);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, t->lines.data());
	glColor3f(r, g, b);
	glDrawArrays(GL_LINES, 0, t->lines.size()/3);
	if (!t->raw_vertexes.empty()) {
		glColor3f(1.0f, 0.0f, 0.0f);
		glPointSize(3.0f);
		glVertexPointer(3, sizeof(btScalar)==sizeof(float) ? GL_FLOAT : GL_DOUBLE, 0, t->raw_vertexes.data());
		glDrawArrays(GL_POINTS, 0, t->raw_vertexes.size()/3);
	}
	glDisableClientState(GL_VERTEX_ARRAY);
}

void Context::initGL()
{
	//fprintf(stderr, "Shaders: %s\n", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

	//glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	if (initialized) return;
	initialized = true;

	program_tex = load_program("simple_texturing.vert.glsl", "", "simple_texturing.frag.glsl");
	program_tex->bindAttributeLocation("input_vertex", ATTR_N_VERTEX);
	program_tex->bindAttributeLocation("input_normal", ATTR_N_NORMAL);
	bool r0 = program_tex->link();
	assert(r0);
	location_input_matrix_modelview_inverse_transpose = program_tex->uniformLocation("input_matrix_modelview_inverse_transpose");
	location_input_matrix_modelview = program_tex->uniformLocation("input_matrix_modelview");
	location_enable_texture = program_tex->uniformLocation("enable_texture");
	location_texture = program_tex->uniformLocation("texture_id");
	location_uni_color = program_tex->uniformLocation("uni_color");
	location_multiply_color = program_tex->uniformLocation("multiply_color");
	// can be -1 if uniform is actually unused in glsl code

	program_displaytex = load_program("fullscreen_triangle.vert.glsl", "", "displaytex.frag.glsl");
	bool r1 = program_displaytex->link();
	assert(r1);

	program_hud = load_program("hud.vert.glsl", "", "displaytex.frag.glsl", "");
	bool r2 = program_hud->link();
	assert(r2);
	location_xywh = program_hud->uniformLocation("xywh");
	location_zpos = program_hud->uniformLocation("zpos");

#ifdef USE_SSAO
	if (ssao_enable)
		ssao_enable = _hbao_init();
#endif
	assert(glGetError() == GL_NO_ERROR);
}

ContextViewport::ContextViewport(const shared_ptr<Context>& cx, int W, int H, double near, double far, double hfov):
	cx(cx), W(W), H(H), near(near), far(far), hfov(hfov)
{
	side = std::max(W, H);
#ifdef USE_SSAO
	_depthlinear_init();
#endif

	fbuf_scene.reset(new Framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, fbuf_scene->handle);
#ifndef USE_SSAO
	tex_color.reset(new Texture());
	glBindTexture(GL_TEXTURE_2D, tex_color->handle);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, W, H);
	glBindTexture(GL_TEXTURE_2D, 0);
	tex_depthstencil.reset(new Texture());
	glBindTexture(GL_TEXTURE_2D, tex_depthstencil->handle);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, W, H);
	glBindTexture(GL_TEXTURE_2D, 0);
#endif
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,        tex_color->handle, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, tex_depthstencil->handle, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	W16  = W;
	W16 += 15;
	W16 &= ~0xF;
	hud_image = QImage(W16, H, QImage::Format_ARGB32);
	hud_image.fill(Qt::transparent);
	hud_texture.reset(new Texture());
	glBindTexture(GL_TEXTURE_2D, hud_texture->handle);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, W16, H);

	hud_vao.reset(new VAO);
	glBindVertexArray(hud_vao->handle);
	hud_vertexbuf.reset(new Buffer);
	glBindBuffer(GL_ARRAY_BUFFER, hud_vertexbuf->handle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(hud_vertex), hud_vertex, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
}

Texture::Texture()  { glGenTextures(1, &handle); }
Texture::~Texture()  { glDeleteTextures(1, &handle); }
Framebuffer::Framebuffer()  { glGenFramebuffers(1, &handle); }
Framebuffer::~Framebuffer()  { glDeleteFramebuffers(1, &handle); }
Buffer::Buffer()  { glGenBuffers(1, &handle); }
Buffer::~Buffer()  { glDeleteBuffers(1, &handle); }
VAO::VAO()  { glGenVertexArrays(1, &handle); }
VAO::~VAO()  { glDeleteVertexArrays(1, &handle); }

void ContextViewport::_render_single_object(const shared_ptr<Household::ShapeDetailLevels>& m, uint32_t options, int detail, const QMatrix4x4& at_pos)
{
	const std::vector<shared_ptr<Shape>>& shapes = m->detail_levels[detail];

	int cnt = shapes.size();
	for (int c=0; c<cnt; c++) {
		shared_ptr<Shape> t = shapes[c];
		if (!t->vao) {
			if (t->primitive_type!=Shape::MESH && t->primitive_type!=Shape::STATIC_MESH)
				primitives_to_mesh(m, detail, c);
			cx->_shape_to_vao(t);
			//if (t->primitive_type==Shape::DEBUG_LINES) {
			//if (options & VIEW_DEBUG_LINES)
			//render_lines_overlay(t);
		}

		btScalar origin_trans_n_rotate[16];
		t->origin.getOpenGLMatrix(origin_trans_n_rotate);
		QMatrix4x4 shape_pos;
		for (int i=0; i<16; i++) shape_pos.data()[i] = origin_trans_n_rotate[i];

		QMatrix4x4 obj_modelview = modelview * at_pos * shape_pos;
		cx->program_tex->setUniformValue(cx->location_input_matrix_modelview, obj_modelview);
		cx->program_tex->setUniformValue(cx->location_input_matrix_modelview_inverse_transpose, obj_modelview.inverted().transposed());

		uint32_t color = 0;
		uint32_t multiply_color = 0xFFFFFF;
		bool use_texture = false;
		bool meta = options & VIEW_METACLASS;
		if (!meta && t->material) {
			color = t->material->diffuse_color;
			multiply_color = t->material->multiply_color;
			use_texture = !t->t.empty() && t->material->texture;
		}
		if (options & VIEW_COLLISION_SHAPE) color ^= (0xFFFFFF & (uint32_t) (uintptr_t) t.get());
		int triangles = t->v.size();

		{
			float r = float(1/256.0) * ((multiply_color >> 16) & 255);
			float g = float(1/256.0) * ((multiply_color >>  8) & 255);
			float b = float(1/256.0) * ((multiply_color >>  0) & 255);
			cx->program_tex->setUniformValue(cx->location_multiply_color, r, g, b, 1);
		}
		{
			float a = float(1/256.0) * ((color >> 24) & 255);
			if (a==0) a = 1; // allow to specify colors in simple form 0xAABBCC
			float r = float(1/256.0) * ((color >> 16) & 255);
			float g = float(1/256.0) * ((color >>  8) & 255);
			float b = float(1/256.0) * ((color >>  0) & 255);
			cx->program_tex->setUniformValue(cx->location_uni_color, r, g, b, cx->pure_color_opacity*a);
		}

		glBindVertexArray(t->vao->handle);
		if (use_texture) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, t->material->texture);
		} else {
		}
		cx->program_tex->setUniformValue(cx->location_enable_texture, use_texture);
		cx->program_tex->setUniformValue(cx->location_texture, 0);

		glDrawArrays(GL_TRIANGLES, 0, triangles/3);
		glBindVertexArray(0);
	}
}

int ContextViewport::_objects_loop(int floor_visible, uint32_t view_options)
{
	shared_ptr<Household::World> world = cx->weak_world.lock();
	if (!world) return 0;

	int ms_render_objectcount = 0;
	for (auto i=world->drawlist.begin(); i!=world->drawlist.end(); ) {
		shared_ptr<Thingy> t = i->lock();
		if (!t) {
			i = world->drawlist.erase(i);
			continue;
		}
		if (t->visibility_123 > floor_visible) {
			++i;
			continue;
		}

		ms_render_objectcount++;

		btScalar m[16];
		t->bullet_position.getOpenGLMatrix(m);
		QMatrix4x4 pos;
		for (int i=0; i<16; i++) pos.data()[i] = m[i];
		t->bullet_local_inertial_frame.inverse().getOpenGLMatrix(m);
		QMatrix4x4 loc;
		for (int i=0; i<16; i++) loc.data()[i] = m[i];

		_render_single_object(t->klass->shapedet_visual, view_options, DETAIL_BEST, pos*loc);

		++i;
	}

	return ms_render_objectcount;
}

void Context::_generate_ruler_vao()
{
	ruler_vao.reset(new VAO);
	glBindVertexArray(ruler_vao->handle);
	ruler_vertexes.reset(new Buffer);
	glBindBuffer(GL_ARRAY_BUFFER, ruler_vertexes->handle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(line_vertex), line_vertex, GL_STATIC_DRAW);
	glVertexAttribPointer(ATTR_N_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(ATTR_N_VERTEX);
	glBindVertexArray(0);
}

void Context::_shape_to_vao(const boost::shared_ptr<Household::Shape>& shape)
{
	shape->vao.reset(new VAO);
	allocated_vaos.push_back(shape->vao);
	glBindVertexArray(shape->vao->handle);

	assert(shape->v.size() > 0);
	shape->buf_v.reset(new Buffer);
	allocated_buffers.push_back(shape->buf_v);
	glBindBuffer(GL_ARRAY_BUFFER, shape->buf_v->handle);
	glBufferData(GL_ARRAY_BUFFER, shape->v.size()*sizeof(shape->v[0]), shape->v.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(ATTR_N_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	assert(shape->norm.size() > 0);
	shape->buf_n.reset(new Buffer);
	allocated_buffers.push_back(shape->buf_n);
	glBindBuffer(GL_ARRAY_BUFFER, shape->buf_n->handle);
	glBufferData(GL_ARRAY_BUFFER, shape->norm.size()*sizeof(shape->norm[0]), shape->norm.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(ATTR_N_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	if (shape->t.size()) {
		shape->buf_t.reset(new Buffer);
		allocated_buffers.push_back(shape->buf_t);
		glBindBuffer(GL_ARRAY_BUFFER, shape->buf_t->handle);
		glBufferData(GL_ARRAY_BUFFER, shape->t.size()*sizeof(shape->t[0]), shape->t.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(ATTR_N_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(ATTR_N_TEXCOORD);
	}

	glEnableVertexAttribArray(ATTR_N_VERTEX);
	glEnableVertexAttribArray(ATTR_N_NORMAL);
	glBindVertexArray(0);
}

void ContextViewport::paint(float user_x, float user_y, float user_z, float wheel, float zrot, float xrot, Household::Camera* camera, int floor_visible, uint32_t view_options, float ruler_size)
{
	if (!cx->program_tex) {
		cx->initGL();
		cx->_generate_ruler_vao();
	}

	if (camera) floor_visible = 65535; // disable visibility_123 for robot cameras
	//fprintf(stderr, "0x%p render %ix%i\n", this, W, H);
#ifdef USE_SSAO
	glBindFramebuffer(GL_FRAMEBUFFER, fbuf_scene->handle);
#else
	// For rendering without shadows, just keep rendering on default framebuffer
	if (camera)
		glBindFramebuffer(GL_FRAMEBUFFER, fbuf_scene->handle); // unless we render offscreen for camera
#endif
	glViewport(0,0,W,H);

	float clear_color[4] = { 0.8, 0.8, 0.9, 1.0 };
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearBufferfv(GL_COLOR, 0, clear_color);
	glClearDepth(1.0);
	glClear(GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

	double xmin, xmax;
	xmax = near * tanf(hfov * M_PI / 180 * 0.5);
	xmin = -xmax;
	QMatrix4x4 projection;
	projection.frustum(xmin, xmax, xmin*H/W, xmax*H/W, near, far);

	QMatrix4x4 matrix_view;
	if (!camera) {
		matrix_view.translate(0, 0, -0.5*wheel);
		matrix_view.rotate(xrot, QVector3D(1.0, 0.0, 0.0));
		matrix_view.rotate(zrot, QVector3D(0.0, 0.0, 1.0));
		matrix_view.translate(-user_x, -user_y, -user_z);
		//gl_Position = projMat * viewMat * modelMat * vec4(position, 1.0);
	} else {
		shared_ptr<Thingy> attached = camera->camera_attached_to.lock();
		btTransform t = attached ? attached->bullet_position.inverse() : camera->camera_pose.inverse();
		btScalar m[16];
		t.getOpenGLMatrix(m);
		for (int i=0; i<16; i++) matrix_view.data()[i] = m[i];
	}

	modelview = projection * matrix_view;
	modelview_inverse_transpose = modelview.inverted().transposed();

	if (cx->need_load_missing_textures) {
		cx->need_load_missing_textures = false;
		cx->load_missing_textures();
	}

	cx->program_tex->bind();
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	cx->program_tex->setUniformValue(cx->location_enable_texture, false);
	cx->program_tex->setUniformValue(cx->location_uni_color, 0,0,0,0.8);
	cx->program_tex->setUniformValue(cx->location_texture, 0);
	cx->program_tex->setUniformValue(cx->location_input_matrix_modelview, modelview);
	cx->program_tex->setUniformValue(cx->location_input_matrix_modelview_inverse_transpose, modelview_inverse_transpose);
	if (~view_options & VIEW_CAMERA_BIT) {
		glBindVertexArray(cx->ruler_vao->handle);
		CHECK_GL_ERROR; // error often here, when context different, also set if (1) above to always enter this code block
		glDrawArrays(GL_LINES, 0, sizeof(line_vertex)/sizeof(float)/3);
		glBindVertexArray(0);
	}
	visible_object_count = _objects_loop(floor_visible, view_options);
	cx->program_tex->release();

#ifdef USE_SSAO
	if (cx->ssao_enable) {
		_hbao_prepare(projection.data());
		_depthlinear_paint(0);
		_ssao_run(0);
	}

	if (camera) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, fbuf_scene->handle); // buffer stays bound so camera can read pixels
		return;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, fbuf_scene->handle);
//	if      (ssao_debug==0) glBindFramebuffer(GL_READ_FRAMEBUFFER, fbuf_scene->handle);
//	else if (ssao_debug==1) glBindFramebuffer(GL_READ_FRAMEBUFFER, fbuf_depthlinear->handle);
//	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
//	if (ssao_debug < 2) {
//		glBlitFramebuffer(0,0,W,H, 0,0,W,H, GL_COLOR_BUFFER_BIT, GL_NEAREST);
//	} else if (ssao_debug==2) {
//		//_texture_paint( cx->cached_bind_texture("roboschool/models_furniture/IKEA_denimcloth.jpg") );
//		_texture_paint( tex_depthlinear->handle );
//	} else if (ssao_debug==3) {
//		_texture_paint( tex_depthstencil->handle );
//	}
//	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
}

void ContextViewport::_texture_paint(GLuint h)
{
	// FIXME
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glUseProgram(cx->program_displaytex->programId());
	glBindTexture(GL_TEXTURE_2D, h);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

int ContextViewport::hud_print_score(const std::string& score)
{
	const int SCORE_HEIGHT = 50;
	if (!score.empty()) {
		hud_print(QRect(0, 0, W, SCORE_HEIGHT), score.c_str(), 0x96FFFFFF, 0xFF000000, Qt::AlignLeft|Qt::AlignVCenter, true);
		return SCORE_HEIGHT;
	}
	return 0;
}

void ContextViewport::hud_update_start()
{
	cx->program_hud->bind();
	glBindVertexArray(hud_vao->handle);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, hud_texture->handle);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void ContextViewport::hud_update(const QRect& r_)
{
	QRect all_area(QPoint(0,0), hud_image.size());
	QRect r = all_area & r_;
	if (r.isEmpty()) return;
	if (1) {
		// Optimization: works!
		glPixelStorei(GL_UNPACK_ROW_LENGTH, hud_image.bytesPerLine()/4);
		glTexSubImage2D(GL_TEXTURE_2D,0,
			r.left(),r.top(),r.width(),r.height(),
			GL_BGRA, GL_UNSIGNED_BYTE, hud_image.scanLine(r.top()) + r.left()*4);
	} else {
		// Lines-only optimization possible
		glTexSubImage2D(GL_TEXTURE_2D,0, 0,0,W16,H, GL_RGBA, GL_UNSIGNED_BYTE, hud_image.scanLine(0));
	}
	cx->program_hud->setUniformValue(cx->location_xywh, -1+2*float(r.left())/W, -1+2*float(r.top())/H, 2*float(r.width())/W, 2*float(r.height())/H);
	cx->program_hud->setUniformValue(cx->location_zpos, GLfloat(0.9));
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void ContextViewport::hud_update_finish()
{
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	cx->program_hud->release();
}

void ContextViewport::hud_print(const QRect& r, const QString& msg_text, uint32_t bg, uint32_t fg, Qt::Alignment a, bool big_font)
{
	const int MARGIN = 10;
	if (!cx->score_font_inited) {
		cx->score_font_big.setFamily("Courier");
		cx->score_font_big.setPixelSize(48);
		cx->score_font_big.setBold(true);
		cx->score_font_small.setFamily("Courier");
		cx->score_font_small.setPixelSize(16);
		cx->score_font_small.setBold(true);
		cx->score_font_inited = true;
	}
	{
		QPainter p(&hud_image);
		p.setCompositionMode(QPainter::CompositionMode_Source);
		QColor bg_color;
		bg_color.setRgba(bg);
		p.fillRect(r, bg_color);
		p.setCompositionMode(QPainter::CompositionMode_SourceOver);
		p.setPen(QRgb(fg));
		p.setFont(big_font ? cx->score_font_big : cx->score_font_small);
		p.drawText(r.adjusted(MARGIN,0,-MARGIN,0), a, msg_text);
	}
	hud_update(r);
}

void opengl_init_before_app(const boost::shared_ptr<Household::World>& wref)
{
	QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL, true);
	QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
	QSurfaceFormat fmt;
	fmt.setSwapInterval(1);
	fmt.setProfile(QSurfaceFormat::CoreProfile);
	fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
	fmt.setVersion(4, 1);
	QSurfaceFormat::setDefaultFormat(fmt);
	QApplication::setApplicationDisplayName("Roboschool");
}

void opengl_init(const boost::shared_ptr<Household::World>& wref)
{
	boost::shared_ptr<SimpleRender::Context>& cx = wref->cx;
	cx.reset(new SimpleRender::Context(wref));
	cx->fmt = QSurfaceFormat::defaultFormat();
	
	cx->surf = new QOffscreenSurface();
	cx->surf->setFormat(cx->fmt);
	cx->surf->create();

	// This doesn't work no matter how you put it -- widgets do not share context.
	// QOpenGLContext::areSharing() is reporting true, put you can't reference framebuffers or VAOs.
	if (1) {
		QOpenGLContext* glcx = QOpenGLContext::globalShareContext();
		QSurfaceFormat fmt_req = cx->fmt;
		QSurfaceFormat fmt_got = glcx->format();
		int got_version = fmt_got.majorVersion()*1000 + fmt_got.minorVersion();
		bool ok_without_shadows = got_version >= 3003;
		bool ok_all_features    = got_version >= 4001;
		if (!ok_without_shadows) {
			fprintf(stderr, "\n\nCannot initialize OpenGL context.\n");
			fprintf(stderr, "Requested version: %i.%i\n", fmt_req.majorVersion(), fmt_req.minorVersion());
			fprintf(stderr, "Actual version: %i.%i\n", fmt_got.majorVersion(), fmt_got.minorVersion());
			fprintf(stderr, "(it must be at least 3.3 to work)\n");
			fprintf(stderr, "For possible fixes, see:\n\nhttps://github.com/openai/roboschool/issues/2\n\n");
			assert(0);
		}
		cx->glcx = glcx;
		cx->ssao_enable = ok_all_features;
		cx->glcx->makeCurrent(cx->surf);
	} else {
		cx->dummy_openglwidget = new QOpenGLWidget();
		cx->dummy_openglwidget->setFormat(cx->fmt);
		cx->dummy_openglwidget->makeCurrent();
		cx->dummy_openglwidget->show();
		QOpenGLContext* glcx = cx->dummy_openglwidget->context();
		assert(glcx);
		glcx->makeCurrent(cx->surf);

		QOpenGLWidget* test = new QOpenGLWidget();
		test->setFormat(cx->fmt);
		test->show();
		test->context() ;
		bool hurray = QOpenGLContext::areSharing(
			glcx,
			test->context());
		cx->glcx = glcx;
		assert(hurray);
	}
}

void opengl_init_existing_app(const boost::shared_ptr<Household::World>& wref)
{
	wref->cx.reset(new SimpleRender::Context(wref));
	wref->cx->fmt = QSurfaceFormat::defaultFormat();
	
	wref->cx->surf = new QOffscreenSurface();
	wref->cx->surf->setFormat(wref->cx->fmt);
	wref->cx->surf->create();

	QOpenGLContext* glcx = QOpenGLContext::globalShareContext();
	QSurfaceFormat fmt_got = glcx->format();
	int got_version = fmt_got.majorVersion()*1000 + fmt_got.minorVersion();
	bool ok_all_features    = got_version >= 4001;

	wref->cx->glcx = glcx;
	wref->cx->ssao_enable = ok_all_features;
	wref->cx->glcx->makeCurrent(wref->cx->surf);
}

} // namespace
