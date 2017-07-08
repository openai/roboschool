#define GL_GLEXT_PROTOTYPES
#include "render-glwidget.h"

#include <QtOpenGL/QtOpenGL>
#include <QtGui/QKeyEvent>

using boost::shared_ptr;
using namespace SimpleRender;
using namespace Household;

Viz::Viz(const shared_ptr<Context>& cx_):
	QOpenGLWidget()
{
	cx = cx_;
	//setFormat(cx->fmt);
	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);
	QFont font("Courier", 12);
	setFont(font);
	font_score = QFont("Courier", 36);
	font_score.setBold(true);
}

void Viz::initializeGL()
{
}

void Camera::camera_render(const shared_ptr<SimpleRender::Context>& cx, bool render_depth, bool render_labeling, bool print_timing)
{
	const int RGB_OVERSAMPLING = 1; // change me to see the difference (good values 0 1 2)
	const int AUX_OVERSAMPLING = 2;

	int dw = camera_res_w;
	int dh = camera_res_h;
	int ow = camera_res_w << RGB_OVERSAMPLING;
	int oh = camera_res_h << RGB_OVERSAMPLING;

	cx->glcx->makeCurrent(cx->surf);
	CHECK_GL_ERROR;

	if (!viewport || viewport->W!=ow || viewport->H!=oh) {
		viewport.reset(new SimpleRender::ContextViewport(cx, ow, oh, camera_near, camera_far, camera_hfov));
		CHECK_GL_ERROR;
	}

	double rgb_depth_render = 0;
	double rgb_oversample = 0;
	double dep_oversample = 0;
	double metatype_render = 0;
	double metatype_oversample = 0;

	QElapsedTimer timer;
	timer.start();

	viewport->paint(0, 0, 0, 0, 0, 0, this, 65535, VIEW_CAMERA_BIT, 0); // PAINT HERE
	CHECK_GL_ERROR;
	viewport->hud_update_start();
	viewport->hud_print_score(score);
	viewport->hud_update_finish();
	CHECK_GL_ERROR;

	rgb_depth_render = timer.nsecsElapsed()/1000000.0;

	// rgb
	timer.start();
	camera_rgb.resize(3*camera_res_w*camera_res_h);
	uint8_t tmp[4*ow*oh]; // only 3*ow*oh required, but glReadPixels() somehow touches memory after this buffer, demonstrated on NVidia 375.20

	glReadPixels(0, 0, ow, oh, GL_RGB, GL_UNSIGNED_BYTE, tmp);

	if (RGB_OVERSAMPLING==0) {
		for (int y=0; y<oh; ++y)
			memcpy(&camera_rgb[y*3*ow], &tmp[(oh-1-y)*3*ow], 3*ow);
	} else {
		uint16_t acc[3*dw*dh];
		memset(acc, 0, sizeof(uint16_t)*3*dw*dh);
		int rs = 3*dw;
		for (int oy=0; oy<oh; oy++) {
			int dy = oy >> RGB_OVERSAMPLING;
			uint8_t* src = &tmp[(oh-1-oy)*3*ow];
			for (int ox=0; ox<ow; ox++) {
				int dx = ox >> RGB_OVERSAMPLING;
				acc[dy*rs + 3*dx + 0] += src[3*ox + 0];
				acc[dy*rs + 3*dx + 1] += src[3*ox + 1];
				acc[dy*rs + 3*dx + 2] += src[3*ox + 2];
			}
		}
		uint8_t* dst = (uint8_t*) &camera_rgb[0];
		for (int t=0; t<3*dw*dh; t++)
			dst[t] = acc[t] >> (RGB_OVERSAMPLING+RGB_OVERSAMPLING);
	}
	rgb_oversample = timer.nsecsElapsed()/1000000.0;

	// depth from the same render
	int auxw = ow >> AUX_OVERSAMPLING;
	int auxh = oh >> AUX_OVERSAMPLING;
	if (render_depth) {
		timer.start();
		camera_aux_w = auxw;
		camera_aux_h = auxh;
		camera_depth.resize(sizeof(float)*auxw*auxh);
		camera_depth_mask.resize(auxw*auxh);
		float ftmp[ow*oh];

		glReadPixels(0, 0, ow, oh, GL_DEPTH_COMPONENT, GL_FLOAT, ftmp);

		if (AUX_OVERSAMPLING==0) {
			for (int y=0; y<oh; ++y) {
				float* dst = (float*) &camera_depth[4*y*ow];
				uint8_t* msk = (uint8_t*) &camera_depth_mask[y*ow];
				float* src = &ftmp[(oh-1-y)*ow];
				for (int x=0; x<ow; ++x) {
					float mean = src[x];
					dst[x] = fmax(-1, (mean - 0.9) * (1/0.1)); // change !=0 branch!
					msk[x] = 1;
				}
			}
		} else {
			float acc[auxw*auxh];
			float sqr[auxw*auxh];
			memset(acc, 0, sizeof(float)*auxw*auxh);
			memset(sqr, 0, sizeof(float)*auxw*auxh);
			int rs = auxw;
			for (int oy=0; oy<oh; oy++) {
				int dy = oy >> AUX_OVERSAMPLING;
				float* src = &ftmp[(oh-1-oy)*ow];
				for (int ox=0; ox<ow; ox++) {
					int dx = ox >> AUX_OVERSAMPLING;
					acc[dy*rs + dx] += src[ox];
					sqr[dy*rs + dx] += src[ox]*src[ox];
				}
			}
			float* dst = (float*) &camera_depth[0];
			uint8_t* msk = (uint8_t*) &camera_depth_mask[0];
			//float min = +1e10;
			//float max = -1e10;
			for (int t=0; t<auxw*auxh; t++) {
				float mean = acc[t] * (1.0/(1 << (AUX_OVERSAMPLING+AUX_OVERSAMPLING)));
				float std_squared = sqr[t] * (1.0/(1 << (AUX_OVERSAMPLING+AUX_OVERSAMPLING))) - mean*mean;
				// a==0 very close, almost clipped
				// a==0.8 half of manipulator reach
				// a==1 infinity
				// useful range 0.8 .. 1.0
				dst[t] = fmax(-1, (mean - 0.9) * (1/0.1)); // change ==0 branch!
				//if (dst[t] < min) min = dst[t];
				//if (dst[t] > max) max = dst[t];
				msk[t] = (std_squared < 0.000002) && (dst[t] < 0.90); // ignore far away
			}
			//fprintf(stderr, " %0.5f .. %0.5f\n", min, max);
		}
		dep_oversample = timer.nsecsElapsed()/1000000.0;
	}

	// dense object type presence, from different render (types as color)
	if (render_labeling) {
		timer.start();

		viewport->paint(0, 0, 0, 0, 0, 0, this, 65535, VIEW_METACLASS|VIEW_CAMERA_BIT, 0); // PAINT HERE

		camera_labeling.resize(auxw*auxh);
		camera_labeling_mask.resize(auxw*auxh);
		glReadPixels(0, 0, ow, oh, GL_RGB, GL_UNSIGNED_BYTE, tmp);
		metatype_render = timer.nsecsElapsed()/1000000.0;

		timer.start();
		if (AUX_OVERSAMPLING==0) {
			assert(auxw==ow && auxh==oh);
			for (int y=0; y<oh; ++y) {
				uint8_t* dst = (uint8_t*) &camera_labeling[y*auxw];
				uint8_t* msk = (uint8_t*) &camera_labeling_mask[y*auxw];
				for (int x=0; x<ow; ++x) {
					uint8_t t = tmp[(oh-1-y)*3*ow + 3*x + 2];
					dst[x] = t;
					msk[x] = 1;
				}
			}
		} else {
			uint8_t bits_count[auxw*auxh*8];
			memset(bits_count, 0, auxw*auxh*8);
			for (int oy=0; oy<oh; ++oy) {
				int dy = oy >> AUX_OVERSAMPLING;
				uint8_t* dst_count = &bits_count[dy*8*auxw];
				uint8_t* src = &tmp[(oh-1-oy)*3*ow];
				for (int ox=0; ox<ow; ++ox) {
					int dx = ox >> AUX_OVERSAMPLING;
					uint8_t t = src[3*ox + 2];
					dst_count[8*dx + 0] += (t & 0x01) >> 0;
					dst_count[8*dx + 1] += (t & 0x02) >> 1;
					dst_count[8*dx + 2] += (t & 0x04) >> 2;
					dst_count[8*dx + 3] += (t & 0x08) >> 3;
					dst_count[8*dx + 4] += (t & 0x10) >> 4;
					dst_count[8*dx + 5] += (t & 0x20) >> 5;
					dst_count[8*dx + 6] += (t & 0x40) >> 6;
					dst_count[8*dx + 7] += (t & 0x80) >> 7;
				}
			}
			uint8_t* dst = (uint8_t*) &camera_labeling[0];
			uint8_t* msk = (uint8_t*) &camera_labeling_mask[0];
			uint8_t threshold      = (1 << (AUX_OVERSAMPLING+AUX_OVERSAMPLING)) * 3 / 3; // 3/3 of subpixels
			uint8_t threshold_item = (1 << (AUX_OVERSAMPLING+AUX_OVERSAMPLING)) * 2 / 3; // 2/3 of subpixels
			for (int t=0; t<auxw*auxh; t++) {
				uint8_t* src_count = &bits_count[8*t];
				dst[t] =
					((src_count[0] >= threshold)<<0) +
					((src_count[1] >= threshold)<<1) +
					((src_count[2] >= threshold)<<2) +
					((src_count[3] >= threshold)<<3) +
					((src_count[4] >= threshold_item)<<4) + // METACLASS_HANDLE
					((src_count[5] >= threshold_item)<<5) + // METACLASS_ITEM
					((src_count[6] >= threshold)<<6) +
					((src_count[7] >= threshold)<<7);
				msk[t] = !!dst[t];
				//float mean = acc[t] * (1.0/(1 << (AUX_OVERSAMPLING+AUX_OVERSAMPLING)));
				//float std_squared = sqr[t] * (1.0/(1 << (AUX_OVERSAMPLING+AUX_OVERSAMPLING))) - mean*mean;
			}
		}
		metatype_oversample = timer.nsecsElapsed()/1000000.0;
	}

	bool balance_classes = true;
	if (balance_classes && render_labeling) {
		int count_floor = 0;
		int count_walls = 0;
		int count_items = 0;
		uint8_t* msk1 = (uint8_t*) &camera_labeling_mask[0];
		uint8_t* msk2 = (uint8_t*) &camera_depth_mask[0];
		uint8_t* lab = (uint8_t*) &camera_labeling[0];
		for (int t=0; t<auxw*auxh; t++) {
			if (msk1[t]==0) continue;
			count_floor += (lab[t] & METACLASS_FLOOR) ? 1 : 0;
			count_walls += (lab[t] & METACLASS_WALL) ? 1 : 0;
			count_items += (lab[t] & (METACLASS_FURNITURE|METACLASS_HANDLE|METACLASS_ITEM)) ? 1 : 0;
		}
		double too_many_walls = double(count_walls + count_floor) / (count_items + 50); // assume 50 item points always visible (50 of 80x64 == 1%)
		if (too_many_walls > 1) {
			int threshold = int(RAND_MAX / too_many_walls);
			for (int t=0; t<auxw*auxh; t++) {
				if (msk1[t]==0) continue;
				if (!(lab[t] & (METACLASS_FLOOR|METACLASS_WALL))) continue;
				if (rand() < threshold) continue;
				msk1[t] = 0;
				msk2[t] = 0;
			}
		}
	}

	if (print_timing) fprintf(stderr,
		"rgb_depth_render=%6.2lfms  "
		"rgb_oversample=%6.2lfms  "
		"dep_oversample=%6.2lfms  "
		"metatype_render=%6.2lfms  "
		"metatype_oversample=%6.2lfms\n",
		rgb_depth_render,
		rgb_oversample,
		dep_oversample,
		metatype_render,
		metatype_oversample
		);
}


void Viz::resizeGL(int w, int h)
{
	QDesktopWidget* desk = QApplication::desktop();
	qreal rat = desk->windowHandle()->devicePixelRatio();
	win_w  = int(w*rat+0.5);
	win_w +=  0xF;
	win_w &= ~0xF;
	win_h  = int(h*rat+0.5);
	//fprintf(stderr, "resizeGL(%i, %i) -> pixels %i x %i\n", w, h, win_w, win_h);
	if (w>0 && h>0) {
		resized = true;
		dummy_fbuf.reset();
	}
}

void Viz::_render_on_correctly_set_up_context()
{
	if (resized) {
		resized = false;
		float near = 0.1;
		float far  = 100.0;
		float hfov = 90;
		render_viewport.reset(new SimpleRender::ContextViewport(cx, win_w, win_h, near, far, hfov));
		render_viewport_resized = true;
		caption.render(0x880000, win_w);
	}
	if (!render_viewport) return;

	QElapsedTimer elapsed;
	elapsed.start();
	if (dup_transparent_mode!=2) {
		uint32_t opt = view_options;
		bool hurray = QOpenGLContext::areSharing(
			cx->glcx,
			context()
			);
		assert(hurray);
		render_viewport->paint(user_x, user_y, user_z, wheel, zrot, yrot, 0, floor_visible, opt, ruler_size);
		CHECK_GL_ERROR;
		ms_render_objectcount = render_viewport->visible_object_count;
	}
	double ms_objects = elapsed.nsecsElapsed() / 1000000.0;

	elapsed.start();
	_paint_hud();
	CHECK_GL_ERROR;
	double ms_billboards = elapsed.nsecsElapsed() / 1000000.0;

	//if (dup_transparent_mode>0) {
	//cx->pure_color_opacity = dup_opacity;
	//render_viewport->paint(user_x, user_y, user_z, wheel, zrot, yrot, 0, floor_visible, view_options | VIEW_COLLISION_SHAPE, ruler_size);
	//cx->pure_color_opacity = 1;
	//}

	ms_render *= 0.9;
	ms_render += 0.1*(ms_objects + ms_billboards);
}

void Viz::render_on_offscreen_surface()
{
#ifdef USE_SSAO
	cx->glcx->makeCurrent(cx->surf);
	CHECK_GL_ERROR;
	_render_on_correctly_set_up_context();
	CHECK_GL_ERROR;
	cx->glcx->doneCurrent();
#endif
}

void Viz::paintGL()
{
#ifdef USE_SSAO
	if (!render_viewport) return;
	if (!dummy_fbuf || render_viewport_resized) {
		render_viewport_resized = false;
		dummy_fbuf.reset(new Framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, dummy_fbuf->handle);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, render_viewport->tex_color->handle, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		CHECK_GL_ERROR;
	}
	glBindFramebuffer(GL_READ_FRAMEBUFFER, dummy_fbuf->handle);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, defaultFramebufferObject());
	glBlitFramebuffer(0,0,render_viewport->W,render_viewport->H, 0,0,win_w,win_h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	CHECK_GL_ERROR;
#else
	// No shadows: just render directly
	_render_on_correctly_set_up_context();
	CHECK_GL_ERROR;
#endif
}

Viz::~Viz()
{
#ifdef USE_SSAO
	cx->glcx->makeCurrent(cx->surf);
	cx.reset(); // Destructor can be here or as well not be (in case this objects is currently used somewhere else)
#endif
	makeCurrent();
}

void Viz::keyReleaseEvent(QKeyEvent* kev)
{
	activate_key_callback(kev->type(), kev->key(), kev->modifiers());
}

void Viz::keyPressEvent(QKeyEvent* kev)
{
	activate_key_callback(kev->type(), kev->key(), kev->modifiers());
	bool asdf_move = false;
	if (asdf_move && (kev->key()==Qt::Key_A || kev->key()==Qt::Key_D)) {
		user_move(0.05*(kev->key()==Qt::Key_A ?  -1 : +1), 0);
	} else if (asdf_move && (kev->key()==Qt::Key_W || kev->key()==Qt::Key_S)) {
		user_move(0, 0.05*(kev->key()==Qt::Key_S ?  +1 : -1));
	} else if ((kev->key()==Qt::Key_PageDown || kev->key()==Qt::Key_PageUp)) {
		double sign = kev->key()==Qt::Key_PageDown ?  -1 : +1;
		user_z += sign * 0.05;
	}
	else if (kev->key()==Qt::Key_QuoteLeft) view_options ^= VIEW_LINES;
	else if (kev->key()==Qt::Key_Tab) view_options ^= VIEW_COLLISION_SHAPE;
	else if (kev->key()==Qt::Key_1) floor_visible = 0;
	else if (kev->key()==Qt::Key_2) floor_visible = 1;
	else if (kev->key()==Qt::Key_3) floor_visible = 2;
	else if (kev->key()==Qt::Key_4) floor_visible = 3;
	else if (kev->key()==Qt::Key_5) floor_visible = 4;
	else if (kev->key()==Qt::Key_F1) cx->slowmo ^= true;
	else if (kev->key()==Qt::Key_F2) view_options ^= VIEW_NO_CAPTIONS;
	else if (kev->key()==Qt::Key_F3) view_options ^= VIEW_NO_HUD;
	//else if (kev->key()==0x21) render_viewport->ssao_debug = 0;
	//else if (kev->key()==0x22 || kev->key()==0x40) render_viewport->ssao_debug = 1;
	//else if (kev->key()==0x23) render_viewport->ssao_debug = 2;
	//else if (kev->key()==0x24) render_viewport->ssao_debug = 3;
	//else if (kev->key()==0x29) render_viewport->ssao_enable ^= true;
	else kev->ignore();
}

void Viz::wheelEvent(QWheelEvent* wev)
{
	wheel *= (1 - 0.001*wev->delta());
}

void Viz::mousePressEvent(QMouseEvent* mev)
{
	if (mev->button()==Qt::LeftButton || mev->button()==Qt::RightButton) {
		mouse_init_x = mouse_prev_x = mev->x();
		mouse_init_y = mouse_prev_y = mev->y();
		drag = (mev->button()==Qt::LeftButton && mev->modifiers()!=Qt::ControlModifier) ? DRAG_ROTATE : DRAG_MOVE;
	}
}

void Viz::mouseReleaseEvent(QMouseEvent* mev)
{
	if (mev->button()==Qt::LeftButton && drag==DRAG_ROTATE) {
		drag = DRAG_NONE;
		double traveled = fabs(mouse_init_x - mev->x()) + fabs(mouse_init_y - mev->y());
		if (traveled < 5) {
			// clicked
		}
	}
	if (mev->button()==Qt::RightButton && drag==DRAG_MOVE) {
		drag = DRAG_NONE;
	}
}

void Viz::user_move(float hor, float vert)
{
	double fx = cos(zrot / 180 * 3.1415926);
	double fy = sin(zrot / 180 * 3.1415926);
	double sx = -fy;
	double sy =  fx;
	user_x += hor * 0.10 * fx;
	user_y -= hor * 0.10 * fy;
	user_x += vert * 0.10 * sx;
	user_y -= vert * 0.10 * sy;
}

void Viz::mouseMoveEvent(QMouseEvent* mev)
{
	if (drag==DRAG_ROTATE) {
		zrot += 0.12*(mev->x() - mouse_prev_x);
		yrot += 0.12*(mev->y() - mouse_prev_y);
		mouse_prev_x = mev->x();
		mouse_prev_y = mev->y();
	} else if (drag==DRAG_MOVE) {
		user_move(-0.008*(mev->x() - mouse_prev_x)*wheel, -0.002*(mev->y() - mouse_prev_y)*wheel);
		mouse_prev_x = mev->x();
		mouse_prev_y = mev->y();
	}
	if (mev->buttons()==0) {
		drag = DRAG_NONE;
	}
}

void Viz::timeout()
{
	if (isVisible())
		update();
}

bool Viz::event(QEvent* ev)
{
	if (ev->type()==QEvent::Timer)
		timeout();
	return QOpenGLWidget::event(ev);
}


// --- camera viz ---

QSize VizCamera::sizeHint() const
{
	boost::shared_ptr<Household::Camera> c = cref.lock();
	return QSize(4*MARGIN + 2*(c ? c->camera_res_w : 192)*3, 2*MARGIN + 2*(c ? c->camera_res_h : 128));
}

void VizCamera::paintEvent(QPaintEvent* ev)
{
	QPainter p(this);
	p.fillRect(ev->rect(), QColor(QRgb(0xFFFFFF)));
	boost::shared_ptr<Household::Camera> camera = cref.lock();
	if (!camera) return;
	int w = camera->camera_res_w;
	int h = camera->camera_res_h;
	int aux_w = camera->camera_aux_w;
	int aux_h = camera->camera_aux_h;
	int SCALE = 2;

	// rgb
	QImage img_rgb(w, h, QImage::Format_RGB32);
	img_rgb.fill(QColor(QRgb(0xFFFFFF)));
	for (int y=0; y<h; y++) {
		uchar* u = img_rgb.scanLine(y);
		uint8_t* src = (uint8_t*) &camera->camera_rgb[3*w*y];
		for (int x=0; x<w; x++) {
			u[4*x + 2] = src[3*x + 0];
			u[4*x + 1] = src[3*x + 1];
			u[4*x + 0] = src[3*x + 2];
		}
	}
	p.drawImage( QRect(MARGIN, MARGIN, w*SCALE, h*SCALE), img_rgb);

	// depth
	QImage img_aux(aux_w, aux_h, QImage::Format_RGB32);
	img_aux.fill(0xE0E0E0);
	static bool inited;
	static uint32_t palette[1024];
	static uint32_t mpalette[256];
	const int palette_size = sizeof(palette)/sizeof(palette[0]);
	if (!inited) {
		float palette_size1 = 1.0 / palette_size;
		for (int c=0; c<palette_size; c++) {
			QColor t;
			t.setHsvF(c*palette_size1, 1, 1);
			//t.setHsvF(c*palette_size1, 1, 0.7 + 0.3*sin(c*palette_size1*30));
			palette[c] = t.rgb();
		}
		for (int c=0; c<int(sizeof(mpalette)/sizeof(palette[0])); c++) {
			uint32_t color = 0;
			if (c & METACLASS_FLOOR)     color |= 0x0000FF;
			if (c & METACLASS_WALL)      color |= 0x000080;
			if (c & METACLASS_MYSELF)    color |= 0x400000;
			if (c & METACLASS_FURNITURE) color |= 0x008800;
			if (c & METACLASS_HANDLE)    color |= 0xFF8800;
			if (c & METACLASS_ITEM)      color |= 0xFF0000;
			mpalette[c] = color;
		}
	}
	if (!camera->camera_depth.empty())
	for (int y=0; y<aux_h; y++) {
		uchar* u = img_aux.scanLine(y);
		float* src = (float*) &camera->camera_depth[4*aux_w*y];
		uint8_t* msk = (uint8_t*) &camera->camera_depth_mask[aux_w*y];
		for (int x=0; x<aux_w; x++) {
			int ind = int(src[x] * 1024);
			(uint32_t&) u[4*x] = msk[x] ? palette[ind & (palette_size-1)] : 0x000000;
		}
	}
	p.drawImage( QRect(MARGIN + SCALE*w + MARGIN, MARGIN, w*SCALE, h*SCALE), img_aux);

	// metaclass
	if (!camera->camera_labeling.empty())
	for (int y=0; y<aux_h; y++) {
		uchar* u = img_aux.scanLine(y);
		uint8_t* src = (uint8_t*) &camera->camera_labeling[aux_w*y];
		uint8_t* msk = (uint8_t*) &camera->camera_labeling_mask[aux_w*y];
		for (int x=0; x<aux_w; x++) {
			(uint32_t&) u[4*x] = msk[x]*mpalette[src[x]];
		}
	}
	p.drawImage( QRect(MARGIN + SCALE*w + MARGIN + SCALE*w + MARGIN, MARGIN, w*SCALE, h*SCALE), img_aux);
	setWindowTitle(QString("RGB %1x%2, AUX %3x%4") . arg(w) . arg(h) . arg(camera->camera_aux_w) . arg(camera->camera_aux_h));
}

void Viz::activate_key_callback(int event_type, int key, int modifiers)
{
	boost::shared_ptr<KeyCallback> k = key_callback.lock();
	if (k) k->key_callback(event_type, key, modifiers);
}

void VizCamera::activate_key_callback(int event_type, int key, int modifiers)
{
	boost::shared_ptr<KeyCallback> k = key_callback.lock();
	if (k) k->key_callback(event_type, key, modifiers);
}

void VizCamera::keyPressEvent(QKeyEvent* kev)
{
	activate_key_callback(kev->type(), kev->key(), kev->modifiers());
}

void VizCamera::keyReleaseEvent(QKeyEvent* kev)
{
	activate_key_callback(kev->type(), kev->key(), kev->modifiers());
}

