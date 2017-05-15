#define GL_GLEXT_PROTOTYPES
#include "render-glwidget.h"
#include <QtOpenGL/QtOpenGL>

static const int HIST = 150;

void Viz::_paint_hud()
{
	using namespace SimpleRender;
	const float MARGIN = 5.0f;

//	if (!billboards.empty()) {
//		const int TIMEOUT_SEC = 3;
//		if (billboards.back().ts + 1000*TIMEOUT_SEC < QDateTime::currentMSecsSinceEpoch())
//			billboards.pop_back();
//		for (ConsoleMessage& msg: billboards) {
//			glRasterPos3f( msg.pos.x(), msg.pos.y(), msg.pos.z() );
//			glPixelZoom(1.0, -1.0);
//			glDrawPixels(
//				msg.msg_image.width(), msg.msg_image.height(),
//				GL_BGRA, GL_UNSIGNED_BYTE,
//				msg.msg_image.bits() );
//			msg.pos.m_floats[2] += 0.01*Household::SCALE; // float upwards
//		}
//	}

	if ((~view_options & (VIEW_NO_HUD|VIEW_NO_CAPTIONS))==0) return;

	render_viewport->hud_update_start();

	int top;
	if (~view_options&VIEW_NO_CAPTIONS)
		top = render_viewport->hud_print_score(score);
	else
		top = MARGIN;

	QRect area(QPoint(0,0), render_viewport->hud_image.size());
	QPainter p(&render_viewport->hud_image);
	p.setCompositionMode(QPainter::CompositionMode_Source);
	p.setPen(QColor(0xFFFFFF));
	p.setFont(cx->score_font_small);

	int bottom = win_h - MARGIN;
	top += MARGIN;
	if (( !console.empty() || !caption.msg_image.isNull() ) && (~view_options&VIEW_NO_CAPTIONS)) {
		const int CONSOLE_SPACING     = 10;
		const int CONSOLE_TIMEOUT_SEC = 7;
		if (!console.empty() && console.back().ts + 1000*CONSOLE_TIMEOUT_SEC < QDateTime::currentMSecsSinceEpoch())
			console.pop_back();

		if (!caption.msg_text.isEmpty()) {
			QRect r(0, win_h - caption.msg_image.height(), caption.msg_image.width(), caption.msg_image.height());
			p.drawImage(r, caption.msg_image);
			render_viewport->hud_update(r);
			bottom -= caption.msg_image.height();
		}

		for (const ConsoleMessage& msg: console) {
			bottom -= msg.msg_image.height();
			if (bottom < 0) break;
			QRect r(MARGIN, bottom, msg.msg_image.width(), msg.msg_image.height());
			p.drawImage(r, msg.msg_image);
			render_viewport->hud_update(r);
			bottom -= CONSOLE_SPACING;
		}
	}

	history_advance(true);

	int right = area.right() - HIST - MARGIN;
	if (obs_hist.size() && (~view_options&VIEW_NO_HUD)) {
		for (int c=0; c<(int)obs.size(); ++c) {
			int x = right;
			int y = top + 26*c;
			QRect r(x, y, HIST, 20);
			if ( (area & r) != r ) continue;
			if ( r.bottom() > bottom ) continue;
			drawhist(p, "obs", c, r, &obs_hist[0] + HIST*c, &obs[0] + c);
		}
	}

	int top_reward = bottom;
	if (reward_hist.size() && (~view_options&VIEW_NO_HUD)) {
		for (int c=0; c<(int)reward.size(); ++c) {
			int x = MARGIN;
			int y = bottom - 56.0*((int)reward.size()-1-c);
			QRect r(x, y-50, HIST, 50);
			if ( (area & r) != r ) continue;
			top_reward = r.top();
			if ( r.top() < top ) continue;
			drawhist(p, "rew", c, r, &reward_hist[0] + HIST*c, &reward[0] + c);
		}
	}

	if (action_hist.size() && (~view_options&VIEW_NO_HUD)) {
		for (int c=0; c<(int)action.size(); ++c) {
			int x = MARGIN;
			int y = top + 26*c;
			QRect r(x, y, HIST, 20);
			if ( (area & r) != r ) continue;
			if ( r.bottom() > top_reward ) continue;
			drawhist(p, "act", c, r, &action_hist[0] + HIST*c, &action[0] + c);
		}
	}

	render_viewport->hud_update_finish();
}

void Viz::drawhist(
	QPainter& p,
	const char* label, int bracketn,
	const QRect& r,
	const float* sensors_hist,
	const float* sensors)
{
	QColor bg = Qt::black;
	bg.setAlpha(100);
	p.setCompositionMode(QPainter::CompositionMode_Source);
	p.fillRect(r, bg);

	int mid_y = r.center().y();
	float half_height = r.height()*0.5f;
	QImage& img = render_viewport->hud_image;

	for (int h=0; h<HIST; h++) {
		int x = r.left() + h;
		float clamp = sensors_hist[h]*0.5;
		if (clamp > 1.0f) clamp = 1.0f;
		if (clamp < -1.0f) clamp = -1.0f;
		int mag = int( 0.5 + mid_y - clamp*half_height );
		for (int y=mid_y; y<mag; y++)
			img.setPixel(x, y, 0xFFFF8080);
		for (int y=mid_y; y>=mag; y--)
			img.setPixel(x, y, 0xFF80FF80);
	}

	char buf[100];
	snprintf(buf, sizeof(buf), "%s[%02i] = %+0.2f", label, bracketn, sensors[0]);
	p.setCompositionMode(QPainter::CompositionMode_SourceOver);
	p.setOpacity(0.7);
	p.drawText(r, QString::fromUtf8(buf), Qt::AlignLeft|Qt::AlignTop);

	render_viewport->hud_update(r);
}

void Viz::history_advance(bool only_ensure_correct_sizes)
{
	int obs_hist_size = obs.size()*HIST;
	if (obs_hist_size != (int)obs_hist.size())
		obs_hist.assign(obs_hist_size, 0);
	if (obs_hist_size > 0 && !only_ensure_correct_sizes) {
		memmove(&obs_hist[0], &obs_hist[1], sizeof(obs_hist[0])*(obs_hist_size-1));
		for (int c=0; c<(int)obs.size(); c++)
			obs_hist[HIST*c+(HIST-1)] = obs[c];
	}

	int action_hist_size = action.size()*HIST;
	if (action_hist_size != (int)action_hist.size())
		action_hist.assign(action_hist_size, 0);
	if (action_hist_size > 0 && !only_ensure_correct_sizes) {
		memmove(&action_hist[0], &action_hist[1], sizeof(action_hist[0])*(action_hist_size-1));
		for (int c=0; c<(int)action.size(); c++)
			action_hist[HIST*c+(HIST-1)] = action[c];
	}

	int reward_hist_size = reward.size()*HIST;
	if (reward_hist_size != (int)reward_hist.size())
		reward_hist.assign(reward_hist_size, 0);
	if (reward_hist_size > 0 && !only_ensure_correct_sizes) {
		memmove(&reward_hist[0], &reward_hist[1], sizeof(reward_hist[0])*(reward_hist_size-1));
		for (int c=0; c<(int)reward.size(); c++)
			reward_hist[HIST*c+(HIST-1)] = reward[c];
	}
}

void ConsoleMessage::render(uint32_t color, int width)
{
	ts = QDateTime::currentMSecsSinceEpoch();
	if (msg_text.isEmpty()) {
		msg_image = QImage();
		return;
	}
	QFont font("Courier", 22);
	font.setBold(true);
	QFontMetrics fm(font);
	QSize size = fm.size(0, msg_text);
	if (width) size.rwidth() = width;
	const int MARGIN = 10;
	size.rwidth() += MARGIN*2;
	size.rwidth() += 16;
	size.rwidth() &= ~0xF;
	msg_image = QImage(size, QImage::Format_ARGB32);
	msg_image.fill(QColor(255,255,255,100));
	QPainter p(&msg_image);
	p.setPen(QRgb(color));
	p.setFont(font);
	p.drawText(QRect(MARGIN, 0, size.width(), size.height()), Qt::AlignLeft, msg_text);
}

void Viz::test_window_print(const std::string& msg_)
{
	ConsoleMessage msg;
	msg.msg_text  = QString::fromUtf8(msg_.c_str());
	msg.render(0x000000);
	console.push_front(msg);
	if (console.size() > 100) console.pop_back();
}

void Viz::test_window_billboard(const btVector3& pos, const std::string& msg_, uint32_t color)
{
	ConsoleMessage msg;
	msg.msg_text = QString::fromUtf8(msg_.c_str());
	msg.pos = pos;
	msg.render(color);
	billboards.push_front(msg);
	if (billboards.size() > 10) billboards.pop_back();
}

void Viz::test_window_big_caption(const std::string& msg_)
{
	QString t = QString::fromUtf8(msg_.c_str());
	if (t==caption.msg_text) return;
	caption.msg_text = t;
	if (render_viewport)
		caption.render(0x880000, render_viewport->W);
}
