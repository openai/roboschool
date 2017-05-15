#include "render-glwidget.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QDoubleSpinBox>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QToolButton>
#include <QtCore/QDir>
#include <QtCore/QDateTime>

#include <bullet/PhysicsClientC_API.h>

using boost::shared_ptr;
using namespace Household;

//extern void robot_links_cache_clear();

namespace Household {
btScalar SCALE = 1.0;
//btScalar COLLISION_MARGIN = 0.02*SCALE;
}

#if 0
static
shared_ptr<Thingy> create_single_thingy_instance_from_shape(const shared_ptr<Household::Shape>& shape, const std::string& name, btTransform pos, uint32_t color, float mass=0.3)
{
	shape->material.reset(new Material(name));
	shape->material->diffuse_color = color;
	shared_ptr<ShapeDetailLevels> shapeset(new ShapeDetailLevels);
	shapeset->detail_levels[DETAIL_BEST].push_back(shape);
	shapeset->materials.reset(new MaterialNamespace);
	shapeset->materials->name2mtl[name] = shape->material;
	shared_ptr<ThingyClass> klass(new ThingyClass);
	klass->class_name = name;
	klass->shapedet_visual = shapeset;
	klass->shapedet_collision = shapeset;
	klass->mass = mass;
	klass->make_compound_from_collision_shapes();
	shared_ptr<Thingy> t(new Thingy(klass));
	t->self_collision_ptr = t.get();
	t->bullet_position = pos;
	return t;
}

static
std::list<shared_ptr<Thingy>> hammer_scene(const shared_ptr<World>& world)
{
	std::list<shared_ptr<Thingy>> keepref;

	shared_ptr<Shape> sphere(new Shape);
	sphere->primitive_type = Shape::SPHERE;
	sphere->sphere.reset(new Sphere({ 0.3f*SCALE }));

	shared_ptr<Shape> finger1(new Shape);
	finger1->primitive_type = Shape::BOX;
	finger1->box.reset(new Box({ 0.06f*SCALE, 0.04f*SCALE, 0.3f*SCALE }));

	shared_ptr<Shape> finger2(new Shape);
	finger2->primitive_type = Shape::BOX;
	finger2->box.reset(new Box({ 0.06f*SCALE, 0.04f*SCALE, 0.3f*SCALE }));

	const float GRIP_WIDTH = 0.1;
	const float EFFORT = 2.1;

	shared_ptr<Thingy> sp = create_single_thingy_instance_from_shape(sphere, "sphere",   btTransform(btQuaternion(0,0,0,1), btVector3(0,                0, 1.0f*SCALE             )), 0xFF0000, 0);
	shared_ptr<Thingy> f1 = create_single_thingy_instance_from_shape(finger1,"finger1",  btTransform(btQuaternion(0,0,0,1), btVector3(0, GRIP_WIDTH*SCALE,(1.0f-0.35f-0.15f)*SCALE)), 0x0000FF, 0.5);
	shared_ptr<Thingy> f2 = create_single_thingy_instance_from_shape(finger2,"finger2",  btTransform(btQuaternion(0,0,0,1), btVector3(0,-GRIP_WIDTH*SCALE,(1.0f-0.35f-0.15f)*SCALE)), 0x0000FF, 0.5);
	world->thingy_add(sp);
	world->thingy_add(f1);
	world->thingy_add(f2);
	keepref.push_back(sp);
	keepref.push_back(f1);
	keepref.push_back(f2);

	btVector3 attach_fingers_point(0, 0, (1.0f-0.35f)*SCALE);
	btVector3 sphere_local = sp->bullet_position.inverse() * attach_fingers_point;
	btVector3 f1_local = f1->bullet_position.inverse() * attach_fingers_point;
	btVector3 f2_local = f2->bullet_position.inverse() * attach_fingers_point;

	btQuaternion q(0,0,0,1);
	{
		shared_ptr<btGeneric6DofSpring2Constraint> dof(new btGeneric6DofSpring2Constraint(
			*f1->bullet_body, *sp->bullet_body, btTransform(q,f1_local), btTransform(q,sphere_local), RO_XYZ
			));
		dof->setAngularLowerLimit(btVector3(0,0,0));
		dof->setAngularUpperLimit(btVector3(0,0,0));
		dof->enableMotor(1, true);
		world->bullet_world->addConstraint(dof.get());
		dof->setTargetVelocity(1, -0.3*SCALE);
		dof->setMaxMotorForce(1, EFFORT*SCALE);
		dof->setLinearLowerLimit(btVector3(0,-GRIP_WIDTH*SCALE,0));
		dof->setLinearUpperLimit(btVector3(0,+GRIP_WIDTH*SCALE,0));
		f1->bullet_constraint = dof;
	}
	{
		shared_ptr<btGeneric6DofSpring2Constraint> dof(new btGeneric6DofSpring2Constraint(
			*f2->bullet_body, *sp->bullet_body, btTransform(q,f2_local), btTransform(q,sphere_local), RO_XYZ
			));
		dof->setAngularLowerLimit(btVector3(0,0,0));
		dof->setAngularUpperLimit(btVector3(0,0,0));
		dof->enableMotor(1, true);
		world->bullet_world->addConstraint(dof.get());
		dof->setTargetVelocity(1, 0.3*SCALE);
		dof->setMaxMotorForce(1, EFFORT*SCALE);
		dof->setLinearLowerLimit(btVector3(0,-GRIP_WIDTH*SCALE,0));
		dof->setLinearUpperLimit(btVector3(0,+GRIP_WIDTH*SCALE,0));
		f2->bullet_constraint = dof;
	}

	shared_ptr<Shape> cylinder(new Shape);
	cylinder->primitive_type = Shape::CYLINDER;
	cylinder->cylinder.reset(new Cylinder({ 0.03f*SCALE, 1.2f*SCALE }));

	shared_ptr<Thingy> pan = create_single_thingy_instance_from_shape(cylinder, "cylinder", btTransform(btQuaternion(0.707,0,0.707,0), btVector3(0.5f*SCALE, 0, (1.0f-0.35f-0.03f/2)*SCALE)), 0x00FF00, 0.2);
	world->thingy_add(pan);
	keepref.push_back(pan);

	return keepref;
}

static
std::list<shared_ptr<Thingy>> primitives_test(const shared_ptr<World>& world)
{
	std::list<shared_ptr<Thingy>> keepref;

	shared_ptr<Shape> sphere(new Shape);
	sphere->primitive_type = Shape::SPHERE;
	sphere->sphere.reset(new Sphere({ 0.05f*SCALE }));

	shared_ptr<Shape> cylinder(new Shape);
	cylinder->primitive_type = Shape::CYLINDER;
	cylinder->cylinder.reset(new Cylinder({ 0.09f*SCALE, 0.3f*SCALE }));

	shared_ptr<Shape> box(new Shape);
	box->primitive_type = Shape::BOX;
	box->box.reset(new Box({ 0.05f*SCALE, 0.05f*SCALE, 0.05f*SCALE }));

	{
		//shared_ptr<Thingy> m2 = create_single_thingy_instance_from_shape(cylinder, "cylinder", btTransform(btQuaternion(0,0,1,0), btVector3(0,1.3f*SCALE,2.6f*SCALE)), 0x00FF00);
		shared_ptr<Thingy> m1 = create_single_thingy_instance_from_shape(sphere,   "sphere",   btTransform(btQuaternion(0,0,0,1), btVector3(0,1.3f*SCALE,1.8f*SCALE)), 0xFF0000);
		shared_ptr<Thingy> m3 = create_single_thingy_instance_from_shape(box,      "box",      btTransform(btQuaternion(0,0,0.01,1), btVector3(0,1.9f*SCALE,0.6f*SCALE)), 0x0000FF);
		world->thingy_add(m1);
		//world->thingy_add(m2);
		world->thingy_add(m3);
		keepref.push_back(m1);
		//keepref.push_back(m2);
		keepref.push_back(m3);
	}

	return keepref;

	shared_ptr<Thingy> c1;
	shared_ptr<Thingy> c2;
	for (int i=0; i<30; i++) {
		c2 = create_single_thingy_instance_from_shape(cylinder, "cylinder", btTransform(btQuaternion(0,0,1,0), btVector3(1.0f*SCALE,4.0f*SCALE,i*0.4f*SCALE)), 0x00FF00);
		world->thingy_add(c2);
		keepref.push_back(c2);
		if (c1) {
			btVector3 mid12 = 0.5*(c1->bullet_position.getOrigin() + c2->bullet_position.getOrigin());
			btVector3 p1 = c1->bullet_position.inverse() * mid12;
			btVector3 p2 = c2->bullet_position.inverse() * mid12;
			shared_ptr<btPoint2PointConstraint> p2p(new btPoint2PointConstraint(*c1->bullet_body, *c2->bullet_body, p1, p2));
			world->bullet_world->addConstraint(p2p.get());
			c2->bullet_constraint = p2p;
		}
		c1 = c2;
	}
	return keepref;
}

static std::list<shared_ptr<Thingy>> keepref;
// TODO: move that away (still useful to test primitives)
#endif


static shared_ptr<World>  world;

static shared_ptr<Robot>  the_robot;
static shared_ptr<Thingy> the_model;
static std::string        the_filename;
static bool               the_model_along_y = false;

static time_t last_reload_time;

static
time_t mtime(const std::string& fn)
{
	QFileInfo fi(QString::fromUtf8(fn.c_str()));
	QDateTime dt = fi.lastModified();
	time_t tt = dt.toTime_t();
	if (tt==0) throw std::runtime_error("cannot stat '" + fn + "'");
	return tt;
}

static
void load_model_or_robot()
{
	world->klass_cache_clear();
	try {
		btQuaternion quat(0,0,0,1);
		if (the_model_along_y)
			quat.setEuler(0, M_PI/2, 0);
		time_t t = mtime(the_filename);
		if (last_reload_time==t) return;
		last_reload_time = t;
		//the_robot = world->load_urdf(the_filename, btTransform(quat, btVector3(0,0,0)), false);
		the_model = world->load_thingy(the_filename, btTransform(quat, btVector3(0,0,0)), 1.0, 1.0, 0xFF0000, false);
		//} else {
//			time_t t = mtime(the_model_filename);
//			if (last_reload_time==t) return;
//			last_reload_time = t;
//			the_model = world->load_thingy(the_model_filename, btTransform(quat, btVector3(0,0,0)), SCALE, 0.0, false, true);
//		}

	} catch (const std::exception& e) {
		fprintf(stderr, "ERROR: %s\n", e.what());
	}

	//the_robot.reset();
	//the_model.reset();

	//keepref = primitives_test(world);
	//keepref = hammer_scene(world);
}

class TestWindow: public QWidget {
	Q_OBJECT
public:
	Viz* viz = 0;
	shared_ptr<World> world;
	QGridLayout* grid;

	QCheckBox* model_along_y;
	QCheckBox* auto_rotate;
	QCheckBox* auto_switch;
	QCheckBox* draw_lines;
	QLabel* auto_switch_label;
	std::vector<QDoubleSpinBox*> joint_spins;
	std::vector<QLabel*> joint_labels;
	std::vector<QToolButton*> joint_button_sv;
	std::vector<QToolButton*> joint_button_ve;
	std::vector<QToolButton*> joint_button_tq;
	std::vector<int> joint_control_mode;

	float prev_zrot = 0;
	float prev_yrot = 0;
	float prev_wheel = 0;

	TestWindow(const shared_ptr<World>& world): QWidget(0), world(world)
	{
		grid = new QGridLayout();
		setLayout(grid);
		world->cx.reset(new SimpleRender::Context(world));
		viz = new Viz(world->cx);
		viz->wheel /= SCALE;
		viz->wheel *= 4; // bigger scale on start, compared to house
		viz->ruler_size = SCALE;

		grid->addWidget(viz, 0, 0, 2, 1);
		grid->setMargin(0);
		grid->setSpacing(0);

		QGridLayout* sidegrid = new QGridLayout;
		grid->addLayout(sidegrid, 0, 1);
		grid->setRowStretch(0, 0);
		grid->setRowStretch(1, 5);
		grid->setColumnStretch(0, 1);
		grid->setColumnMinimumWidth(1, 300);
		sidegrid->setMargin(5);
		sidegrid->setSpacing(0);
		sidegrid->setColumnStretch(0, 1);

		int y = 0;
		model_along_y = new QCheckBox("Model along &Y-axis");
		sidegrid->addWidget(model_along_y, y++, 0, 1, 4);
		auto_switch_label = new QLabel("Press F1 to switch mode");
		sidegrid->addWidget(auto_switch_label, y++, 0, 1, 4);
		auto_switch = new QCheckBox("Auto &switch mode");
		sidegrid->addWidget(auto_switch, y++, 0, 1, 4);
		auto_rotate = new QCheckBox("Auto &rotate");
		sidegrid->addWidget(auto_rotate, y++, 0, 1, 4);
		draw_lines  = new QCheckBox("&Lines (press `)");
		sidegrid->addWidget(draw_lines, y++, 0, 1, 4);

		QFont small_font = font();
		small_font.setPointSizeF(small_font.pointSizeF() / 1.5);

		int jcnt = the_robot ? the_robot->joints.size() : 0;
		joint_button_sv.resize(jcnt);
		joint_button_ve.resize(jcnt);
		joint_button_tq.resize(jcnt);
		joint_control_mode.resize(jcnt);
		joint_spins.resize(jcnt);
		joint_labels.resize(jcnt);
		if (the_robot)
		for (int c=0; c<jcnt; ++c) {
			shared_ptr<Joint> j = the_robot->joints[c];
			if (!j) continue;
			QDoubleSpinBox* spin = new QDoubleSpinBox();
			spin->setPrefix(QString("Joint %1 = ") . arg(c));
			spin->setRange(-1, 1);
			float pos, speed;
			j->joint_current_position(&pos, &speed);
			spin->setValue(pos);
			spin->setSingleStep(0.02);
			joint_spins[c] = spin;
			sidegrid->addWidget(spin, y, 0);
			QToolButton* butt_sv = 0;
			if (j->joint_has_limits) {
				butt_sv = new QToolButton();
				butt_sv->setObjectName(QString("%1") . arg(c));
				butt_sv->setText("Sv");
				butt_sv->setAutoRaise(true);
				QObject::connect(butt_sv, SIGNAL(clicked(bool)), this, SLOT(sv_clicked()));
				sidegrid->addWidget(butt_sv, y, 1);
				joint_control_mode[c] = 0;
			} else {
				joint_control_mode[c] = 1;
			}
			QToolButton* butt_ve = new QToolButton();
			QToolButton* butt_tq = new QToolButton();
			butt_ve->setObjectName(QString("%1") . arg(c));
			butt_tq->setObjectName(QString("%1") . arg(c));
			butt_ve->setText("Ve");
			butt_tq->setText("Tq");
			butt_ve->setAutoRaise(true);
			butt_tq->setAutoRaise(true);
			QObject::connect(butt_ve, SIGNAL(clicked(bool)), this, SLOT(ve_clicked()));
			QObject::connect(butt_tq, SIGNAL(clicked(bool)), this, SLOT(tq_clicked()));
			sidegrid->addWidget(butt_ve, y, 2);
			sidegrid->addWidget(butt_tq, y, 3);
			joint_button_sv[c] = butt_sv;
			joint_button_ve[c] = butt_ve;
			joint_button_tq[c] = butt_tq;
			y += 1;
			QLabel* lab = new QLabel();
			lab->setFont(small_font);
			joint_labels[c] = lab;
			sidegrid->addWidget(lab);
			y += 1;
			sidegrid->setRowMinimumHeight(y, 10);
			y += 1;
		}

		refresh_joint_control_modes();

		startTimer(10);
	}

	~TestWindow()
	{
		delete viz;
	}

	int ts = 0;
	int view_lines = 0;

	void timeout()
	{
		ts += 1;
		double phase = (ts % 300) / 300.0;

		if (the_model_along_y != model_along_y->isChecked()) {
			the_model_along_y = model_along_y->isChecked();
			last_reload_time = 0;
		}

		load_model_or_robot();

		if (auto_rotate->isChecked())
			viz->zrot += 0.05;

		if (auto_switch->isChecked()) {
			if (phase > 0.6) {
				viz->dup_transparent_mode = 2;
			} else if (phase > 0.3) {
				viz->dup_transparent_mode = 1;
			} else {
				viz->dup_transparent_mode = 0;
			}
		}

		if (draw_lines->isChecked() != view_lines) {
			if (draw_lines->isChecked())
				viz->view_options |= SimpleRender::VIEW_LINES;
			else
				viz->view_options &= ~SimpleRender::VIEW_LINES;

		} else if ( !!(viz->view_options & SimpleRender::VIEW_LINES) != view_lines) {
			view_lines = !!(viz->view_options & SimpleRender::VIEW_LINES);
			draw_lines->setChecked(view_lines);
		}

		if (the_robot)
		for (int c=0; c<(int)the_robot->joints.size(); ++c) {
			shared_ptr<Joint> j = the_robot->joints[c];
			if (!j) continue;
			float pos, speed;
			j->joint_current_position(&pos, &speed);
			joint_labels[c]->setText(QString("%1 %2") . arg(j->joint_name.c_str()) . arg(pos, 0, 'f', 3));
			QDoubleSpinBox* spin = joint_spins[c];
			float p = spin->value();
			switch (joint_control_mode[c]) {
			case 0: j->set_servo_target(p, 0.1, 0.1, 0.1, 5); break;
			case 1: j->set_target_speed(p, 0.1, 0.1); break;
			case 2: j->set_motor_torque(p); break;
			default: assert(0);
			}
		}
		world->bullet_step(1);
		viz->render_on_offscreen_surface();
		viz->update();
	}

	void keyPressEvent(QKeyEvent* kev)
	{
		if (kev->key()==Qt::Key_F1)
			viz->dup_transparent_mode = (viz->dup_transparent_mode + 1) % 3;
		else if (kev->key()==Qt::Key_F3)
			last_reload_time = 0;
		else kev->ignore();
	}

	bool event(QEvent* ev)
	{
		if (ev->type()==QEvent::Timer)
			timeout();
		return QWidget::event(ev);
	}

	void refresh_joint_control_modes()
	{
		if (!the_robot) return;
		for (int c=0; c<(int)the_robot->joints.size(); ++c) {
			if (!the_robot->joints[c]) continue;
			if (joint_button_sv[c])
				joint_button_sv[c]->setCheckable(true);
			joint_button_ve[c]->setCheckable(true);
			joint_button_tq[c]->setCheckable(true);
			if (joint_button_sv[c])
				joint_button_sv[c]->setChecked(joint_control_mode[c]==0);
			joint_button_ve[c]->setChecked(joint_control_mode[c]==1);
			joint_button_tq[c]->setChecked(joint_control_mode[c]==2);
		}
	}

public slots:
	void sv_clicked()
	{
		int c = sender()->objectName().toInt();
		joint_control_mode[c] = 0;
		refresh_joint_control_modes();
	}

	void ve_clicked()
	{
		int c = sender()->objectName().toInt();
		joint_control_mode[c] = 1;
		refresh_joint_control_modes();
	}

	void tq_clicked()
	{
		int c = sender()->objectName().toInt();
		joint_control_mode[c] = 2;
		refresh_joint_control_modes();
	}
};

int main(int argc, char *argv[])
{
#ifdef Q_MAC_OS
[NSApp activateIgnoringOtherApps:YES];
#endif
	QApplication app(argc, argv);
	if (argc < 2) {
		fprintf(stderr, "This is robot/model testing utility.\n\nUsage:\n");
		fprintf(stderr, "%s <urdf_resources_dir> <urdf>\n", argv[0]);
		fprintf(stderr, "%s <model_obj_dae_stl_etc>\n", argv[0]);
		fprintf(stderr, "\nUseful examples:\n");
		fprintf(stderr, "%s models_robot r2d2.urdf\n", argv[0]);
		fprintf(stderr, "%s models_robot fetch_description/robots/fetch.urdf\n", argv[0]);
		fprintf(stderr, "%s models_robot pr2_description/pr2_desc.urdf\n", argv[0]);
		return 1;
	}

	fprintf(stderr, "Press F3 to force reload (when you change dependencies of file)\n");

	the_filename = argv[1];

	world.reset(new World);
	world->bullet_init(0.0*SCALE, 1/60.0);

	load_model_or_robot();
	if (!the_robot && !the_model) return 1; // warning printed

	TestWindow window(world);
	window.resize(1280, 1024);
	window.show();
	window.setWindowTitle(QString::fromUtf8(the_filename.c_str()));

	QEventLoop loop;
	while (1) {
		world->bullet_step(1);
		world->query_positions();
		loop.processEvents(QEventLoop::AllEvents);
		if (!window.isVisible()) break;
	}

	the_model.reset();
	the_robot.reset();
	world.reset();

	return 0;
}

#include ".generated/test-tool-qt4.moc"

