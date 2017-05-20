//#line 2 "glsl/common.h"

#define AO_RANDOMTEX_SIZE 4

#ifdef __cplusplus
namespace ssao
{
using namespace nv_math;
#endif

struct HBAOData {
  float   RadiusToScreen; // radius
  float   R2;             // 1/radius
  float   NegInvR2;       // radius * radius
  float   NDotVBias;
 
  float InvFullResolution[2];
  float InvQuarterResolution[2];
  
  float   AOMultiplier;
  float   PowExponent;
  
  float   projInfo[4];
  float   projScale[2];
  int     projOrtho;
  
  float  float2Offsets[4*AO_RANDOMTEX_SIZE*AO_RANDOMTEX_SIZE];  // 4 == vec4
  float  jitters[4*AO_RANDOMTEX_SIZE*AO_RANDOMTEX_SIZE];
};

#ifdef __cplusplus
}
#endif

/*-----------------------------------------------------------------------
  Copyright (c) 2014, NVIDIA. All rights reserved.

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
