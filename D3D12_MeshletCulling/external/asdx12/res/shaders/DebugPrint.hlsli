//-----------------------------------------------------------------------------
// File : DebugPrint.hlsli
// Desc : Debug Print Function.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#ifndef DEBUG_PRINT_HLSLI
#define DEBUG_PRINT_HLSLI

// ---- GLSL Number Printing - @P_Malin ----
// Creative Commons CC0 1.0 Universal (CC-0) 
// https://www.shadertoy.com/view/4sBSWW

static const float2 kDefaultFontSize = float2(16.0f, 20.0f);

float mod(float a, float b)
{
    return a - b * floor(a / b);
}

float DigitBin(const int x)
{
    switch(x)
    {
        case 0: { return 480599.0; }
        case 1: { return 139810.0; }
        case 2: { return 476951.0; }
        case 3: { return 476999.0; }
        case 4: { return 350020.0; }
        case 5: { return 464711.0; }
        case 6: { return 464727.0; }
        case 7: { return 476228.0; }
        case 8: { return 481111.0; }
        case 9: { return 481095.0; }
        default:{ return 0.0; }
    }
}

float PrintValue
(
    float2 stringCoords,
    float  value,
    float  maxDigits,
    float  decimalPlaces
)
{
    if ((stringCoords.y < 0.0) || (stringCoords.y >= 1.0))
        return 0.0;
    
    bool neg = (value < 0.0);
    value = abs(value);
    
    float log10Value   = log2(value) / log2(10.0);
    float biggestIndex = max(floor(log10Value), 0.0);
    float digitIndex   = maxDigits - floor(stringCoords.x);
    float charBin      = 0.0;
    
    if (digitIndex > (-decimalPlaces - 1.01))
    {
        if (digitIndex > biggestIndex)
        {
            if (neg && (digitIndex < (biggestIndex + 1.5)))
                charBin = 1792.0;
        }
        else
        {
            if (digitIndex == -1.0)
            {
                if (decimalPlaces > 0.0)
                    charBin = 2.0;
            }
            else
            {
                float reducedRangeValue = value;
                if (digitIndex < 0.0)
                {
                    reducedRangeValue = frac(value);
                    digitIndex += 1.0;
                }
                float digitValue = abs(reducedRangeValue / (pow(10.0, digitIndex)));
                charBin = DigitBin(int(floor(mod(digitValue, 10.0))));
            }
        }
    }
    return floor(mod(charBin / pow(2.0, floor(frac(stringCoords.x) * 4.0) + (floor((1.0f - stringCoords.y) * 5.0) * 4.0)), 2.0));
}

float PrintFloat
(
    const in float2 svPosition,     // 処理対象ピクセル(スクリーン座標)
    const in float2 drawPos,        // 文字列を描画する位置座標(スクリーン座標)
    const in float2 fontSize,       // フォントサイズ.
    const in float  value,          // 描画する数値.
    const in uint   maxDigits,      // 最大桁数.
    const in uint   decimalPlaces   // 小数点以下桁数.
)
{
    float2 stringCharCoords = (svPosition - drawPos) / fontSize;
    return PrintValue(stringCharCoords, value, (float)maxDigits, (float)decimalPlaces);
}

float PrintInt
(
    const in float2 svPosition,     // 処理対象ピクセル(スクリーン座標)
    const in float2 drawPos,        // 文字列を描画する位置座標(スクリーン座標).
    const in float2 fontSize,       // フォントサイズ.
    const in int    value,          // 描画する数値.
    const in uint   maxDigits,      // 最大桁数.
)
{
    return PrintFloat(svPosition, drawPos, fontSize, (float)value, maxDigits, 0);
}



#endif//DEBUG_PRINT_HLSLI
