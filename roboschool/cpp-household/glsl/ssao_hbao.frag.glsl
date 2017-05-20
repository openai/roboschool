//#line 2 "ssao_hbao.frag.glsl"
// no #version here, to insert #define's in C++ code

// The pragma below is critical for optimal performance
// in this fragment shader to let the shader compiler
// fully optimize the maths and batch the texture fetches
// optimally
#pragma optionNV(unroll all)

#define AO_RANDOMTEX_SIZE 4

uniform float   RadiusToScreen; // radius
uniform float   R2;             // 1/radius
uniform float   NegInvR2;       // radius * radius
uniform float   NDotVBias;

uniform vec2    InvFullResolution;
uniform vec2    InvQuarterResolution;

uniform float   AOMultiplier;
uniform float   PowExponent;

uniform vec4    projInfo;
uniform vec2    projScale;
uniform int     projOrtho;

uniform vec4    float2Offsets[AO_RANDOMTEX_SIZE*AO_RANDOMTEX_SIZE];
uniform vec4    jitters[AO_RANDOMTEX_SIZE*AO_RANDOMTEX_SIZE];


#ifndef AO_DEINTERLEAVED
#define AO_DEINTERLEAVED 1
#endif

#ifndef AO_BLUR
#define AO_BLUR 1
#endif

#ifndef AO_LAYERED
#define AO_LAYERED 1
#endif

#define M_PI 3.14159265f

// tweakables
const float  NUM_STEPS = 4;
const float  NUM_DIRECTIONS = 8; // texRandom/g_Jitter initialization depends on this

#if AO_DEINTERLEAVED

#if AO_LAYERED
  vec2 g_Float2Offset = float2Offsets[gl_PrimitiveID].xy;
  vec4 g_Jitter       = jitters[gl_PrimitiveID];
  
  layout(binding=0) uniform sampler2DArray texLinearDepth;
  layout(binding=1) uniform sampler2D texViewNormal;

  vec3 getQuarterCoord(vec2 UV){
    return vec3(UV,float(gl_PrimitiveID));
  }
  #if AO_LAYERED == 1
  
    #if AO_BLUR
      layout(binding=0,rg16f) uniform image2DArray imgOutput;
    #else
      layout(binding=0,r8) uniform image2DArray imgOutput;
    #endif

    void outputColor(vec4 color) {
      imageStore(imgOutput, ivec3(ivec2(gl_FragCoord.xy),gl_PrimitiveID), color);
    }
  #else
    layout(location=0,index=0) out vec4 out_Color;
  
    void outputColor(vec4 color) {
      out_Color = color;
    }
  #endif
#else
  layout(location=0) uniform vec2 g_Float2Offset;
  layout(location=1) uniform vec4 g_Jitter;
  
  layout(binding=0) uniform sampler2D texLinearDepth;
  layout(binding=1) uniform sampler2D texViewNormal;
  
  vec2 getQuarterCoord(vec2 UV){
    return UV;
  }

  layout(location=0,index=0) out vec4 out_Color;
  
  void outputColor(vec4 color) {
    out_Color = color;
  }
#endif
  
#else  // not AO_DEINTERLEAVED
  uniform sampler2D texLinearDepth;
  uniform sampler2D texRandom;
  
  layout(location=0,index=0) out vec4 out_Color;
  //out vec4 out_Color;
  
  void outputColor(vec4 color) {
    out_Color = color;
  }
#endif

in vec2 texCoord;

//----------------------------------------------------------------------------------

vec3 UVToView(vec2 uv, float eye_z)
{
  return vec3((uv * projInfo.xy + projInfo.zw) * (projOrtho != 0 ? 1. : eye_z), eye_z);
}

#if AO_DEINTERLEAVED

vec3 FetchQuarterResViewPos(vec2 UV)
{
  float ViewDepth = textureLod(texLinearDepth,getQuarterCoord(UV),0).x;
  return UVToView(UV, ViewDepth);
}

#else // not AO_DEINTERLEAVED

vec3 FetchViewPos(vec2 UV)
{
  float ViewDepth = textureLod(texLinearDepth, UV, 0).x;
  return UVToView(UV, ViewDepth);
}

vec3 MinDiff(vec3 P, vec3 Pr, vec3 Pl)
{
  vec3 V1 = Pr - P;
  vec3 V2 = P - Pl;
  return (dot(V1,V1) < dot(V2,V2)) ? V1 : V2;
}

vec3 ReconstructNormal(vec2 UV, vec3 P)
{
  vec3 Pr = FetchViewPos(UV + vec2(InvFullResolution.x, 0));
  vec3 Pl = FetchViewPos(UV + vec2(-InvFullResolution.x, 0));
  vec3 Pt = FetchViewPos(UV + vec2(0, InvFullResolution.y));
  vec3 Pb = FetchViewPos(UV + vec2(0, -InvFullResolution.y));
  return normalize(cross(MinDiff(P, Pr, Pl), MinDiff(P, Pt, Pb)));
}

#endif //AO_DEINTERLEAVED

//----------------------------------------------------------------------------------
float Falloff(float DistanceSquare)
{
  // 1 scalar mad instruction
  return DistanceSquare * NegInvR2 + 1.0;
}

//----------------------------------------------------------------------------------
// P = view-space position at the kernel center
// N = view-space normal at the kernel center
// S = view-space position of the current sample
//----------------------------------------------------------------------------------
float ComputeAO(vec3 P, vec3 N, vec3 S)
{
  vec3 V = S - P;
  float VdotV = dot(V, V);
  float NdotV = dot(N, V) * 1.0/sqrt(VdotV);

  // Use saturate(x) instead of max(x,0.f) because that is faster on Kepler
  return clamp(NdotV - NDotVBias,0,1) * clamp(Falloff(VdotV),0,1);
}

//----------------------------------------------------------------------------------
vec2 RotateDirection(vec2 Dir, vec2 CosSin)
{
  return vec2(Dir.x*CosSin.x - Dir.y*CosSin.y,
              Dir.x*CosSin.y + Dir.y*CosSin.x);
}

//----------------------------------------------------------------------------------
vec4 GetJitter()
{
#if AO_DEINTERLEAVED
  // Get the current jitter vector from the per-pass constant buffer
  return g_Jitter;
#else
  // (cos(Alpha),sin(Alpha),rand1,rand2)
  return textureLod( texRandom, (gl_FragCoord.xy / AO_RANDOMTEX_SIZE), 0);
#endif
}

//----------------------------------------------------------------------------------
float ComputeCoarseAO(vec2 FullResUV, float RadiusPixels, vec4 Rand, vec3 ViewPosition, vec3 ViewNormal)
{
#if AO_DEINTERLEAVED
  RadiusPixels /= 4.0;
#endif

  // Divide by NUM_STEPS+1 so that the farthest samples are not fully attenuated
  float StepSizePixels = RadiusPixels / (NUM_STEPS + 1);

  const float Alpha = 2.0 * M_PI / NUM_DIRECTIONS;
  float AO = 0;

  for (float DirectionIndex = 0; DirectionIndex < NUM_DIRECTIONS; ++DirectionIndex)
  {
    float Angle = Alpha * DirectionIndex;

    // Compute normalized 2D direction
    vec2 Direction = RotateDirection(vec2(cos(Angle), sin(Angle)), Rand.xy);

    // Jitter starting sample within the first step
    float RayPixels = (Rand.z * StepSizePixels + 1.0);

    for (float StepIndex = 0; StepIndex < NUM_STEPS; ++StepIndex)
    {
#if AO_DEINTERLEAVED
      vec2 SnappedUV = round(RayPixels * Direction) * InvQuarterResolution + FullResUV;
      vec3 S = FetchQuarterResViewPos(SnappedUV);
#else
      vec2 SnappedUV = round(RayPixels * Direction) * InvFullResolution + FullResUV;
      vec3 S = FetchViewPos(SnappedUV);
#endif

      RayPixels += StepSizePixels;

      AO += ComputeAO(ViewPosition, ViewNormal, S);
    }
  }

  AO *= AOMultiplier / (NUM_DIRECTIONS * NUM_STEPS);
  return clamp(1.0 - AO * 2.0,0,1);
}

//----------------------------------------------------------------------------------
void main()
{
#if AO_DEINTERLEAVED
  vec2 base = floor(gl_FragCoord.xy) * 4.0 + g_Float2Offset;
  vec2 uv = base * (InvQuarterResolution / 4.0);

  vec3 ViewPosition = FetchQuarterResViewPos(uv);

  vec4 NormalAndAO =  texelFetch( texViewNormal, ivec2(base), 0);
  vec3 ViewNormal =  -(NormalAndAO.xyz * 2.0 - 1.0);
#else
  vec2 uv = texCoord;
  vec3 ViewPosition = FetchViewPos(uv);

  // Reconstruct view-space normal from nearest neighbors
  vec3 ViewNormal = -ReconstructNormal(uv, ViewPosition);
#endif

  // Compute projection of disk of radius R into screen space
  float RadiusPixels = RadiusToScreen / (projOrtho != 0 ? 1.0 : ViewPosition.z);

  // Get jitter vector for the current full-res pixel
  vec4 Rand = GetJitter();

  float AO = ComputeCoarseAO(uv, RadiusPixels, Rand, ViewPosition, ViewNormal);

#if AO_BLUR
  outputColor(vec4(pow(AO, PowExponent), ViewPosition.z, 0, 0));
#else
  outputColor(vec4(pow(AO, PowExponent)));
#endif
  
}


/*
Based on DeinterleavedTexturing sample by Louis Bavoil
https://github.com/NVIDIAGameWorks/D3DSamples/tree/master/samples/DeinterleavedTexturing
*/

/*-----------------------------------------------------------------------
  Copyright (c) 2014-2015, NVIDIA. All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   * Neither the name of its contributors may be used to endorse 
     or promote products derived from this software without specific
     prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
  OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------*/
