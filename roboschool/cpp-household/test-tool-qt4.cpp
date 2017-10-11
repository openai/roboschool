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
#include <QtCore/QElapsedTimer>

#include <bullet/PhysicsClientC_API.h>

using boost::shared_ptr;
using namespace Household;

namespace Household {
btScalar SCALE = 1.0;
}

static shared_ptr<World>  world;

static shared_ptr<Robot>  the_robot;
static std::list<shared_ptr<Robot>> other_robots_keep_reference;
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
	try {
		time_t t = mtime(the_filename);
		if (last_reload_time==t)
			return;
		world->clean_everything();
		world->klass_cache_clear();
		btQuaternion quat(0,0,0,1);
		if (the_model_along_y)
			quat.setEuler(0, M_PI/2, 0);
		last_reload_time = t;

		QString test = QString::fromLocal8Bit(the_filename.c_str());
		if (test.endsWith(".urdf")) {
			QElapsedTimer timer;
			timer.start();
			the_robot = world->load_urdf(the_filename, btTransform(quat, btVector3(0,0,0)), false, false);
			double load_ms = timer.nsecsElapsed()/1000000.0;
			fprintf(stderr, "%s loaded in %0.2lfms\n", the_filename.c_str(), load_ms);

		} else if (test.endsWith(".xml") || test.endsWith(".sdl")) {
			the_robot.reset();
			std::list<shared_ptr<Robot>> robots = world->load_sdf_mjcf(the_filename, test.endsWith(".xml"));
			other_robots_keep_reference = robots;
			for (auto r: robots)
				if (!the_robot || the_robot->joints.size() < r->joints.size())
					the_robot = r;

		} else {
			the_model = world->load_thingy(the_filename, btTransform(quat, btVector3(0,0,0)), 1.0, 1.0, 0xFF0000, false);
		}

	} catch (const std::exception& e) {
		fprintf(stderr, "ERROR: %s\n", e.what());
	}
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
		viz = new Viz(world->cx);
		viz->wheel /= SCALE;
		viz->wheel *= 0.5; // bigger scale on start, compared to house
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
			spin->setValue(j->joint_current_position);
			spin->setSingleStep(0.1);
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
			//joint_control_mode[c] = 2;
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

		if (the_robot && the_robot->joints.size() > 0) {
			rolling_state_update = (rolling_state_update + 1) % (int)the_robot->joints.size();
			for (int c=0; c<(int)the_robot->joints.size(); ++c) {
				shared_ptr<Joint> j = the_robot->joints[c];
				if (!j) continue;
				QDoubleSpinBox* spin = joint_spins[c];
				float p = spin->value();
				switch (joint_control_mode[c]) {
				case 0: j->set_relative_servo_target(p, 0.08, 0.8); break;
				case 1: j->set_target_speed(p, 0.1, 20); break;
				case 2: j->set_motor_torque(p); break;
				default: assert(0);
				}
				if (rolling_state_update==c) {
					float pos, speed;
					j->joint_current_relative_position(&pos, &speed);
					joint_labels[c]->setText(QString("%1 %2") . arg(j->joint_name.c_str()) . arg(pos, 0, 'f', 3)); // this is slow
				}
			}
		}

		viz->render_on_offscreen_surface();
		viz->repaint();
	}

	int rolling_state_update = 0;

	void keyPressEvent(QKeyEvent* kev)
	{
		if (kev->key()==Qt::Key_F1)
			viz->dup_transparent_mode = (viz->dup_transparent_mode + 1) % 3;
		else if (kev->key()==Qt::Key_F3)
			last_reload_time = 0;
		else kev->ignore();
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

	world.reset(new World);
	SimpleRender::opengl_init_before_app(world);
	QApplication app(argc, argv);
	SimpleRender::opengl_init(world);

	fprintf(stderr, "Press F3 to force reload (when you change dependencies of file)\n");

	the_filename = argv[1];

	world->bullet_init(0.0*SCALE, 1/60.0/4);

	load_model_or_robot();
	if (!the_robot && !the_model) return 1; // warning printed


	TestWindow window(world);
	window.resize(1280, 1024);
	window.show();

	QEventLoop loop;
	while (1) {
		world->bullet_step(1); // Slowmo, 4 for normal.
		window.timeout();
		loop.processEvents(QEventLoop::AllEvents);
		if (!window.isVisible()) break;
		window.setWindowTitle(
			QString("%4 — %1 objects, %2ms bullet, %3ms render")
			. arg(window.viz->ms_render_objectcount)
			. arg(world->performance_bullet_ms, 0, 'f', 2)
			. arg(window.viz->ms_render, 0, 'f', 2)
			. arg(QString::fromLocal8Bit(the_filename.c_str())) );
	}

	the_model.reset();
	the_robot.reset();
	world.reset();

	return 0;
}

#include ".generated/test-tool-qt4.moc"

