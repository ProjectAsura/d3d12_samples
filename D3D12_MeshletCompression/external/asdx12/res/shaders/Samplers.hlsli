//-----------------------------------------------------------------------------
// File : Samplers.hlsli
// Desc : Common Samplers.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------

#ifndef SAMPLERS_HLSLI
#define SAMPLERS_HLSLI

SamplerState            PointWrap           : register(s0);
SamplerState            PointClamp          : register(s1);
SamplerState            PointMirror         : register(s2);
SamplerState            LinearWrap          : register(s3);
SamplerState            LinearClamp         : register(s4);
SamplerState            LinearMirror        : register(s5);
SamplerState            AnisotropicWrap     : register(s6); // MaxAnisotropy = 16.
SamplerState            AnisotropicClamp    : register(s7); // MaxAnisotorpy = 16.
SamplerState            AnisotropicMirror   : register(s8); // MaxAnisotropy = 16.
SamplerComparisonState  LessEqualSampler    : register(s9); // LinearBorder Color(0, 0, 0, 1).
SamplerComparisonState  GreaterSampler      : register(s10);// LinearBorder Color(0, 0, 0, 1).

#endif//SAMPLERS_HLSLI
