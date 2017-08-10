#include "render-simple.h"

#include <QtWidgets/QOpenGLWidget>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

using boost::shared_ptr;

struct KeyCallback {
	virtual ~KeyCallback()  { }
	virtual void key_callback(int event_type, int key, int modifiers) =0;
};

struct ConsoleMessage {
	QString msg_text;
	QImage  msg_image;
	qint64 ts;
	btVector3 pos;
	void render(uint32_t color, int width=0);
};

class Viz: public QOpenGLWidget {
public:
	shared_ptr<SimpleRender::Context> cx;
	shared_ptr<SimpleRender::ContextViewport> render_viewport;
	bool render_viewport_resized = true;
	shared_ptr<SimpleRender::Framebuffer> dummy_fbuf;
	QFont font_score;

	void render_on_offscreen_surface();

	Viz(const shared_ptr<SimpleRender::Context>& cx);
	~Viz();
	void initializeGL();
	void _render_on_correctly_set_up_context();
	void _paint_hud();
	void paintGL();
	void resizeGL(int w, int h);
	void keyPressEvent(QKeyEvent* kev);
	void keyReleaseEvent(QKeyEvent* kev);
	void wheelEvent(QWheelEvent* wev);
	void mousePressEvent(QMouseEvent* mev);
	void mouseReleaseEvent(QMouseEvent* mev);
	void mouseMoveEvent(QMouseEvent* mev);
	bool event(QEvent* ev);
	void timeout();

	float user_x = 0;
	float user_z = 0;
	float user_y = 0;
	float yrot = -60;
	float zrot = 0;
	float wheel = 10.00;
	float ruler_size = 1.0;
	int floor_visible = 10;
	void user_move(float hor, float vert);

	enum { DRAG_NONE, DRAG_ROTATE, DRAG_MOVE };
	int drag = 0;
	double mouse_prev_x = 0;
	double mouse_prev_y = 0;
	double mouse_init_x = 0;
	double mouse_init_y = 0;

	double ms_render = 0;
	int ms_render_objectcount = 0;

	uint32_t view_options = 0;
	float dup_opacity = 0.5;
	int dup_transparent_mode = 0;

	//shared_ptr<Household::Camera> camera;

	std::vector<float> obs;
	std::vector<float> obs_hist;
	std::vector<float> action;
	std::vector<float> action_hist;
	std::vector<float> reward;
	std::vector<float> reward_hist;
	std::string score;
	void history_advance(bool only_ensure_correct_sizes);
	void drawhist(QPainter& p, const char* label, int bracketn, const QRect& r, const float* sensors_hist, const float* sensors);

	int win_w, win_h;
	bool resized = false;
	std::list<ConsoleMessage> console;
	void test_window_print(const std::string& msg);

	std::list<ConsoleMessage> billboards;
	void test_window_billboard(const btVector3& pos, const std::string& s, uint32_t color);

	ConsoleMessage caption;
	void test_window_big_caption(const std::string& s);

	boost::weak_ptr<KeyCallback> key_callback;
	void activate_key_callback(int event_type, int key, int modifiers);
};

class VizCamera: public QWidget {
public:
	boost::weak_ptr<Household::Camera> cref;
	boost::weak_ptr<KeyCallback> key_callback;
	std::string score;
	const int MARGIN = 20;

	VizCamera(const shared_ptr<Household::Camera>& cref): cref(cref)  { }
	void paintEvent(QPaintEvent* ev);
	void keyPressEvent(QKeyEvent* kev);
	void keyReleaseEvent(QKeyEvent* kev);
	QSize sizeHint() const;

	void activate_key_callback(int event_type, int key, int modifiers);
};
