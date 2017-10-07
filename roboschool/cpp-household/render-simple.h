#pragma once
#include <ios>
#include "household.h"

#include <boost/shared_ptr.hpp>
#include <QtWidgets/QOpenGLWidget>
#include <QtGui/QSurface>
#include <QtGui/QOffscreenSurface>
#include <QtGui/qmatrix4x4.h>

struct aiMesh;
class QGLShaderProgram;
class QGLFramebufferObject;

namespace ssao {
class ScreenSpaceAmbientOcclusion;
}

// optimizes blur, by storing depth along with ssao calculation
// avoids accessing two different textures
#define USE_AO_SPECIALBLUR  1

namespace SimpleRender {

using boost::shared_ptr;
using boost::weak_ptr;

extern void primitives_to_mesh(const shared_ptr<Household::ShapeDetailLevels>& m, int want_detail, int shape_n);
extern shared_ptr<QGLShaderProgram> load_program(const std::string& vert_fn, const std::string& geom_fn, const std::string& frag_fn, const char* vert_defines=0, const char* geom_defines=0, const char* frag_defines=0);

enum {
	VIEW_CAMERA_BIT      = 0x0001,
	VIEW_LINES           = 0x0001,
	VIEW_DEBUG_LINES     = 0x0002,
	VIEW_COLLISION_SHAPE = 0x0004,
	VIEW_METACLASS       = 0x0010,
	VIEW_NO_HUD          = 0x1000,
	VIEW_NO_CAPTIONS     = 0x2000,
};

#define CHECK_GL_ERROR { int e = glGetError(); if (e!=GL_NO_ERROR) fprintf(stderr, "%s:%i ERROR: 0x%x\n", __FILE__, __LINE__, e); assert(e == GL_NO_ERROR); }

struct Texture {
	GLuint handle;
	Texture();
	~Texture();
};

struct Framebuffer {
	GLuint handle;
	Framebuffer();
	~Framebuffer();
};

struct Buffer {
	GLuint handle;
	Buffer();
	~Buffer();
};

struct VAO {
	GLuint handle;
	VAO();
	~VAO();
};

const int AO_RANDOMTEX_SIZE = 4;
const int MAX_SAMPLES = 8;
const int NUM_MRT = 8;
const int HBAO_RANDOM_SIZE = AO_RANDOMTEX_SIZE;
const int HBAO_RANDOM_ELEMENTS = HBAO_RANDOM_SIZE*HBAO_RANDOM_SIZE;

struct Context {
	Context(const shared_ptr<Household::World>& world);
	~Context();

	QFont score_font_big;
	QFont score_font_small;
	bool score_font_inited = false;
	bool slowmo = false;

	weak_ptr<Household::World> weak_world;

	QSurfaceFormat fmt;
	QOffscreenSurface* surf = 0;
	QOpenGLContext* glcx = 0;
	QOpenGLWidget* dummy_openglwidget = 0;
	std::vector<shared_ptr<Texture>> textures;

	int location_input_matrix_modelview_inverse_transpose;
	int location_input_matrix_modelview;
	int location_enable_texture;
	int location_texture;
	int location_uni_color;
	int location_multiply_color;
	shared_ptr<QGLShaderProgram> program_tex;

	int location_clipInfo;
	shared_ptr<QGLShaderProgram> program_depth_linearize;
	//shared_ptr<QGLShaderProgram> program_depth_linearize_msaa;

	shared_ptr<QGLShaderProgram> program_displaytex;

	int location_RadiusToScreen;
	int location_R2;             // 1/radius
	int location_NegInvR2;       // radius * radius
	int location_NDotVBias;
	int location_InvFullResolution;
	int location_InvQuarterResolution;
	int location_AOMultiplier;
	int location_PowExponent;
	int location_projInfo;
	int location_projScale;
	int location_projOrtho;
	int location_float2Offsets;
	int location_jitters;
	int location_texLinearDepth;
	int location_texRandom;

	bool ssao_enable = true;
	shared_ptr<QGLShaderProgram> program_hbao_calc;
	shared_ptr<QGLShaderProgram> program_calc_blur;

	std::list<shared_ptr<VAO>>    allocated_vaos;
	std::list<shared_ptr<Buffer>> allocated_buffers;

	int location_xywh;
	int location_zpos;
	shared_ptr<QGLShaderProgram> program_hud;

	float pure_color_opacity = 1.0;

	int a=0, b=0, c=0;

	bool initialized = false;

	void load_missing_textures();
	bool need_load_missing_textures = true;

	int cached_bind_texture(const std::string& image_fn);
	std::map<std::string, int> bind_cache;

	shared_ptr<struct UsefulStuff> useful = 0;
	void initGL();

	bool _hbao_init();
	shared_ptr<Texture> hbao_random;
	shared_ptr<Texture> hbao_randomview[MAX_SAMPLES];

	shared_ptr<VAO> ruler_vao;
	shared_ptr<Buffer> ruler_vertexes;

	void _shape_to_vao(const boost::shared_ptr<Household::Shape>& shape);
	void _generate_ruler_vao();
};

class ContextViewport {
public:
	shared_ptr<Context> cx;
	int visible_object_count;

	int W, H, W16;
	double side, near, far, hfov;
	QMatrix4x4 modelview;
	QMatrix4x4 modelview_inverse_transpose;

	int  ssao_debug = 0;
	bool ortho = false;
	bool blur = false;
	int samples = 1;

	// heavy shadow
	//float ssao_radius    = 1.6; // search for "ssao.params"
	//float ssao_intensity = 3.0;
	//float ssao_bias      = 0.70;

	// light
	float ssao_radius    = 0.7;
	float ssao_intensity = 0.9;
	float ssao_bias      = 0.8;

	ContextViewport(const shared_ptr<Context>& cx, int W, int H, double near, double far, double hfov);

	shared_ptr<Framebuffer> fbuf_scene;
	shared_ptr<Framebuffer> fbuf_depthlinear;
	shared_ptr<Framebuffer> fbuf_viewnormal;
	shared_ptr<Framebuffer> fbuf_hbao_calc;
	shared_ptr<Framebuffer> fbuf_hbao2_deinterleave;
	shared_ptr<Framebuffer> fbuf_hbao2_calc;

	shared_ptr<Texture> tex_color;
	shared_ptr<Texture> tex_depthstencil;
	shared_ptr<Texture> tex_depthlinear;
	shared_ptr<Texture> tex_viewnormal;
	shared_ptr<Texture> hbao_result;
	shared_ptr<Texture> hbao_blur;
	shared_ptr<Texture> hbao2_deptharray;
	shared_ptr<Texture> hbao2_depthview[HBAO_RANDOM_ELEMENTS];
	shared_ptr<Texture> hbao2_resultarray;

	QImage              hud_image;
	shared_ptr<Texture> hud_texture;
	shared_ptr<VAO>     hud_vao;
	shared_ptr<Buffer>  hud_vertexbuf;
	void hud_update_start();
	void hud_update(const QRect& r);
	int  hud_print_score(const std::string& score);
	void hud_print(const QRect& r, const QString& msg_text, uint32_t bg, uint32_t fg, Qt::Alignment a, bool big_font);
	void hud_update_finish();

	void paint(float user_x, float user_y, float user_z, float wheel, float zrot, float xrot, Household::Camera* camera, int floor_visible, uint32_t view_options, float ruler_size);
private:
	void _depthlinear_init();
	void _depthlinear_paint(int sample_idx);
	void _hbao_prepare(const float* proj_matrix);
	void _ssao_run(int sampleIdx);
	void _texture_paint(GLuint h);
	int  _objects_loop(int floor_visible, uint32_t view_options);
	void _render_single_object(const shared_ptr<Household::ShapeDetailLevels>& m, uint32_t f, int detail, const QMatrix4x4& at_pos);
};

extern void opengl_init_before_app(const boost::shared_ptr<Household::World>& wref);
extern void opengl_init(const boost::shared_ptr<Household::World>& wref);
extern void opengl_init_existing_app(const boost::shared_ptr<Household::World>& wref);

} // namespace
