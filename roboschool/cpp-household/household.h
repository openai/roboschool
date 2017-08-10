#pragma once
#include "assets.h"
#include <bullet/PhysicsClientC_API.h>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace Household {

extern btScalar SCALE;
extern btScalar COLLISION_MARGIN;

using boost::shared_ptr;
using boost::weak_ptr;
struct World;


enum {
	METACLASS_FLOOR     = 0x01,
	METACLASS_WALL      = 0x02,
	METACLASS_MYSELF    = 0x04,
	METACLASS_FURNITURE = 0x08,
	METACLASS_HANDLE    = 0x10,
	METACLASS_ITEM      = 0x20,
};

struct ThingyClass {
	std::string class_name;
	uint8_t metaclass = 0;
	bool frozen = false;

	// optional structures for simple render
	shared_ptr<ShapeDetailLevels> shapedet_visual;
	shared_ptr<ShapeDetailLevels> shapedet_collision;

	shared_ptr<ThingyClass> modified_from_class; // it's there for static_mesh
};

struct Thingy {
	shared_ptr<ThingyClass> klass;

	std::string name;
	int visibility_123 = 0;
	//btScalar highest_point = 0; // for initial setup, to put other thingy on top

	Thingy()  { bullet_position.setIdentity(); bullet_link_position.setIdentity(); bullet_local_inertial_frame.setIdentity(); }
	~Thingy()  { remove_from_bullet(); }
	void remove_from_bullet();
	bool is_sleeping()  { return false; }

	void set_multiply_color(const std::string& tex, uint32_t* color, std::string* replace_texture);

	bool bullet_ignore = false;
	int bullet_handle = -1;
	int bullet_link_n = -1;
	btTransform bullet_position;
	btTransform bullet_link_position;
	btTransform bullet_local_inertial_frame;
	bool in_drawlist = false;
	btVector3   bullet_speed;
	btVector3   bullet_angular_speed;
	bool bullet_queried_at_least_once = false;
	//std::list<shared_ptr<Thingy>> subobjects_keepalive; // subobjects shouldn't be destroyed before parent (right order is parent first), nonempty only in robots with joints
};

struct Joint {
	weak_ptr<struct Robot> robot;
	weak_ptr<struct World> wref;

	std::string joint_name;
	int bullet_joint_n = -1;
	int bullet_qindex = -1;
	int bullet_uindex = -1;
	enum { ROTATIONAL_MOTOR, LINEAR_MOTOR };
	int joint_type = ROTATIONAL_MOTOR;
	bool joint_has_limits = false;
	float joint_limit1 = -1.0;   // if limit1 > limit2 rotation is infinite
	float joint_limit2 = -2.0;
	float joint_max_force = 1.0;
	float joint_max_velocity = 1.0;

	float joint_current_position = 0;
	float joint_current_speed = 0;

	bool first_torque_call = true;
	bool torque_need_repeat = false;
	float torque_repeat_val = 0;

	void joint_current_relative_position(float* pos, float* speed);
	void reset_current_position(float pos, float vel);

	void set_motor_torque(float torque);

	void set_target_speed(float target_speed, float kd, float maxforce);
	void set_relative_target_speed(float target_speed, float kp);

	void set_servo_target(float target_pos, float kp, float kd, float maxforce);
	void set_relative_servo_target(float target_pos, float kp, float kd);

	void activate();
};

struct Camera {
	std::string camera_name;
	std::string score;
	weak_ptr<Thingy> camera_attached_to;
	btTransform camera_pose; // used if camera_attached_to==0

	int camera_res_w = 192;
	int camera_res_h = 128;
	int camera_aux_w = 96;
	int camera_aux_h = 64;
	float camera_hfov  = 90; // horizontal fov
	float camera_near  = 0.1;
	float camera_far   = 100;
	float camera_fps   = 60;
	std::string camera_rgb;
	std::string camera_depth;
	std::string camera_depth_mask;
	std::string camera_labeling;
	std::string camera_labeling_mask;

	shared_ptr<SimpleRender::ContextViewport> viewport;
	void camera_render(const shared_ptr<SimpleRender::Context>& cx, bool render_depth, bool render_labeling, bool print_timing);

	Camera()  { camera_pose.setIdentity(); }
};

struct Robot {
	shared_ptr<Thingy> root_part;
	int bullet_handle;
	std::string original_urdf_name;
	std::vector<shared_ptr<Thingy>> robot_parts;
	std::vector<shared_ptr<Joint>> joints;
	std::vector<shared_ptr<Camera>> cameras;
	//void replace_texture(const std::string& material_name, const std::string& new_jpeg_png);
};

struct World: boost::enable_shared_from_this<World> {
	b3PhysicsClientHandle client;
	boost::shared_ptr<App> app_ref; // Keep application alive, while some worlds exist. If no worlds exist then new world gets created, probably will crash :(
	~World();

	float settings_gravity          = 0;
	float settings_timestep         = 0;
	float settings_timestep_sent    = 0;
	float settings_skip_frames_sent = 0;
	void settings_apply();

	//std::set<weak_ptr<ThingyClass>> classes;
	std::map<std::string, weak_ptr<ThingyClass>> klass_cache;
	shared_ptr<ThingyClass> klass_cache_find_or_create(const std::string& kname);
	void klass_cache_clear();

	std::vector<weak_ptr<Robot>> robotlist;
	std::map<int, weak_ptr<Robot>> bullet_handle_to_robot;

	std::vector<weak_ptr<Thingy>> drawlist;
	void thingy_add_to_drawlist(const shared_ptr<Thingy>& t);
	double ts = 0;

	shared_ptr<SimpleRender::Context> cx;

	void bullet_init(float gravity, float timestep);
	void bullet_step(int skip_frames);
	void clean_everything();
	void query_positions();
	void query_body_position(const shared_ptr<Robot>& robot);

	std::list<shared_ptr<Household::Thingy>> bullet_contact_list(const shared_ptr<Thingy>& t);
	double performance_bullet_ms;

	shared_ptr<Thingy> load_thingy(const std::string& the_filename, const btTransform& tr, float scale, float mass, uint32_t color, bool decoration_only);
	shared_ptr<Robot> load_urdf(const std::string& fn, const btTransform& tr, bool fixed_base, bool self_collision);
	std::list<shared_ptr<Robot>> load_sdf_mjcf(const std::string& fn, bool mjcf);
	void load_robot_shapes(const shared_ptr<Robot>& robot);
	void load_robot_joints(const shared_ptr<Robot>& robot, const std::string& origin_fn);

	void robot_move(const shared_ptr<Robot>& robot, const btTransform& tr, const btVector3& speed);

	shared_ptr<Thingy> debug_rect(btScalar x1, btScalar y1, btScalar x2, btScalar y2, btScalar h, uint32_t color);
	shared_ptr<Thingy> debug_line(btScalar x1, btScalar y1, btScalar z1, btScalar x2, btScalar y2, btScalar z2, uint32_t color);
	shared_ptr<Thingy> debug_sphere(btScalar x, btScalar y, btScalar z, btScalar rad, uint32_t color);

//	shared_ptr<MaterialNamespace> textures_cache;
//	shared_ptr<Material> texture_from_file_cached(const std::string& tex_fn);

//	shared_ptr<Thingy> tool_wall(
//		btScalar grid_size, float tex1, float tex_v_zero,
//		const std::vector<btScalar>& path,
//		bool closed,
//		const std::vector<btScalar>& low,
//		const std::vector<btScalar>& high,
//		const std::string& side_tex, float side_tex_scale,
//		const std::string& top_tex,  float top_tex_scale,
//		const std::string& bottom_tex, float bott_tex_scale,
//		const std::string& but_tex,  float butt_tex_scale);
//	shared_ptr<Thingy> tool_quad_prism(
//		float tex1, float tex_v_zero,
//		std::vector<btScalar>& path,
//		btScalar low,
//		btScalar high,
//		const std::string& side_tex_,
//		const std::string& top_tex_,
//		const std::string& bottom_tex_);
};

} // namespace Household
