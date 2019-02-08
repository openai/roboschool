#include <boost/python.hpp>
#include <boost/weak_ptr.hpp>

#include "render-glwidget.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>
#include <QtGui/QWindow>
#include <QtCore/QElapsedTimer>
#include <QtCore/QBuffer>

using boost::shared_ptr;
using namespace boost::python;

namespace Household {
btScalar SCALE = 1.0;
btScalar COLLISION_MARGIN = 0.002*SCALE;
}

using Household::SCALE;

extern std::string glsl_path;

inline float square(float x)  { return x*x; }

void list2vec(const boost::python::list& ns, std::vector<btScalar>& v)
{
	int L = len(ns);
	v.resize(L);
	for (int i=0; i<L; ++i) {
		v[i] = boost::python::extract<btScalar>(ns[i])*SCALE;
	}
}

struct Pose {
	btScalar x, y, z;
	btScalar qx, qy, qz, qw;
	Pose(): x(0), y(0), z(0), qx(0), qy(0), qz(0), qw(1)  { }
	void set_xyz(btScalar _x, btScalar _y, btScalar _z)  { x = _x*SCALE; y = _y*SCALE; z = _z*SCALE; }
	void move_xyz(btScalar _x, btScalar _y, btScalar _z)  { x += _x*SCALE; y += _y*SCALE; z += _z*SCALE; }
	void set_quaternion(btScalar _x, btScalar _y, btScalar _z, btScalar _w)  { qx = _x; qy = _y; qz = _z; qw = _w; }
	void set_rpy(btScalar r, btScalar p, btScalar y)
	{
		double t0 = std::cos(y * 0.5);
		double t1 = std::sin(y * 0.5);
		double t2 = std::cos(r * 0.5);
		double t3 = std::sin(r * 0.5);
		double t4 = std::cos(p * 0.5);
		double t5 = std::sin(p * 0.5);
		qw = t0 * t2 * t4 + t1 * t3 * t5;
		qx = t0 * t3 * t4 - t1 * t2 * t5;
		qy = t0 * t2 * t5 + t1 * t3 * t4;
		qz = t1 * t2 * t4 - t0 * t3 * t5;
	}
	void rotate_z(btScalar angle)  {  btQuaternion t(qx, qy, qz, qw); btQuaternion t2; t2.setRotation(btVector3(0,0,1), angle); t = t2*t; qx = t.x(); qy = t.y(); qz = t.z(); qw = t.w(); }
	btTransform convert_to_bt_transform() const { return btTransform(btQuaternion(qx, qy, qz, qw), btVector3(x, y, z)); }
	void from_bt_transform(const btTransform& tr)  { btVector3 t = tr.getOrigin(); btQuaternion q = tr.getRotation(); set_xyz(t.x()/SCALE, t.y()/SCALE, t.z()/SCALE); set_quaternion(q.x(), q.y(), q.z(), q.w()); }
	tuple xyz()  { return make_tuple( x/SCALE, y/SCALE, z/SCALE ); }
	tuple quatertion()  { return make_tuple( qx, qy, qz, qw ); }
	tuple rpy()
	{
		btScalar sqw = qw*qw;
		btScalar sqx = qx*qx;
		btScalar sqy = qy*qy;
		btScalar sqz = qz*qz;
		btScalar t2 = -2.0 * (qx*qz - qy*qw) / (sqx + sqy + sqz + sqw);
		btScalar yaw   = atan2(2.0 * (qx*qy + qz*qw), ( sqx - sqy - sqz + sqw));
		btScalar roll  = atan2(2.0 * (qy*qz + qx*qw), (-sqx - sqy + sqz + sqw));
		t2 = t2 >  1.0f ?  1.0f : t2;
		t2 = t2 < -1.0f ? -1.0f : t2;
		btScalar pitch = asin(t2);
		return make_tuple(roll, pitch, yaw);
	}
	Pose dot(const Pose& other)
	{
		Pose r;
		r.from_bt_transform(convert_to_bt_transform() * other.convert_to_bt_transform());
		return r;
	}
};

struct Thingy {
	shared_ptr<Household::Thingy> tref;
	shared_ptr<Household::World>  wref;
	Thingy(const shared_ptr<Household::Thingy>& t, const shared_ptr<Household::World>& w): tref(t), wref(w)  { }

	long __hash__()  { return (long) (uintptr_t) tref.get(); }   // sizeof(int)==4 && sizeof(uintptr_t)==8 -- common case for linux64
	bool __eq__(const Thingy& other)  { return tref.get()==other.tref.get(); }

	//btScalar highest_point()  { return tref->highest_point/SCALE; }
	Pose pose()  { Pose r; r.from_bt_transform(tref->bullet_position); return r; }
	tuple speed()  { assert(tref->bullet_queried_at_least_once); return make_tuple(tref->bullet_speed.x()/SCALE, tref->bullet_speed.y()/SCALE, tref->bullet_speed.z()/SCALE); }
	tuple angular_speed()  { assert(tref->bullet_queried_at_least_once); return make_tuple(tref->bullet_angular_speed.x(), tref->bullet_angular_speed.y(), tref->bullet_angular_speed.z()); }
	//void push(btScalar impx, btScalar impy, btScalar impz)  { tref->bullet_body->applyCentralImpulse(btVector3(impx*SCALE, impy*SCALE, impz*SCALE)); tref->bullet_body->activate(true); }
	//void turn(btScalar r, btScalar p, btScalar y)  { tref->bullet_body->applyTorqueImpulse(btVector3(r*SCALE, p*SCALE, y*SCALE)); tref->bullet_body->activate(true); }

	void set_name(const std::string& name)  { tref->name = name; }
	std::string get_name()  { return tref->name; }

	void set_visibility_123(int f)  { tref->visibility_123 = f; }
	int get_visibility_123()  { return tref->visibility_123; }

	void set_multiply_color(const std::string& tex, uint32_t c)  { tref->set_multiply_color(tex, &c, 0); } // this works on mostly white textures
	//void replace_texture(const std::string& tex, std::string newfn)  { tref->set_multiply_color(tex, 0, &newfn); }
	void assign_metaclass(uint8_t mclass)  { tref->klass->metaclass = mclass; }

	std::list<boost::weak_ptr<Household::Thingy>> sleep_list;
	boost::python::list contact_list()
	{
		boost::python::list r;
		if (!tref->is_sleeping()) {
			sleep_list.clear();
			for (const shared_ptr<Household::Thingy>& t: wref->bullet_contact_list(tref)) {
				r.append(Thingy(t, wref));
				sleep_list.push_back(t);
			}
		} else {
			for (const boost::weak_ptr<Household::Thingy>& t_: sleep_list) {
				shared_ptr<Household::Thingy> t = t_.lock();
				if (t) r.append(Thingy(t, wref));
			}
		}
		return r;
	}
};

struct App {
	QApplication* app;
	QEventLoop* loop;

	App()
	{
		static int argc = 1;
		static const char* argv[] = { "Roboschool Simulator" };
		QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
		app = new QApplication(argc, const_cast<char**>(argv));
		loop = new QEventLoop();
	}

	virtual ~App()
	{
		delete loop;
		delete app;
	}

	void process_events()
	{
		loop->processEvents(QEventLoop::AllEvents);
	}
};

static boost::weak_ptr<App> the_app;

shared_ptr<App> app_create_as_needed(const shared_ptr<Household::World>& wref)
{
	shared_ptr<App> app = the_app.lock();
	if (app) {
		wref->app_ref = app;
		//SimpleRender::opengl_init_existing_app(wref);
		return app;
	}
	SimpleRender::opengl_init_before_app(wref);
	app.reset(new App);
	the_app = app;
	SimpleRender::opengl_init(wref);
	wref->app_ref = app;
	return app;
}

struct PythonKeyCallback: KeyCallback {
	boost::python::object py_callback;
	void key_callback(int event_type, int key, int modifiers)
	{
		py_callback(event_type, key, modifiers);
	}
};

struct Camera {
	shared_ptr<Household::Camera> cref;
	shared_ptr<Household::World> wref;

	Camera(const shared_ptr<Household::Camera>& cref, const shared_ptr<Household::World>& wref): cref(cref), wref(wref)  { }
	std::string name()  { return cref->camera_name; }
	tuple resolution()  { return make_tuple(cref->camera_res_w, cref->camera_res_h); }

	VizCamera* window = 0;
	shared_ptr<App> app; // keep app alive until we delete window
	shared_ptr<PythonKeyCallback> cb;

	bool test_window()
	{
		if (window) {
			if (!window->isVisible()) return false;
			window->update();
			return true;
		}
		if (!app) app = app_create_as_needed(wref);
		window = new VizCamera(cref);
		window->show();
		window->key_callback = cb;
		return true;
	}

	void set_key_callback(boost::python::object f)
	{
		cb.reset(new PythonKeyCallback);
		cb->py_callback = f;
		if (window)
			window->key_callback = cb;
	}

	void test_window_score(const std::string& score)
	{
		cref->score = score;
	}

	~Camera()
	{
		delete window;
		app.reset();
	}

	void set_pose(const Pose& p) { cref->camera_pose = p.convert_to_bt_transform(); }
	void set_hfov(float hor_fov) { cref->camera_hfov = hor_fov; }
	void set_near(float near)    { cref->camera_near = near; }
	void set_far(float far)      { cref->camera_near = far; }

	boost::python::object render(bool render_depth, bool render_labeling, bool print_timing)
	{
		if (!app) app = app_create_as_needed(wref);
		cref->camera_render(wref->cx, render_depth, render_labeling, print_timing);
		return make_tuple(
				object(handle<>(PyBytes_FromStringAndSize(cref->camera_rgb.c_str(), cref->camera_rgb.size()))),
				render_depth ? object(handle<>(PyBytes_FromStringAndSize(cref->camera_depth.c_str(), cref->camera_depth.size()))) : object(),
				render_depth ? object(handle<>(PyBytes_FromStringAndSize(cref->camera_depth_mask.c_str(), cref->camera_depth_mask.size()))) : object(),
				render_labeling ? object(handle<>(PyBytes_FromStringAndSize(cref->camera_labeling.c_str(), cref->camera_labeling.size()))) : object(),
				render_labeling ? object(handle<>(PyBytes_FromStringAndSize(cref->camera_labeling_mask.c_str(), cref->camera_labeling_mask.size()))) : object()
				);
	}

	void move_and_look_at(float from_x, float from_y, float from_z, float obj_x, float obj_y, float obj_z)
	{
		Pose pose;
		pose.set_xyz(0, 0, 0);
		float dist = sqrt( square(obj_x-from_x) + square(obj_y-from_y) );
		pose.set_rpy(M_PI/2 + atan2(obj_z-from_z, dist), 0, 0);
		pose.rotate_z( atan2(obj_y-from_y, obj_x-from_x) - M_PI/2 );
		pose.move_xyz( from_x, from_y, from_z );
		set_pose(pose);
	}
};

struct Joint {
	shared_ptr<Household::Joint> jref;

	Joint(const shared_ptr<Household::Joint>& jref): jref(jref)  { }
	std::string name()  { return jref->joint_name; }
	void set_motor_torque(float q)  { jref->set_motor_torque(q); }
	void set_target_speed(float target_speed, float kd, float maxforce)  { jref->set_target_speed(target_speed, kd, maxforce); }
	void set_servo_target(float target_pos, float kp, float kd, float maxforce)  { jref->set_servo_target(target_pos, kp, kd, maxforce); }
	void reset_current_position(float pos, float vel)  { jref->reset_current_position(pos, vel); }
	boost::python::tuple current_position()  { return make_tuple(jref->joint_current_position, jref->joint_current_speed); }
	boost::python::tuple current_relative_position()  { float pos, speed; jref->joint_current_relative_position(&pos, &speed); return make_tuple(pos, speed); }
	boost::python::tuple limits()  { return make_tuple(jref->joint_limit1, jref->joint_limit2, jref->joint_max_force, jref->joint_max_velocity); }

	std::string type()
	{
		switch (jref->joint_type) {
		//case Household::Joint::ROTATIONAL_SERVO: return "servo";
		case Household::Joint::ROTATIONAL_MOTOR: return "motor";
		//case Household::Joint::LINEAR_SERVO: return "linear_servo";
		case Household::Joint::LINEAR_MOTOR: return "linear_motor";
		default: return "unknown";
		}
	}
};

struct Robot {
	shared_ptr<Household::Robot> rref;
	shared_ptr<Household::World> wref;
	Robot(const shared_ptr<Household::Robot>& rref, const shared_ptr<Household::World>& wref): rref(rref), wref(wref)  { }
	//boost::python::list cameras()  { boost::python::list r; for (auto c: rref->cameras) r.append(Camera(c, wref)); return r; }
	boost::python::list joints()  { boost::python::list r; for (auto j: rref->joints) if (j) r.append(Joint(j)); return r; }
	boost::python::list parts()   { boost::python::list r; for (auto p: rref->robot_parts) if (p) r.append(Thingy(p, wref)); return r; }
	Thingy root_part()  { return Thingy(rref->root_part, wref); }
	Pose pose()  { Pose r; r.from_bt_transform(rref->root_part->bullet_position); return r; }
	void query_position()  { wref->query_body_position(rref); } // necessary for robot that is just created, before any step() done
	void set_pose(const Pose& p)  { wref->robot_move(rref, p.convert_to_bt_transform(), btVector3(0,0,0)); }
	void set_pose_and_speed(const Pose& p, float vx, float vy, float vz)  { wref->robot_move(rref, p.convert_to_bt_transform(), btVector3(vx,vy,vz)); }
	//void replace_texture(const std::string& material_name, const std::string& new_jpeg_png)  { rref->replace_texture(material_name, new_jpeg_png); }
};

struct World {
	shared_ptr<Household::World> wref;
	shared_ptr<App> app;

	World(float gravity, float timestep)
	{
		wref.reset(new Household::World);
		wref->bullet_init(gravity*SCALE, timestep);
	}

	~World()
	{
		delete window;
	}

	void clean_everything() { wref->clean_everything(); }

	Thingy load_thingy(const std::string& mesh_or_urdf_filename, const Pose& pose, double scale, double mass, int color, bool decoration_only)
	{
		return Thingy(wref->load_thingy(mesh_or_urdf_filename, pose.convert_to_bt_transform(), scale*SCALE, mass, color, decoration_only), wref);
	}

	Robot load_urdf(const std::string& fn, const Pose& pose, bool fixed_base, bool self_collision)
	{
		Robot r(wref->load_urdf(fn, pose.convert_to_bt_transform(), fixed_base, self_collision), wref);
		return r;
	}

	boost::python::list load_sdf(const std::string& fn)
	{
		std::list<shared_ptr<Household::Robot>> rlist = wref->load_sdf_mjcf(fn, false);
		boost::python::list ret;
		for (auto r: rlist)
			ret.append(Robot(r, wref));
		return ret;
	}

	boost::python::list load_mjcf(const std::string& fn)
	{
		std::list<shared_ptr<Household::Robot>> rlist = wref->load_sdf_mjcf(fn, true);
		boost::python::list ret;
		for (auto r: rlist)
			ret.append(Robot(r, wref));
		return ret;
	}

	Thingy debug_rect(double x1, double y1, double x2, double y2, double h, uint32_t color)
	{
		return Thingy(wref->debug_rect(x1*SCALE, y1*SCALE, x2*SCALE, y2*SCALE, h*SCALE, color), wref);
	}

	Thingy debug_line(double x1, double y1, double z1, double x2, double y2, double z2, uint32_t color)
	{
		return Thingy(wref->debug_line(x1*SCALE, y1*SCALE, z1*SCALE, x2*SCALE, y2*SCALE, z2*SCALE, color), wref);
	}

	Thingy debug_sphere(double x, double y, double z, double rad, uint32_t color)
	{
		return Thingy(wref->debug_sphere(x*SCALE, y*SCALE, z*SCALE, rad*SCALE, color), wref);
	}

	double ts()  { return wref->ts; }

	bool step(int repeat)
	{
		bool have_window = window && window->isVisible();
		bool slowmo = wref->cx && wref->cx->slowmo && window && window->isVisible();
		if (slowmo) {
			wref->bullet_step(1);
			int counter = 1;
			int old_s = 0;
			for (int i=0; i<10; i++) {
				int new_s = i*repeat/10;
				if (new_s > old_s) {
					old_s = new_s;
					wref->bullet_step(1);
					counter++;
				}
				if (slowmo || i==0) {
					app->process_events();
					if (have_window) {
						window->render_on_offscreen_surface();
						window->repaint();
					}
				}
			}
			assert(counter==repeat);
		} else {
			wref->bullet_step(repeat);
			if (app) {
				app->process_events();
				if (have_window) {
					window->render_on_offscreen_surface();
					window->repaint();
				}
			}
		}

		return false;
	}

	Viz* window = 0;
	shared_ptr<PythonKeyCallback> cb;
	int ms_countdown = 0;

	bool test_window()
	{
		if (!app) app = app_create_as_needed(wref);
		if (window) {
			app->process_events();
			if (window->isVisible()) {
				if (ms_countdown==0) {
					if (wref->cx->slowmo) {
						window->setWindowTitle("SLOWMO");
					} else {
						window->setWindowTitle(
							QString("%1 objects, %2ms bullet, %3ms render")
							. arg(window->ms_render_objectcount)
							. arg(wref->performance_bullet_ms, 0, 'f', 2)
							. arg(window->ms_render, 0, 'f', 2) );
					}
					ms_countdown = 10;
				} else {
					ms_countdown -= 1;
				}
				return true;
			}
			return false; // Not visible (can't create again, app should quit)
		}
		window = new Viz(wref->cx);
		window->key_callback = cb;
		window->wheel /= SCALE;
		QDesktopWidget* desk = QApplication::desktop();
		qreal rat = desk->windowHandle()->devicePixelRatio();
		window->resize(int(1280/rat), int(1024/rat));
		window->show();
		window->test_window_big_caption(big_caption);
		return true;
	}

	void set_key_callback(boost::python::object f)
	{
		cb.reset(new PythonKeyCallback);
		cb->py_callback = f;
		if (window)
			window->key_callback = cb;
	}

	void test_window_print(const std::string& msg)
	{
		if (window) window->test_window_print(msg);
	}

	void test_window_billboard(const Pose& t, const std::string& msg, uint32_t color)
	{
		if (!window) return;
		window->test_window_billboard(btVector3(t.x, t.y, t.z), msg, color);
	}

	std::string big_caption;
	void test_window_big_caption(const std::string& msg)
	{
		if (!window) big_caption = msg;
		else window->test_window_big_caption(msg);
	}

	void test_window_observations(const boost::python::list& obs)
	{
		if (!window) return;
		int cnt = boost::python::len(obs);
		window->obs.resize(cnt);
		for (int i=0; i<cnt; i++)
			window->obs[i] = extract<float>(obs[i])();
	}

	void test_window_actions(const boost::python::list& act)
	{
		if (!window) return;
		int cnt = boost::python::len(act);
		window->action.resize(cnt);
		for (int i=0; i<cnt; i++)
			window->action[i] = extract<float>(act[i])();
	}

	void test_window_rewards(const boost::python::list& reward)
	{
		if (!window) return;
		int cnt = boost::python::len(reward);
		window->reward.resize(cnt);
		for (int i=0; i<cnt; i++)
			window->reward[i] = extract<float>(reward[i])();
	}

	void test_window_score(const std::string& score)
	{
		if (!window) return;
		window->score = score;
	}

	void test_window_history_advance()
	{
		if (!window) return;
		window->history_advance(false);
	}

	void test_window_history_reset()
	{
		if (!window) return;
		window->reward_hist.resize(0);
		window->action_hist.resize(0);
		window->obs_hist.resize(0);
	}

	Camera new_camera_free_float(
		int camera_res_w, int camera_res_h, const std::string& camera_name)
	{
		shared_ptr<Household::Camera> cam(new Household::Camera);
		cam->camera_name = camera_name;
		cam->camera_res_w = camera_res_w;
		cam->camera_res_h = camera_res_h;
		return Camera(cam, wref);
	}

//	Thingy tool_wall(
//		btScalar grid_size, float tex1, float tex_v_zero,
//		const boost::python::list& path_, bool closed,
//		const boost::python::list& low_,
//		const boost::python::list& high_,
//		const std::string& side_tex, float side_tex_scale,
//		const std::string& top_tex,  float top_tex_scale,
//		const std::string& bott_tex, float bott_tex_scale,
//		const std::string& butt_tex, float butt_tex_scale)
//	{
//		std::vector<btScalar> path; list2vec(path_, path);
//		std::vector<btScalar> low;  list2vec(low_, low);
//		std::vector<btScalar> high; list2vec(high_, high);
//		return Thingy(wref->tool_wall(
//			grid_size*SCALE, tex1*SCALE, tex_v_zero*SCALE, path, closed, low, high,
//			side_tex, side_tex_scale,
//			top_tex,  top_tex_scale,
//			bott_tex, bott_tex_scale,
//			butt_tex, butt_tex_scale), wref);
//	}

//	Thingy tool_quad_prism(
//		float tex1, float tex1_zero,
//		const boost::python::list& path_,
//		btScalar low,
//		btScalar high,
//		const std::string& side_tex_,
//		const std::string& top_tex_,
//		const std::string& bottom_tex_)
//	{
//		std::vector<btScalar> path; list2vec(path_, path);
//		return Thingy(wref->tool_quad_prism(tex1*SCALE, tex1_zero*SCALE, path, low*SCALE, high*SCALE, side_tex_, top_tex_, bottom_tex_), wref);
//	}

	void set_glsl_path(const std::string& dir)
	{
		glsl_path = dir;
	}
};

Pose tip_z(btScalar x, btScalar y, btScalar z, btScalar yaw)
{
	Pose p;
	p.set_xyz(x, y, z);
	p.set_rpy(0, 0, 0);
	p.rotate_z(yaw*M_PI/180);
	return p;
}

Pose tip_y(float x, float y, float z, float yaw)
{
	Pose p;
	p.set_xyz(x, y, z);
	p.set_rpy(0, M_PI/2, 0);
	p.rotate_z(yaw*M_PI/180);
	return p;
}


void sanity_checks()
{
	float t;
	int r = sscanf("5.5", "%f", &t);
	if (r!=1 || t!=5.5f) {
		fprintf(stderr, "Sanity check failed: sscanf(\"5.5\", \"%%f\", ...)) doesn't work. Fix it by LC_ALL=C or LC_NUMERIC=en_GB.UTF-8\n");
		fprintf(stderr, "(because a lot of .xml files use \".\" as decimal separator, scanf should work on them!)\n");
		exit(1);
	}

	QImage image(8, 8, QImage::Format_RGB32);
	image.fill(0xFF0000);
        QByteArray ba;
        {
		QBuffer buffer(&ba);
		buffer.open(QIODevice::WriteOnly);
		image.save(&buffer, "JPG");
	}
	QImage test;
	{
		QBuffer buffer(&ba);
		buffer.open(QIODevice::ReadOnly);
		test.load(&buffer, "JPG");
	}
	if (test.width() != image.width()) {
		fprintf(stderr, "Sanity check failed: your Qt installation is broken (test width %d != image width %d) You can try to fix it by export QT_PLUGIN_PATH=<path_to_qt_plugins>\n", test.width(), image.width());
		exit(1);
	}
}

void cpp_household_init()
{
	sanity_checks();

	using namespace boost::python;

	class_<Pose>("Pose")
	.def("set_xyz", &Pose::set_xyz)
	.def("move_xyz", &Pose::move_xyz)
	.def("set_rpy", &Pose::set_rpy)
	.def("set_quaternion", &Pose::set_quaternion)
	.def("rpy", &Pose::rpy)
	.def("xyz", &Pose::xyz)
	.def("quatertion", &Pose::quatertion)
	.def("rotate_z", &Pose::rotate_z)
	.def("dot", &Pose::dot)
	;

	class_<Thingy>("Thingy", no_init)
	.def("pose", &Thingy::pose)
	.def("speed", &Thingy::speed)
	//.def("angular_speed", &Thingy::angular_speed)
	//.def("push", &Thingy::push)
	//.def("turn", &Thingy::turn)
	.add_property("name", &Thingy::get_name, &Thingy::set_name)
	.add_property("visibility_123", &Thingy::get_visibility_123, &Thingy::set_visibility_123)
	//.add_property("highest_point", &Thingy::highest_point)
	.def("contact_list", &Thingy::contact_list)
	.def("__hash__", &Thingy::__hash__)
	.def("__eq__", &Thingy::__eq__)
	.def("set_multiply_color", &Thingy::set_multiply_color)
	//.def("replace_texture", &Thingy::replace_texture)
	.def("assign_metaclass", &Thingy::assign_metaclass)  // assigns to class, not to instance
	;

	class_<Camera>("Camera", no_init)
	.add_property("name", &Camera::name)
	.add_property("resolution", &Camera::resolution)
	.def("render", &Camera::render)
	.def("test_window", &Camera::test_window)
	.def("test_window_score", &Camera::test_window_score)
	.def("set_key_callback", &Camera::set_key_callback)
	.def("set_hfov", &Camera::set_hfov)
	.def("set_near", &Camera::set_near)
	.def("set_far", &Camera::set_far)
	.def("set_pose", &Camera::set_pose)
	.def("move_and_look_at", &Camera::move_and_look_at)  // same as set_pose(), only sets camera position and orientation
	;

	class_<Joint>("Joint", no_init)
	.add_property("name", &Joint::name)
	.add_property("type", &Joint::type)
	.def("set_servo_target", &Joint::set_servo_target)
	.def("set_target_speed", &Joint::set_target_speed)
	.def("set_motor_torque", &Joint::set_motor_torque)
	.def("current_position", &Joint::current_position)
	.def("current_relative_position", &Joint::current_relative_position)
	.def("reset_current_position", &Joint::reset_current_position)
	//.def("reset_current_relative_position", &Joint::reset_current_relative_position)
	.def("limits", &Joint::limits)
	;

	class_<Robot>("Robot", no_init)
	//.add_property("cameras", &Robot::cameras)
	.add_property("joints", &Robot::joints)
	.add_property("parts", &Robot::parts)
	.add_property("root_part", &Robot::root_part)
	.def("set_pose", &Robot::set_pose)
	.def("set_pose_and_speed", &Robot::set_pose_and_speed)
	.def("query_position", &Robot::query_position)
	//.def("replace_texture", &Robot::replace_texture)
	;

	class_<World>("World", init<float,float>())
	.def("clean_everything", &World::clean_everything)
	.def("load_urdf", &World::load_urdf)
	.def("load_sdf", &World::load_sdf)
	.def("load_mjcf", &World::load_mjcf)
	.def("load_thingy", &World::load_thingy)
	.def("new_camera_free_float", &World::new_camera_free_float)
	.def("step", &World::step)
	.add_property("ts", &World::ts)
	.def("test_window", &World::test_window)
	.def("test_window_print", &World::test_window_print)
	.def("test_window_billboard", &World::test_window_billboard)
	.def("test_window_big_caption", &World::test_window_big_caption)
	.def("test_window_observations", &World::test_window_observations)
	.def("test_window_rewards", &World::test_window_rewards)
	.def("test_window_actions", &World::test_window_actions)
	.def("test_window_score", &World::test_window_score)
	.def("test_window_history_advance", &World::test_window_history_advance)
	.def("test_window_history_reset", &World::test_window_history_reset)
	.def("set_key_callback", &World::set_key_callback)
	//.def("tool_wall", &World::tool_wall)
	//.def("tool_quad_prism", &World::tool_quad_prism)
	.def("debug_rect", &World::debug_rect)
	.def("debug_line", &World::debug_line)
	.def("debug_sphere", &World::debug_sphere)
	.def("set_glsl_path", &World::set_glsl_path)
	;

	scope().attr("tip_z") = tip_z;
	scope().attr("tip_y") = tip_y;
	scope().attr("COLLISION_MARGIN") = Household::COLLISION_MARGIN/SCALE;
	scope().attr("METACLASS_FLOOR")    = (int) Household::METACLASS_FLOOR;
	scope().attr("METACLASS_WALL")     = (int) Household::METACLASS_WALL;
	scope().attr("METACLASS_MYSELF")   = (int) Household::METACLASS_MYSELF;
	scope().attr("METACLASS_FURNITURE")= (int) Household::METACLASS_FURNITURE;
	scope().attr("METACLASS_ITEM")     = (int) Household::METACLASS_ITEM;
	scope().attr("METACLASS_HANDLE")   = (int) Household::METACLASS_HANDLE;
}

BOOST_PYTHON_MODULE(cpp_household)
{
	cpp_household_init();
}

BOOST_PYTHON_MODULE(cpp_household_d)
{
	cpp_household_init();
}
