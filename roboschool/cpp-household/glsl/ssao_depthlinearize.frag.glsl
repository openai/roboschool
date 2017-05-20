//#line 2 "ssao_depthlinearize.frag.glsl"
// no #version here, to insert #define's in C++ code

#ifndef DEPTHLINEARIZE_MSAA
#define DEPTHLINEARIZE_MSAA 0
#endif

uniform vec4 clipInfo; // z_n * z_f,  z_n - z_f,  z_f, perspective = 1 : 0

//#if DEPTHLINEARIZE_MSAA
//layout(location=1) uniform int sampleIndex;
//layout(binding=0)  uniform sampler2DMS inputTexture;
//#else
uniform sampler2D inputTexture;
//#endif

out highp vec4 out_Color;

float reconstructCSZ(float d, vec4 clipInfo) {
  if (clipInfo[3] != 0) {
    return (clipInfo[0] / (clipInfo[1] * d + clipInfo[2]));
  }
  else {
    return (clipInfo[1]+clipInfo[2] - d * clipInfo[1]);
  }
}
/*
    if (in_perspective == 1.0) // perspective
    {
        ze = (zNear * zFar) / (zFar - zb * (zFar - zNear)); 
    }
    else // orthographic proj 
    {
        ze  = zNear + zb  * (zFar - zNear);
    }
*/
void main() {
//#if DEPTHLINEARIZE_MSAA
//  float depth = texelFetch(inputTexture, ivec2(gl_FragCoord.xy), sampleIndex).x;
//#else
    float depth = texelFetch(inputTexture, ivec2(gl_FragCoord.xy), 0).x;
//#endif
    out_Color = vec4(reconstructCSZ(depth, clipInfo), 0,0,1);
}

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
