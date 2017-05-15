// Copyright (c) 2012 NVIDIA Corporation. All rights reserved.
//
// TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
// *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
// OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, NONINFRINGEMENT,IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA 
// OR ITS SUPPLIERS BE  LIABLE  FOR  ANY  DIRECT, SPECIAL,  INCIDENTAL,  INDIRECT,  OR  
// CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS 
// OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY 
// OTHER PECUNIARY LOSS) ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE, 
// EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
//
// Please direct any bugs or questions to SDKFeedback@nvidia.com

#ifndef NV_SHADER_TYPES_H
#define NV_SHADER_TYPES_H

#include "NvFoundation.h"
#include "nv_math_types.h"

namespace nv_math {

#if defined(__amd64__) || defined(__x86_64__) || defined(_M_X64) || defined(__AMD64__)
  // Matrices, must align to 4 vector (16 bytes)
  NV_ALIGN(16, typedef mat4f mat4);

  // vectors, 4-tuples and 3-tuples must align to 16 bytes
  //  2-vectors must align to 8 bytes
  NV_ALIGN(16, typedef vec4f vec4 );
  NV_ALIGN(16, typedef vec3f vec3 );
  NV_ALIGN(8 , typedef vec2f vec2 );

  NV_ALIGN(16, typedef vec4i ivec4 );
  NV_ALIGN(16, typedef vec3i ivec3 );
  NV_ALIGN(8 , typedef vec2i ivec2 );

  NV_ALIGN(16, typedef vec4ui uvec4 );
  NV_ALIGN(16, typedef vec3ui uvec3 );
  NV_ALIGN(8,  typedef vec2ui uvec2 );
#else
  // Matrices, must align to 4 vector (16 bytes)
  typedef  mat4f mat4;

  // vectors, 4-tuples and 3-tuples must align to 16 bytes
  //  2-vectors must align to 8 bytes
  typedef  vec4f vec4;
  typedef  vec3f vec3;
  typedef  vec2f vec2;

  typedef  vec4i ivec4;
  typedef  vec3i ivec3;
  typedef  vec2i ivec2;

  typedef  vec4ui uvec4;
  typedef  vec3ui uvec3;
  typedef  vec2ui uvec2;
#endif

//class to make uint look like bool to make GLSL packing rules happy
struct boolClass
{
	unsigned int _rep;

	boolClass() : _rep(false) {}
	boolClass( bool b) : _rep(b) {}
	operator bool() { return _rep == 0 ? false : true; }
	boolClass& operator=( bool b) { _rep = b; return *this; }
};

//NVP_ALIGN_V(4, typedef boolClass bool32);

}

#endif
