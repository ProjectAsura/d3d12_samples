//-----------------------------------------------------------------------------
// File : WaveHelper.hlsli
// Desc : Wave Intrinsics Helper Functions.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#ifndef ASDX_WAVE_HELPER_HLSLI
#define ASDX_WAVE_HELPER_HLSLI

//-----------------------------------------------------------------------------
//      指定された値を分類分けします.
//-----------------------------------------------------------------------------
uint WaveCompactValue(uint checkValue)
{
    // レーンのユニークなコンパクションマスク.
    uint4 mask;

    // すべてのアクティブレーンが取り除かれるまでループ.
    for (;;)
    {
        // アクティブレーンの最初の値を読み取る.
        uint firstValue = WaveReadLaneFirst(checkValue);
 
        // mask は残っているアクティブレーンに対してのみ更新される.
        mask = WaveActiveBallot(firstValue == checkValue);
 
        // firstValue を持つすべてのレーンを次のイテレーションから除外する.
        if (firstValue == checkValue)
             break;
    }
    // この時点で、マスクの各レーンは、同じ値を持つ他のレーンのすべてのビットマスクを含んでいなければならない.
    uint index = WavePrefixSum(mask); // これはレーンごとに異なるマスクで独立して行われる.
    return index;
}

//-----------------------------------------------------------------------------
//      最後のレーン番号を取得します.
//-----------------------------------------------------------------------------
uint WaveGetLastLaneIndex()
{
    uint4 ballot = WaveActiveBallot(true);
    uint4 bits   = firstbithigh(ballot); // Returns -1 (0xFFFFFFFF) if no bits set.

    // RDN2にて,firstbithigh() がベクトル化されるケースがあるため，ここでスカラー化を保証する.
    bits = WaveReadLaneFirst(bits);

#if __HLSL_VERSION >= 2021
    bits = select(bits == 0xFFFFFFFF, 0, bits + uint4(0, 32, 64, 96));
#else
    bits = (bits == 0xFFFFFFFF) ? 0 : bits + uint4(0, 32, 64, 96);
#endif
 
    return max(max(bits.x, bits.y), max(bits.z, bits.w));
}

#if __HLSL_VERSION >= 2021
//-----------------------------------------------------------------------------
//      最後のレーンの値を取得します.
//-----------------------------------------------------------------------------
template<typename T>
T WaveReadLaneLast(T val)
{
    uint lastLane = WaveGetLastLaneIndex();
    return WaveReadLaneAt(val, lastLane);
}
#else
int WaveReadLaneLast(int val)
{
    uint lastLane = WaveGetLastLaneIndex();
    return WaveReadLaneAt(val, lastLane);
}
int2 WaveReadLaneLast(int2 val)
{
    uint lastLane = WaveGetLastLaneIndex();
    return WaveReadLaneAt(val, lastLane);
}
int3 WaveReadLaneLast(int3 val)
{
    uint lastLane = WaveGetLastLaneIndex();
    return WaveReadLaneAt(val, lastLane);
}
int4 WaveReadLaneLast(int4 val)
{
    uint lastLane = WaveGetLastLaneIndex();
    return WaveReadLaneAt(val, lastLane);
}
uint WaveReadLaneLast(uint val)
{
    uint lastLane = WaveGetLastLaneIndex();
    return WaveReadLaneAt(val, lastLane);
}
uint2 WaveReadLaneLast(uint2 val)
{
    uint lastLane = WaveGetLastLaneIndex();
    return WaveReadLaneAt(val, lastLane);
}
uint3 WaveReadLaneLast(uint3 val)
{
    uint lastLane = WaveGetLastLaneIndex();
    return WaveReadLaneAt(val, lastLane);
}
uint4 WaveReadLaneLast(uint4 val)
{
    uint lastLane = WaveGetLastLaneIndex();
    return WaveReadLaneAt(val, lastLane);
}
float WaveReadLaneLast(float val)
{
    uint lastLane = WaveGetLastLaneIndex();
    return WaveReadLaneAt(val, lastLane);
}
float2 WaveReadLaneLast(float2 val)
{
    uint lastLane = WaveGetLastLaneIndex();
    return WaveReadLaneAt(val, lastLane);
}
float3 WaveReadLaneLast(float3 val)
{
    uint lastLane = WaveGetLastLaneIndex();
    return WaveReadLaneAt(val, lastLane);
}
float4 WaveReadLaneLast(float4 val)
{
    uint lastLane = WaveGetLastLaneIndex();
    return WaveReadLaneAt(val, lastLane);
}
#endif

//-----------------------------------------------------------------------------
//      アクティブレーンに対して指定値の線形補間を実行します.
//-----------------------------------------------------------------------------
float2 WaveActiveLerp(float value, float t)
{
    float prefixProduct = WavePrefixProduct(1.0f - t);
    float laneValue     = value * t * prefixProduct;
    float interpolation = WaveActiveSum(laneValue);
 
    float postfixProduct = prefixProduct * (1.0f - t);
    float oneMinusT      = WaveReadLaneLast(postfixProduct);
 
    return float2(interpolation, oneMinusT);
}

#if __HLSL_VERSION >= 2021
//-----------------------------------------------------------------------------
//      スカラー化ロードを試みます.
//-----------------------------------------------------------------------------
template<typename T>
void TryScalarLoad(out T result, StructuredBuffer<T> buffer, uint index)
{
    // [Mishima 2016] 三嶋仁, 清水昭尋, "「バイオハザード7」を実現するレンダリング技術",
    // CEDEC 2016, Slide 15, https://cedil.cesa.or.jp/cedil_sessions/view/1488

    uint lane = WaveReadFirstLane(index);
    if (WaveActiveAllTrue(index == lane))
        result = buffer[lane];
    else
        result = buffer[index];
}
#else
//-----------------------------------------------------------------------------
//      スカラー化ロードを試みます.
//-----------------------------------------------------------------------------
#define TryScalarLoad(result, buffer, index)    \
{                                               \
    uint lane = WaveReadLaneFirst(index);       \
    if (WaveActiveAllTrue(index == lane))       \
        result = buffer[lane];                  \
    else                                        \
        result = buffer[index];                 \
}
#endif

#endif//ASDX_WAVE_HELPER_HLSLI
