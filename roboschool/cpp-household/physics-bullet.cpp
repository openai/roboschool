#include "render-glwidget.h"
#include <QtWidgets/QApplication>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>

namespace Household {

#ifdef CHROME_TRACING
static int chrome_trace_log = -1;
#endif

void World::bullet_init(float gravity, float timestep)
{
	char* fake_argv[] = { 0 };
	//client = b3CreateInProcessPhysicsServerAndConnectMainThread(0, fake_argv);
	//client = b3CreateInProcessPhysicsServerAndConnect(0, fake_argv);
	client = b3ConnectPhysicsDirect();
	settings_gravity = gravity;
	settings_timestep = timestep;
	settings_apply();

#ifdef CHROME_TRACING
	b3SharedMemoryCommandHandle command;
	command = b3StateLoggingCommandInit(client);
	b3StateLoggingStart(command, STATE_LOGGING_PROFILE_TIMINGS, "chrome_tracing.json");
	b3SharedMemoryStatusHandle status = b3SubmitClientCommandAndWaitStatus(client, command);
	int status_type = b3GetStatusType(status);
	if (status_type == CMD_STATE_LOGGING_START_COMPLETED) {
		chrome_trace_log = b3GetStatusLoggingUniqueId(status);
		fprintf(stderr, "Start chrome trace log, handle %i\n", chrome_trace_log);
	}
#endif
}

World::~World()
{
#ifdef CHROME_TRACING
	if (chrome_trace_log!=-1) {
		fprintf(stderr, "Stop chrome trace log, handle %i\n", chrome_trace_log);
		b3SharedMemoryCommandHandle command = b3StateLoggingCommandInit(client);
		b3StateLoggingStop(command, chrome_trace_log);
		b3SubmitClientCommandAndWaitStatus(client, command);
		chrome_trace_log = -1;
	}
#endif
	b3DisconnectSharedMemory(client);
}

void World::settings_apply()
{
}

void World::clean_everything()
{
	b3SubmitClientCommandAndWaitStatus(client, b3InitResetSimulationCommand(client));
	for (const boost::weak_ptr<Robot>& r: robotlist) {
		boost::shared_ptr<Robot> robot = r.lock();
		if (!robot) continue;
		robot->bullet_handle = -1;
	}
	robotlist.clear();
	drawlist.clear();
	bullet_handle_to_robot.clear();
	ts = 0;
	settings_timestep_sent = 0;
	settings_apply();
	// klass_cache -- leave it alone, it contains visual shapes useful for quick restart, klass_cache_clear() if you need reload
}

shared_ptr<Robot> World::load_urdf(const std::string& fn, const btTransform& tr, bool fixed_base, bool self_collision)
{
	shared_ptr<Robot> robot(new Robot);
	robot->original_urdf_name = fn;
	int statusType;
	b3SharedMemoryCommandHandle command = b3LoadUrdfCommandInit(client, fn.c_str());
	b3LoadUrdfCommandSetStartPosition(command, tr.getOrigin()[0], tr.getOrigin()[1], tr.getOrigin()[2]);
	b3LoadUrdfCommandSetStartOrientation(command, tr.getRotation()[0], tr.getRotation()[1], tr.getRotation()[2], tr.getRotation()[3]);
	b3LoadUrdfCommandSetUseFixedBase(command, fixed_base);
	if (self_collision)
		b3LoadUrdfCommandSetFlags(command, URDF_USE_SELF_COLLISION|URDF_USE_SELF_COLLISION_EXCLUDE_ALL_PARENTS);
	b3SharedMemoryStatusHandle statusHandle = b3SubmitClientCommandAndWaitStatus(client, command);
	statusType = b3GetStatusType(statusHandle);
	if (statusType != CMD_URDF_LOADING_COMPLETED) {
		fprintf(stderr, "Cannot load URDF file '%s'.\n", fn.c_str());
		return robot;
	}
	robot->bullet_handle = b3GetStatusBodyIndex(statusHandle);
	load_robot_joints(robot, fn);
	load_robot_shapes(robot);
	robotlist.push_back(robot);
	bullet_handle_to_robot[robot->bullet_handle] = robot;
	return robot;
}

std::list<shared_ptr<Robot>> World::load_sdf_mjcf(const std::string& fn, bool mjcf)
{
	std::list<shared_ptr<Robot>> ret;
	const int MAX_SDF_BODIES = 512;
	int bodyIndicesOut[MAX_SDF_BODIES];
	int N;
	if (mjcf) {
		b3SharedMemoryCommandHandle command = b3LoadMJCFCommandInit(client, fn.c_str());
		b3LoadMJCFCommandSetFlags(command, URDF_USE_SELF_COLLISION|URDF_USE_SELF_COLLISION_EXCLUDE_ALL_PARENTS);
		b3SharedMemoryStatusHandle status = b3SubmitClientCommandAndWaitStatus(client, command);
		if (b3GetStatusType(status) != CMD_MJCF_LOADING_COMPLETED) {
			fprintf(stderr, "'%s': cannot load MJCF.\n", fn.c_str());
			return ret;
		}
		N = b3GetStatusBodyIndices(status, bodyIndicesOut, MAX_SDF_BODIES);
	} else {
		b3SharedMemoryCommandHandle command = b3LoadSdfCommandInit(client, fn.c_str());
		b3SharedMemoryStatusHandle status = b3SubmitClientCommandAndWaitStatus(client, command);
		if (b3GetStatusType(status) != CMD_SDF_LOADING_COMPLETED) {
			fprintf(stderr, "'%s': cannot load SDF.\n", fn.c_str());
			return ret;
		}
		N = b3GetStatusBodyIndices(status, bodyIndicesOut, MAX_SDF_BODIES);
	}
	if (N > MAX_SDF_BODIES)
		fprintf(stderr, "'%s': too many bodies (%i).\n", fn.c_str(), N);
	for (int c=0; c<N; c++) {
		shared_ptr<Robot> robot(new Robot);
		robot->bullet_handle = bodyIndicesOut[c];
		load_robot_joints(robot, fn);
		load_robot_shapes(robot);
		robotlist.push_back(robot);
		bullet_handle_to_robot[robot->bullet_handle] = robot;
		ret.push_back(robot);
	}
	return ret;
}

static
void load_shape_into_class(
	const shared_ptr<ThingyClass>& klass,
	int geom, const std::string& fn, float ex, float ey, float ez, uint32_t color,
	const btTransform& viz_frame)
{
	shared_ptr<ShapeDetailLevels> save_here = klass->shapedet_visual;

	bool center_of_mass_sphere = false;
	if (center_of_mass_sphere && save_here->detail_levels[DETAIL_BEST].empty()) {
		shared_ptr<Shape> com(new Shape);
		com->primitive_type = Shape::SPHERE;
		com->sphere.reset(new Sphere({ 0.1f*SCALE }));
		com->material.reset(new Material("center_of_mass_sphere"));
		com->material->diffuse_color = 0x40FF0000;
		save_here->detail_levels[DETAIL_BEST].push_back(com);
		if (!save_here->materials)
			save_here->materials.reset(new MaterialNamespace);
		save_here->materials->name2mtl["center_of_mass_sphere"] = com->material;
	}

	shared_ptr<Shape> primitive(new Shape);
	if (geom==5) { // URDF_GEOM_MESH
		save_here->load_later_on = true;
		save_here->load_later_fn = fn;
		save_here->load_later_transform = viz_frame;
		primitive.reset();
	} else if (geom==2) { // URDF_GEOM_SPHERE
		primitive->primitive_type = Shape::SPHERE;
		primitive->sphere.reset(new Sphere({ ex*SCALE }));
	} else if (geom==3) { // URDF_GEOM_BOX
		primitive->primitive_type = Shape::BOX;
		primitive->box.reset(new Box({ ex*SCALE, ey*SCALE, ez*SCALE }));
	} else if (geom==4 || geom==7) { // URDF_GEOM_CYLINDER URDF_GEOM_CAPSULE
		primitive->primitive_type = geom==7 ? Shape::CAPSULE : Shape::CYLINDER;
		primitive->cylinder.reset(new Cylinder({ ey*SCALE, ex*SCALE })); // rad, length
	} else {
		// URDF_GEOM_PLANE ignore
		primitive.reset();
	}
	if (primitive) {
		primitive->origin = viz_frame;
		char buf[20];
		snprintf(buf, sizeof(buf), "#%08x", color);
		primitive->material.reset(new Material(buf));
		primitive->material->diffuse_color = color;
		if (!save_here->materials)
			save_here->materials.reset(new MaterialNamespace);
		save_here->materials->name2mtl[buf] = primitive->material;
		save_here->detail_levels[DETAIL_BEST].push_back(primitive);
	}
}

shared_ptr<Thingy> World::load_thingy(const std::string& the_filename, const btTransform& tr, float scale, float mass, uint32_t color, bool decoration_only)
{
	shared_ptr<ThingyClass> klass = klass_cache_find_or_create(the_filename);
	shared_ptr<Thingy> t(new Thingy());
	t->klass = klass;
	t->bullet_ignore = true;
	t->bullet_position = tr;
	t->name = the_filename;
	thingy_add_to_drawlist(t);
	if (t->klass->frozen) return t;
	//mass;
	//decoration_only;
	btTransform ident;
	ident.setIdentity();
	load_shape_into_class(klass, 5, the_filename, scale, scale, scale, color, ident);
	if (cx) cx->need_load_missing_textures = true;
	t->klass->frozen = true; // only one shape in thingy, will load quickly next time
	return t;
}

void World::load_robot_joints(const shared_ptr<Robot>& robot, const std::string& original_fn)
{
	b3BodyInfo root;
	b3GetBodyInfo(client, robot->bullet_handle, &root);
	robot->root_part.reset(new Thingy);
	robot->root_part->name = root.m_baseName;
	robot->original_urdf_name = original_fn + ":" + root.m_baseName;

	int cnt = b3GetNumJoints(client, robot->bullet_handle);
	robot->joints.resize(cnt);
	robot->robot_parts.resize(cnt);
	for (int c=0; c<cnt; c++) {
		struct b3JointInfo info;
		b3GetJointInfo(client, robot->bullet_handle, c, &info);
		//enum JointType {
		//eRevoluteType = 0,
		//ePrismaticType = 1,
		//eSphericalType = 2,
		//ePlanarType = 3,
		//eFixedType = 4,
		//ePoint2PointType = 5,
		if (info.m_jointType==eRevoluteType || info.m_jointType==ePrismaticType) {
			shared_ptr<Joint>& j = robot->joints[c];
			j.reset(new Joint);
			j->wref = shared_from_this();
			j->robot = robot;
			j->joint_name = info.m_jointName;
			j->joint_type = info.m_jointType==eRevoluteType ? Joint::ROTATIONAL_MOTOR : Joint::LINEAR_MOTOR;
			j->bullet_qindex = info.m_qIndex;
			j->bullet_uindex = info.m_uIndex;
			j->bullet_joint_n = c;
			j->joint_has_limits = info.m_jointLowerLimit < info.m_jointUpperLimit;
			j->joint_limit1 = info.m_jointLowerLimit;
			j->joint_limit2 = info.m_jointUpperLimit;
			j->joint_max_force = info.m_jointMaxForce;
			j->joint_max_velocity = info.m_jointMaxVelocity;
		}

		shared_ptr<Thingy> part = robot->robot_parts[c];
		part.reset(new Thingy);
		part->bullet_handle = robot->bullet_handle;
		part->bullet_link_n = c;
		part->name = info.m_linkName;
		robot->robot_parts[c] = part;
	}
}

void World::klass_cache_clear()
{
	klass_cache.clear();
}

shared_ptr<ThingyClass> World::klass_cache_find_or_create(const std::string& class_name)
{
	auto f = klass_cache.find(class_name);
	if (f!=klass_cache.end()) {
		shared_ptr<ThingyClass> have_one = f->second.lock();
		if (have_one) return have_one;
	}
	shared_ptr<ThingyClass> k(new ThingyClass);
	k->class_name = class_name;
	k->shapedet_visual.reset(new ShapeDetailLevels);
	klass_cache[class_name] = k;
	return k;
}

void World::load_robot_shapes(const shared_ptr<Robot>& robot)
{
	b3SharedMemoryCommandHandle commandHandle = b3InitRequestVisualShapeInformation(client, robot->bullet_handle);
	b3SharedMemoryStatusHandle statusHandle = b3SubmitClientCommandAndWaitStatus(client, commandHandle);
	int statusType = b3GetStatusType(statusHandle);
	if (statusType != CMD_VISUAL_SHAPE_INFO_COMPLETED) return;

	b3VisualShapeInformation visualShapeInfo;
	b3GetVisualShapeInformation(client, &visualShapeInfo);
	for (int i=0; i<visualShapeInfo.m_numVisualShapes; i++) {
		int link_n = visualShapeInfo.m_visualShapeData[i].m_linkIndex;
		if (link_n < -1 || link_n > (int)robot->robot_parts.size()) {
			int set_breakpoint_here = 5;
			assert(0);
		}
		shared_ptr<Thingy> part;
		if (link_n==-1) {
			part = robot->root_part;
		} else {
			part = robot->robot_parts[link_n];
		}
		std::string fn = visualShapeInfo.m_visualShapeData[i].m_meshAssetFileName;
		int geom = visualShapeInfo.m_visualShapeData[i].m_visualGeometryType;

		uint32_t color =
			(uint32_t(255*visualShapeInfo.m_visualShapeData[i].m_rgbaColor[0]) << 16) |
			(uint32_t(255*visualShapeInfo.m_visualShapeData[i].m_rgbaColor[1]) << 8) |
			(uint32_t(255*visualShapeInfo.m_visualShapeData[i].m_rgbaColor[2]) << 0) |
			(uint32_t(255*visualShapeInfo.m_visualShapeData[i].m_rgbaColor[3]) << 24);

		if (!part->klass) {
			char klass_name[1024];
			snprintf(klass_name, sizeof(klass_name), "%s:%03i", robot->original_urdf_name.c_str(), link_n);
			//fprintf(stderr, "allocating class=='%s' link==%i geom=%i fn=='%s'\n", klass_name, link_n, geom, fn.c_str());
			part->klass = klass_cache_find_or_create(klass_name);
		}

		btVector3 pos;
		btQuaternion quat;
		{
			pos[0] = visualShapeInfo.m_visualShapeData[i].m_localVisualFrame[0];
			pos[1] = visualShapeInfo.m_visualShapeData[i].m_localVisualFrame[1];
			pos[2] = visualShapeInfo.m_visualShapeData[i].m_localVisualFrame[2];
			quat[0] = visualShapeInfo.m_visualShapeData[i].m_localVisualFrame[3];
			quat[1] = visualShapeInfo.m_visualShapeData[i].m_localVisualFrame[4];
			quat[2] = visualShapeInfo.m_visualShapeData[i].m_localVisualFrame[5];
			quat[3] = visualShapeInfo.m_visualShapeData[i].m_localVisualFrame[6];
		}

		if (!part->klass->frozen) {
			//fprintf(stderr, "adding visual to link==%i geom=%i fn=='%s', already have %i viz shapes\n", link_n, geom, fn.c_str(),
			//	(int)part->klass->shapedet_visual->detail_levels[DETAIL_BEST].size());
			load_shape_into_class(part->klass, geom, fn,
				visualShapeInfo.m_visualShapeData[i].m_dimensions[0],
				visualShapeInfo.m_visualShapeData[i].m_dimensions[1],
				visualShapeInfo.m_visualShapeData[i].m_dimensions[2],
				color,
				btTransform(quat, pos)
				);
		}
		if (cx) cx->need_load_missing_textures = true;

		part->bullet_link_n = link_n;
		thingy_add_to_drawlist(part);
	}

	for (const shared_ptr<Thingy>& part: robot->robot_parts)
		if (part->klass)
			part->klass->frozen = true;
	if (robot->root_part->klass)
		robot->root_part->klass->frozen = true;
}

void World::thingy_add_to_drawlist(const shared_ptr<Thingy>& t)
{
	if (!t->in_drawlist) {
		t->in_drawlist = true;
		drawlist.push_back(t);
	}
}

void Thingy::remove_from_bullet()
{
	int breakpoint_here = 5;
}

void World::bullet_step(int skip_frames)
{
	QElapsedTimer elapsed;
	elapsed.start();

	float need_timestep = settings_timestep*skip_frames;
	if (
		settings_timestep_sent != need_timestep ||
		settings_skip_frames_sent != skip_frames
	) {
		b3SharedMemoryCommandHandle command = b3InitPhysicsParamCommand(client);
		b3PhysicsParamSetGravity(command, 0, 0, -settings_gravity);
		b3PhysicsParamSetNumSolverIterations(command, 5);
		b3PhysicsParamSetDefaultContactERP(command, 0.9);
		b3PhysicsParamSetTimeStep(command, need_timestep);
		settings_timestep_sent = need_timestep;
		b3PhysicsParamSetNumSubSteps(command, skip_frames);
		settings_skip_frames_sent = skip_frames;
		b3SubmitClientCommandAndWaitStatus(client, command);
	}

	for (const boost::weak_ptr<Robot>& wr: robotlist) {
		boost::shared_ptr<Robot> robot = wr.lock();
		if (!robot) continue;
		b3SharedMemoryCommandHandle cmd = 0;
		for (const shared_ptr<Joint>& j: robot->joints) {
			if (!j) continue;
			if (j->torque_need_repeat) {
				if (!cmd) cmd = b3JointControlCommandInit2(client, robot->bullet_handle, CONTROL_MODE_TORQUE);
				b3JointControlSetDesiredForceTorque(cmd, j->bullet_uindex, j->torque_repeat_val);
			}
		}
		if (cmd) b3SubmitClientCommandAndWaitStatus(client, cmd);
	}

	double ms_post_joints = elapsed.nsecsElapsed() / 1000000.0;
	elapsed.start();

	b3SharedMemoryCommandHandle cmd = b3InitStepSimulationCommand(client);
	b3SubmitClientCommandAndWaitStatus(client, cmd);

	ts += settings_timestep*skip_frames;

	double ms_step = elapsed.nsecsElapsed() / 1000000.0;
	elapsed.start();
	query_positions();
	double ms_query = elapsed.nsecsElapsed() / 1000000.0;

	static double mean;
	mean *= 0.95;
	mean += 0.05*ms_step;
	//fprintf(stderr, "j=%0.2lf, step=%0.2lf (mean=%0.2f), query=%0.2lf\n", ms_post_joints, ms_step, mean, ms_query);

	performance_bullet_ms = ms_post_joints + ms_step + ms_query;
}

void World::query_positions()
{
	for (const boost::weak_ptr<Robot>& r: robotlist) {
		boost::shared_ptr<Robot> robot = r.lock();
		if (!robot) continue;
		query_body_position(robot);
	}
}

static
btTransform transform_from_doubles(const double* pos, const double* quat)
{
	btVector3 p;
	btQuaternion q;
	{
		p[0] = pos[0];
		p[1] = pos[1];
		p[2] = pos[2];
		q[0] = quat[0];
		q[1] = quat[1];
		q[2] = quat[2];
		q[3] = quat[3];
	}
	return btTransform(q, p);
}

void World::query_body_position(const shared_ptr<Robot>& robot)
{
	if (!robot->root_part) return;

	b3SharedMemoryCommandHandle cmd_handle = b3RequestActualStateCommandInit(client, robot->bullet_handle);
	b3RequestActualStateCommandComputeLinkVelocity(cmd_handle, ACTUAL_STATE_COMPUTE_LINKVELOCITY);
	b3SharedMemoryStatusHandle status_handle = b3SubmitClientCommandAndWaitStatus(client, cmd_handle);

	const double* root_inertial_frame;
	const double* q;
	const double* q_dot;
	b3GetStatusActualState(
		status_handle, 0 /* body_unique_id */,
		0 /* num_degree_of_freedom_q */, 0 /* num_degree_of_freedom_u */,
		&root_inertial_frame /*root_local_inertial_frame*/, &q,
		&q_dot, 0 /* joint_reaction_forces */);
	//printf("%f ", jointReactionForces[i]);
	assert(robot->root_part->bullet_link_n==-1);
	robot->root_part->bullet_position = transform_from_doubles(q, q+3);
	robot->root_part->bullet_speed[0] = q_dot[0];
	robot->root_part->bullet_speed[1] = q_dot[1];
	robot->root_part->bullet_speed[2] = q_dot[2];
	robot->root_part->bullet_angular_speed[0] = q_dot[3];
	robot->root_part->bullet_angular_speed[1] = q_dot[4];
	robot->root_part->bullet_angular_speed[2] = q_dot[5];
	robot->root_part->bullet_local_inertial_frame = transform_from_doubles(root_inertial_frame, root_inertial_frame+3);
	robot->root_part->bullet_link_position = transform_from_doubles(q, q+3);
	robot->root_part->bullet_queried_at_least_once = true;

	int status_type = b3GetStatusType(status_handle);
	if (status_type != CMD_ACTUAL_STATE_UPDATE_COMPLETED)
		return;

	for (const shared_ptr<Thingy>& part: robot->robot_parts) {
		struct b3LinkState linkstate;
		if (!part) continue;
		if (part->bullet_link_n==-1) continue;
		b3GetLinkState(client, status_handle, part->bullet_link_n, &linkstate);
		part->bullet_position = transform_from_doubles(linkstate.m_worldPosition, linkstate.m_worldOrientation);
		part->bullet_local_inertial_frame = transform_from_doubles(linkstate.m_localInertialPosition, linkstate.m_localInertialOrientation);
		part->bullet_link_position = transform_from_doubles(linkstate.m_worldLinkFramePosition, linkstate.m_worldLinkFrameOrientation);
		part->bullet_speed[0] = linkstate.m_worldLinearVelocity[0];
		part->bullet_speed[1] = linkstate.m_worldLinearVelocity[1];
		part->bullet_speed[2] = linkstate.m_worldLinearVelocity[2];
		part->bullet_angular_speed[0] = linkstate.m_worldAngularVelocity[0];
		part->bullet_angular_speed[1] = linkstate.m_worldAngularVelocity[1];
		part->bullet_angular_speed[2] = linkstate.m_worldAngularVelocity[2];
		part->bullet_queried_at_least_once = true;
	}

	for (const shared_ptr<Joint>& j: robot->joints) {
		if (!j) continue;
		j->joint_current_position = q[j->bullet_qindex];
		j->joint_current_speed = q_dot[j->bullet_uindex];
	}
}

void Joint::set_motor_torque(float torque)
{
	shared_ptr<Robot> r = robot.lock();
	shared_ptr<World> w = wref.lock();
	if (!r || !w) return;
	if (first_torque_call) {
		set_servo_target(0, 0.1, 0.1, 0);
		first_torque_call = false;
	}
	torque_need_repeat = true;
	torque_repeat_val = torque;
}

void Joint::set_target_speed(float target_speed, float kd, float maxforce)
{
	shared_ptr<Robot> r = robot.lock();
	shared_ptr<World> w = wref.lock();
	if (!r || !w) return;
	b3SharedMemoryCommandHandle cmd = b3JointControlCommandInit2(w->client, r->bullet_handle, CONTROL_MODE_VELOCITY);
	b3JointControlSetDesiredVelocity(cmd, bullet_uindex, target_speed);
	b3JointControlSetKd(cmd,              bullet_uindex, kd);
	b3JointControlSetMaximumForce(cmd,    bullet_uindex, maxforce);
	b3SubmitClientCommandAndWaitStatus(w->client, cmd);
	first_torque_call = true;
	torque_need_repeat = false;
}

void Joint::set_servo_target(float target_pos, float kp, float kd, float maxforce)
{
	shared_ptr<Robot> r = robot.lock();
	shared_ptr<World> w = wref.lock();
	if (!r || !w) return;
	b3SharedMemoryCommandHandle cmd = b3JointControlCommandInit2(w->client, r->bullet_handle, CONTROL_MODE_POSITION_VELOCITY_PD);
	b3JointControlSetDesiredPosition(cmd, bullet_qindex, target_pos);
	b3JointControlSetKp(cmd,              bullet_uindex, kp);
	b3JointControlSetKd(cmd,              bullet_uindex, kd);
	b3JointControlSetMaximumForce(cmd,    bullet_uindex, maxforce);
	b3SubmitClientCommandAndWaitStatus(w->client, cmd);
	first_torque_call = true;
	torque_need_repeat = false;
}

void Joint::set_relative_servo_target(float target_pos, float kp, float kd)
{
	float pos_mid = 0.5*(joint_limit1 + joint_limit2);
	set_servo_target( pos_mid + 0.5*target_pos*(joint_limit2 - joint_limit1),
		kp, kd,
		joint_max_force ? joint_max_force : 40); // 40 is about as strong as humanoid hands
}

void Joint::joint_current_relative_position(float* pos, float* speed)
{
	float rpos, rspeed;
	rpos = joint_current_position;
	rspeed = joint_current_speed;
	if (joint_has_limits) {
		float pos_mid = 0.5 * (joint_limit1 + joint_limit2);
		rpos = 2 * (rpos - pos_mid) / (joint_limit2 - joint_limit1);
	}
	if (joint_max_velocity > 0) {
		rspeed /= joint_max_velocity;
	} else {
		if (joint_type==Household::Joint::ROTATIONAL_MOTOR)
			rspeed *= 0.1; // normalize for 10 radian per second == 1  (6.3 radian/s is one rpm)
		else
			rspeed *= 0.5; // typical distance 1 meter, something fast travel it in 0.5 seconds (speed is 2)
	}
	*pos = rpos;
	*speed = rspeed;
}

void Joint::reset_current_position(float pos, float vel)
{
	shared_ptr<Robot> r = robot.lock();
	shared_ptr<World> w = wref.lock();
	if (!r || !w) return;
	b3SharedMemoryCommandHandle cmd = b3CreatePoseCommandInit(w->client, r->bullet_handle);
	b3CreatePoseCommandSetJointPosition(w->client, cmd, bullet_joint_n, pos);
	b3CreatePoseCommandSetJointVelocity(w->client, cmd, bullet_joint_n, vel);
	b3SubmitClientCommandAndWaitStatus(w->client, cmd);
}

void Joint::activate()
{
}

void World::robot_move(const shared_ptr<Robot>& robot, const btTransform& tr, const btVector3& speed)
{
	b3SharedMemoryCommandHandle cmd = b3CreatePoseCommandInit(client, robot->bullet_handle);
	b3CreatePoseCommandSetBasePosition(cmd, tr.getOrigin()[0], tr.getOrigin()[1], tr.getOrigin()[2]);
	b3CreatePoseCommandSetBaseOrientation(cmd, tr.getRotation()[0], tr.getRotation()[1], tr.getRotation()[2], tr.getRotation()[3]);
	double tmp[3];
	tmp[0] = speed[0];
	tmp[1] = speed[1];
	tmp[2] = speed[2];
	b3CreatePoseCommandSetBaseLinearVelocity(cmd, tmp);
	b3SubmitClientCommandAndWaitStatus(client, cmd);
}

std::list<shared_ptr<Household::Thingy>> World::bullet_contact_list(const shared_ptr<Thingy>& t)
{
	b3SharedMemoryCommandHandle cmd = b3InitRequestContactPointInformation(client);
	b3SetContactFilterBodyA(cmd, t->bullet_handle);
	b3SetContactFilterLinkA(cmd, t->bullet_link_n);
	b3SharedMemoryStatusHandle statusHandle = b3SubmitClientCommandAndWaitStatus(client, cmd);
	int statusType = b3GetStatusType(statusHandle);
	b3ContactInformation contacts;
	assert(statusType==CMD_CONTACT_POINT_INFORMATION_COMPLETED);
	b3GetContactPointInformation(client, &contacts);

	std::list<shared_ptr<Household::Thingy>> result;
	for (int c=0; c<contacts.m_numContactPoints; c++) {
		const b3ContactPointData& ct = contacts.m_contactPointData[c];
		int body_a, link_a;
		int body_b, link_b;
		body_a = ct.m_bodyUniqueIdA;
		link_a = ct.m_linkIndexA;
		body_b = ct.m_bodyUniqueIdB;
		link_b = ct.m_linkIndexB;
		//fprintf(stderr, "collision, asked for %02i:%02i, got %02i:%02i vs %02i:%02i\n",
		//	t->bullet_handle, t->bullet_link_n,
		//	ct.m_bodyUniqueIdA, ct.m_linkIndexA,
		//	ct.m_bodyUniqueIdB, ct.m_linkIndexB
		//	);
		assert(body_a==t->bullet_handle);
		assert(link_a==t->bullet_link_n);
		auto f = bullet_handle_to_robot.find(body_b);
		if (f==bullet_handle_to_robot.end()) {
			fprintf(stderr, "World::bullet_contact_list() contact with object that was not created via World interface.\n");
			continue;
		}
		shared_ptr<Robot> other_robot = f->second.lock();
		if (!other_robot) {
			fprintf(stderr, "World::bullet_contact_list() contact with object that is dead according to local bookkeeping.\n");
			continue;
		}
		shared_ptr<Thingy> other_part = link_b==-1 ? other_robot->root_part : other_robot->robot_parts[link_b];
		result.push_back(other_part);
	}
	return result;
}

}

