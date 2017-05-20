#define GL_GLEXT_PROTOTYPES
#include "render-simple.h"
#include <QtOpenGL/QtOpenGL>
#include <QtOpenGL/QGLFramebufferObject>

#include "glsl/nv_math.h"
#include "glsl/nv_math_types.h"
#include "glsl/nv_math_glsltypes.h"
#include "glsl/common.h"
#ifdef __APPLE__
#include <gl3.h>
#include <gl3ext.h>
#endif

extern std::string glsl_path;

namespace SimpleRender {

using namespace Household;
using namespace nv_math;

void ContextViewport::_depthlinear_init()
{
	if (samples > 1) {
//		tex_color.reset(new Texture());
//		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, tex_color->handle);
//		glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA8, W, H, GL_FALSE);
//		glBindTexture (GL_TEXTURE_2D_MULTISAMPLE, 0);
//		tex_depthstencil.reset(new Texture());
//		glBindTexture (GL_TEXTURE_2D_MULTISAMPLE, tex_depthstencil->handle);
//		glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_DEPTH24_STENCIL8, W, H, GL_FALSE);
//		glBindTexture (GL_TEXTURE_2D_MULTISAMPLE, 0);
	} else {
		tex_color.reset(new Texture());
		glBindTexture(GL_TEXTURE_2D, tex_color->handle);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, W, H);
		glBindTexture(GL_TEXTURE_2D, 0);
		tex_depthstencil.reset(new Texture());
		glBindTexture(GL_TEXTURE_2D, tex_depthstencil->handle);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, W, H);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	tex_depthlinear.reset(new Texture());
	glBindTexture(GL_TEXTURE_2D, tex_depthlinear->handle);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, W, H);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
	fbuf_depthlinear.reset(new Framebuffer());
	glBindFramebuffer(GL_FRAMEBUFFER, fbuf_depthlinear->handle);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex_depthlinear->handle, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ContextViewport::_depthlinear_paint(int sample_idx)
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbuf_depthlinear->handle); // to framebuf
	if (samples > 1) {
//		glUseProgram(cx->program_depth_linearize_msaa->programId());
//		glUniform4f(0, near*far, near-far, far, ortho ? 0.0f : 1.0f);
//		glUniform1i(1, sample_idx);
//		glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D_MULTISAMPLE, tex_depthstencil->handle);
//		glDrawArrays(GL_TRIANGLES, 0, 3);
//		glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D_MULTISAMPLE, 0);
//		assert(0);
	} else {
		glUseProgram(cx->program_depth_linearize->programId());
		glUniform4f(cx->location_clipInfo, near*far, near-far, far, ortho ? 0.0f : 1.0f);
		//location_inputTexture
		// MAC
		glBindVertexArray(cx->ruler_vao->handle);
		glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, tex_depthstencil->handle); // and to depthstencil
		glDrawArrays(GL_TRIANGLES, 0, 3);
		// MAC
		glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, 0);
	}
	glUseProgram(0);
	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

struct UsefulStuff {
	ssao::HBAOData hbaoUbo; // from common.h
	vec4f hbaoRandom[HBAO_RANDOM_ELEMENTS * MAX_SAMPLES];
};

bool Context::_hbao_init()
{
	program_depth_linearize = load_program("fullscreen_triangle.vert.glsl", "", "ssao_depthlinearize.frag.glsl", 0, 0, "#version 410\n");
	if (!program_depth_linearize->log().isEmpty()) {
		fprintf(stderr, "Roboschool built-in render compiled with shadows, but SSAO shaders didn't load (1)\n");
		return false;
	}
	program_depth_linearize->link(); // always returns true :(
	location_clipInfo = program_depth_linearize->uniformLocation("clipInfo");

	program_hbao_calc = load_program("fullscreen_triangle.vert.glsl", "", "ssao_hbao.frag.glsl", 0, 0, "#version 410\n#define AO_DEINTERLEAVED 0\n#define AO_BLUR 0\n#define AO_LAYERED 0\n");
	if (!program_hbao_calc->log().isEmpty()) {
		fprintf(stderr, "Roboschool built-in render compiled with shadows, but SSAO shaders didn't load (2)\n");
		return false;
	}
	program_hbao_calc->link();
	location_RadiusToScreen = program_hbao_calc->uniformLocation("RadiusToScreen");
	location_R2 = program_hbao_calc->uniformLocation("R2");
	location_NegInvR2 = program_hbao_calc->uniformLocation("NegInvR2");
	location_NDotVBias = program_hbao_calc->uniformLocation("NDotVBias");
	location_InvFullResolution = program_hbao_calc->uniformLocation("InvFullResolution");
	location_InvQuarterResolution = program_hbao_calc->uniformLocation("InvQuarterResolution");
	location_AOMultiplier = program_hbao_calc->uniformLocation("AOMultiplier");
	location_PowExponent = program_hbao_calc->uniformLocation("PowExponent");
	location_projInfo = program_hbao_calc->uniformLocation("projInfo");
	location_projScale = program_hbao_calc->uniformLocation("projScale");
	location_projOrtho = program_hbao_calc->uniformLocation("projOrtho");
	location_float2Offsets = program_hbao_calc->uniformLocation("float2Offsets");
	location_jitters = program_hbao_calc->uniformLocation("jitters");

	location_texLinearDepth = program_hbao_calc->uniformLocation("texLinearDepth");
	location_texRandom = program_hbao_calc->uniformLocation("texRandom");

	useful.reset(new UsefulStuff);

	float numDir = 8; // keep in sync to glsl
	signed short hbaoRandomShort[HBAO_RANDOM_ELEMENTS*MAX_SAMPLES*4];
	for(int i=0; i<HBAO_RANDOM_ELEMENTS*MAX_SAMPLES; i++) {
		float r1 = static_cast<float>(rand()) / static_cast <float>(RAND_MAX);;
		float r2 = static_cast<float>(rand()) / static_cast <float>(RAND_MAX);;
		// Use random rotation angles in [0,2PI/NUM_DIRECTIONS)
		float angle = 2.f * nv_pi * r1 / numDir;
		useful->hbaoRandom[i].x = cosf(angle);
		useful->hbaoRandom[i].y = sinf(angle);
		useful->hbaoRandom[i].z = r2;
		useful->hbaoRandom[i].w = 0;
#define SCALE ((1<<15))
		hbaoRandomShort[i*4+0] = (signed short)(SCALE*useful->hbaoRandom[i].x);
		hbaoRandomShort[i*4+1] = (signed short)(SCALE*useful->hbaoRandom[i].y);
		hbaoRandomShort[i*4+2] = (signed short)(SCALE*useful->hbaoRandom[i].z);
		hbaoRandomShort[i*4+3] = (signed short)(SCALE*useful->hbaoRandom[i].w);
#undef SCALE
	}

	hbao_random.reset(new Texture());
	glBindTexture(GL_TEXTURE_2D_ARRAY, hbao_random->handle);
	glTexStorage3D (GL_TEXTURE_2D_ARRAY,1,GL_RGBA16_SNORM,HBAO_RANDOM_SIZE,HBAO_RANDOM_SIZE,MAX_SAMPLES);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY,0,0,0,0, HBAO_RANDOM_SIZE,HBAO_RANDOM_SIZE,MAX_SAMPLES,GL_RGBA,GL_SHORT,hbaoRandomShort);
	glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D_ARRAY,0);

	for (int i=0; i<MAX_SAMPLES; i++) {
		hbao_randomview[i].reset(new Texture());
		glTextureView(hbao_randomview[i]->handle, GL_TEXTURE_2D, hbao_random->handle, GL_RGBA16_SNORM, 0, 1, i, 1);
		glBindTexture(GL_TEXTURE_2D, hbao_randomview[i]->handle);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	return true;
}

void ContextViewport::_hbao_prepare(const float* proj_matrix)
{
#if 0
	float p1, p2, p3, testy;
	testy = 1.0;
	FILE* f = fopen("ssao.params", "rt");
	if (f) {
		int r = fscanf(f, "%f %f %f %f", &p1, &p2, &p3, &testy);
		fclose(f);
		if (r==4) {
			ssao_radius = p1;
			ssao_intensity = p2;
			ssao_bias = p3;
		}
	}
	cx->useful->hbaoUbo._pad0[0] = testy;
#endif

	const float* P = proj_matrix;

	cx->useful->hbaoUbo.projOrtho = ortho;
	float projScale;
	if (ortho) {
		cx->useful->hbaoUbo.projInfo[0] =  20.0f / ( P[4*0+0]);      // ((x) * R - L)
		cx->useful->hbaoUbo.projInfo[1] =  20.0f / ( P[4*1+1]);      // ((y) * T - B)
		cx->useful->hbaoUbo.projInfo[2] = -( 1.0f + P[4*3+0]) / P[4*0+0]; // L
		cx->useful->hbaoUbo.projInfo[3] = -( 1.0f - P[4*3+1]) / P[4*1+1]; // B
		projScale = float(side) / cx->useful->hbaoUbo.projInfo[1];
	} else {
		cx->useful->hbaoUbo.projInfo[0] =  2.0f / (P[4*0+0]);       // (x) * (R - L)/N
		cx->useful->hbaoUbo.projInfo[1] =  2.0f / (P[4*1+1]);       // (y) * (T - B)/N
		cx->useful->hbaoUbo.projInfo[2] = -( 1.0f - P[4*2+0]) / P[4*0+0]; // L/N
		cx->useful->hbaoUbo.projInfo[3] = -( 1.0f + P[4*2+1]) / P[4*1+1]; // B/N
		projScale = float(side) / (tanf(hfov * nv_to_rad * 0.5f) * 2.0f);
	}

	// radius
	float R = ssao_radius;
	cx->useful->hbaoUbo.R2 = R * R;
	cx->useful->hbaoUbo.NegInvR2 = -1.0f / cx->useful->hbaoUbo.R2;
	cx->useful->hbaoUbo.RadiusToScreen = R * 0.5f * projScale;

	// ao
	cx->useful->hbaoUbo.PowExponent = std::max(ssao_intensity, 0.0f);
	cx->useful->hbaoUbo.NDotVBias = std::min(std::max(0.0f, ssao_bias), 1.0f);
	cx->useful->hbaoUbo.AOMultiplier = 1.0f / (1.0f - cx->useful->hbaoUbo.NDotVBias);

	// resolution
	int quarterW = ((W+3)/4);
	int quarterH = ((H+3)/4);

	cx->useful->hbaoUbo.InvQuarterResolution[0] = 1.0f/float(quarterW);
	cx->useful->hbaoUbo.InvQuarterResolution[1] = 1.0f/float(quarterH);
	cx->useful->hbaoUbo.InvFullResolution[0] = 1.0f/float(W);
	cx->useful->hbaoUbo.InvFullResolution[1] = 1.0f/float(H);

	for (int i = 0; i < HBAO_RANDOM_ELEMENTS; i++){
		cx->useful->hbaoUbo.float2Offsets[4*i+0] = float(i % 4) + 0.5f;
		cx->useful->hbaoUbo.float2Offsets[4*i+1] = float(i / 4) + 0.5f;
		cx->useful->hbaoUbo.float2Offsets[4*i+2] = 0;
		cx->useful->hbaoUbo.float2Offsets[4*i+3] = 0;
		cx->useful->hbaoUbo.jitters[4*i+0] = cx->useful->hbaoRandom[i][0];
		cx->useful->hbaoUbo.jitters[4*i+1] = cx->useful->hbaoRandom[i][1];
		cx->useful->hbaoUbo.jitters[4*i+2] = cx->useful->hbaoRandom[i][2];
		cx->useful->hbaoUbo.jitters[4*i+3] = cx->useful->hbaoRandom[i][3];
	}
}

void ContextViewport::_ssao_run(int sampleIdx)
{
	if (blur) {
		glBindFramebuffer(GL_FRAMEBUFFER, fbuf_hbao_calc->handle);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
	} else {
		glBindFramebuffer(GL_FRAMEBUFFER, fbuf_scene->handle);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ZERO,GL_SRC_COLOR);
		if (samples > 1){
			glEnable(GL_SAMPLE_MASK);
			glSampleMaski(0, 1<<sampleIdx);
		}
	}

	//glUseProgram( (USE_AO_SPECIALBLUR && blur) ?
	//	cx->program_calc_blur->programId() :
	//	cx->program_hbao_calc->programId() );

	cx->program_hbao_calc->bind();
	cx->program_hbao_calc->setUniformValue(cx->location_RadiusToScreen, cx->useful->hbaoUbo.RadiusToScreen);
	cx->program_hbao_calc->setUniformValue(cx->location_R2, cx->useful->hbaoUbo.R2);
	cx->program_hbao_calc->setUniformValue(cx->location_NegInvR2, cx->useful->hbaoUbo.NegInvR2);
	cx->program_hbao_calc->setUniformValue(cx->location_NDotVBias, cx->useful->hbaoUbo.NDotVBias);
	cx->program_hbao_calc->setUniformValue(cx->location_InvFullResolution, cx->useful->hbaoUbo.InvFullResolution[0], cx->useful->hbaoUbo.InvFullResolution[1]);
	cx->program_hbao_calc->setUniformValue(cx->location_InvQuarterResolution, cx->useful->hbaoUbo.InvQuarterResolution[0], cx->useful->hbaoUbo.InvQuarterResolution[1]);
	cx->program_hbao_calc->setUniformValue(cx->location_AOMultiplier, cx->useful->hbaoUbo.AOMultiplier);
	cx->program_hbao_calc->setUniformValue(cx->location_PowExponent, cx->useful->hbaoUbo.PowExponent);
	cx->program_hbao_calc->setUniformValue(cx->location_projInfo,
		cx->useful->hbaoUbo.projInfo[0],
		cx->useful->hbaoUbo.projInfo[1],
		cx->useful->hbaoUbo.projInfo[2],
		cx->useful->hbaoUbo.projInfo[3]);
	cx->program_hbao_calc->setUniformValue(cx->location_projScale, cx->useful->hbaoUbo.projScale[0], cx->useful->hbaoUbo.projScale[1]);
	cx->program_hbao_calc->setUniformValue(cx->location_projOrtho, cx->useful->hbaoUbo.projOrtho);
	// layered etc
	glUniform4fv(cx->location_float2Offsets, sizeof(cx->useful->hbaoUbo.float2Offsets)/sizeof(cx->useful->hbaoUbo.float2Offsets[0])/4, cx->useful->hbaoUbo.float2Offsets);
	glUniform4fv(cx->location_jitters, sizeof(cx->useful->hbaoUbo.jitters)/sizeof(cx->useful->hbaoUbo.jitters[0])/4, cx->useful->hbaoUbo.jitters);
	glUniform1i(cx->location_texLinearDepth, 1); // texture unit 0
	glUniform1i(cx->location_texRandom, 0); // texture unit 1

	glBindVertexArray(cx->ruler_vao->handle);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cx->hbao_randomview[sampleIdx]->handle);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, tex_depthlinear->handle);

	glDrawArrays(GL_TRIANGLES,0,3);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);

	if (blur) {
		//drawHbaoBlur(projection,width,height,sampleIdx);
	}

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_SAMPLE_MASK);
	glSampleMaski(0, ~0);

	cx->program_hbao_calc->release();
}

} // namespace
