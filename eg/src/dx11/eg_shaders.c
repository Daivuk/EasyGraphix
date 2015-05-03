#include <assert.h>
#include "eg_shaders.h"

ID3DBlob *compileShader(const char *szSource, const char *szProfile)
{
    ID3DBlob *shaderBlob = NULL;
#ifdef _DEBUG
    ID3DBlob *errorBlob = NULL;
#endif

    HRESULT result = D3DCompile(szSource, (SIZE_T)strlen(szSource), NULL, NULL, NULL, "main", szProfile,
                                D3DCOMPILE_ENABLE_STRICTNESS
#ifdef _DEBUG
                                | D3DCOMPILE_DEBUG
#endif
                                , 0, &shaderBlob, &errorBlob);

#ifdef _DEBUG
    if (errorBlob)
    {
        char *pError = (char*)errorBlob->lpVtbl->GetBufferPointer(errorBlob);
        assert(FALSE);
    }
#endif

    return shaderBlob;
}

#define MULTILINE(...) #__VA_ARGS__

// Shaders
const char *g_vs = MULTILINE(
    cbuffer ViewProjCB:register(b0)
    {
        matrix viewProj;
    }

    cbuffer ModelVB:register(b1)
    {
        matrix model;
    }

    struct sInput
    {
        float3 position:POSITION;
        float3 normal:NORMAL;
        float3 tangent:TANGENT;
        float3 binormal:BINORMAL;
        float2 texCoord:TEXCOORD;
        float4 color:COLOR;
    };

    struct sOutput
    {
        float4 position:SV_POSITION;
        float3 normal:NORMAL;
        float3 tangent:TANGENT;
        float3 binormal:BINORMAL;
        float2 texCoord:TEXCOORD;
        float4 color:COLOR;
        float2 depth:TEXCOORD1;
    };

    sOutput main(sInput input)
    {
        sOutput output;
        float4 worldPosition = mul(float4(input.position, 1), model);
        output.position = mul(worldPosition, viewProj);
        output.normal = normalize(mul(float4(input.normal, 0), model).xyz);
        output.tangent = normalize(mul(float4(input.tangent, 0), model).xyz);
        output.binormal = normalize(mul(float4(input.binormal, 0), model).xyz);
        output.texCoord = input.texCoord;
        output.color = input.color;
        output.depth.xy = output.position.zw;
        return output;
    }
    );

#define PSSTART_NOLIT \
    "Texture2D xDiffuse:register(t0);" \
    "SamplerState sSampler:register(s0);" \
    "struct sInput" \
    "{" \
    "    float4 position:SV_POSITION;" \
    "    float3 normal:NORMAL;" \
    "    float3 tangent:TANGENT;" \
    "    float3 binormal:BINORMAL;" \
    "    float2 texCoord:TEXCOORD;" \
    "    float4 color:COLOR;" \
    "    float2 depth:TEXCOORD1;" \
    "};" \
    "float4 main(sInput input):SV_TARGET" \
    "{" \
    "    float4 xdiffuse = xDiffuse.Sample(sSampler, input.texCoord);"

#define PSEND_NOLIT \
    "    return xdiffuse * input.color;" \
    "}"

#define PSSTART \
    "Texture2D xDiffuse:register(t0);" \
    "Texture2D xNormal:register(t1);" \
    "Texture2D xMaterial:register(t2);" \
    "SamplerState sSampler:register(s0);" \
    "struct sInput" \
    "{" \
    "    float4 position:SV_POSITION;" \
    "    float3 normal:NORMAL;" \
    "    float3 tangent:TANGENT;" \
    "    float3 binormal:BINORMAL;" \
    "    float2 texCoord:TEXCOORD;" \
    "    float4 color:COLOR;" \
    "    float2 depth:TEXCOORD1;" \
    "};" \
    "struct sOutput" \
    "{" \
    "    float4 diffuse:SV_Target0;" \
    "    float4 depth:SV_Target1;" \
    "    float4 normal:SV_Target2;" \
    "    float4 material:SV_Target3;" \
    "};" \
    "sOutput main(sInput input)" \
    "{" \
    "    float4 xdiffuse = xDiffuse.Sample(sSampler, input.texCoord);" \
    "    float4 xnormal = xNormal.Sample(sSampler, input.texCoord);" \
    "    float4 xmaterial = xMaterial.Sample(sSampler, input.texCoord);"

#define PSEND \
    "    sOutput output;" \
    "    output.diffuse = xdiffuse * input.color;" \
    "    output.depth = input.depth.x / input.depth.y;" \
    "    xnormal.xyz = xnormal.xyz * 2 - 1;" \
    "    output.normal.xyz = normalize(xnormal.x * input.tangent + xnormal.y * input.binormal + xnormal.z * input.normal);" \
    "    output.normal.xyz = output.normal.xyz * .5 + .5;" \
    "    output.normal.a = 0;" \
    "    output.material = xmaterial;" \
    "    return output;" \
    "}"

#define PSALPHATESTSTART \
"cbuffer AlphaTestRefCB : register(b4)" \
"{" \
"    float alphaRef;" \
"    float vignetteExponent;" \
"    float2 AlphaTestRefCB_padding;" \
"}"

const char *g_pses[18] = {
    PSSTART PSEND, // EG_LIGHTING !EG_ALPHA_TEST
    PSALPHATESTSTART PSSTART "discard;" PSEND, // EG_LIGHTING EG_ALPHA_TEST
    PSALPHATESTSTART PSSTART "if (xdiffuse.a < alphaRef) discard;" PSEND, // EG_LIGHTING EG_ALPHA_TEST
    PSALPHATESTSTART PSSTART "if (xdiffuse.a == alphaRef) discard;" PSEND, // EG_LIGHTING EG_ALPHA_TEST
    PSALPHATESTSTART PSSTART "if (xdiffuse.a <= alphaRef) discard;" PSEND, // EG_LIGHTING EG_ALPHA_TEST
    PSALPHATESTSTART PSSTART "if (xdiffuse.a > alphaRef) discard;" PSEND, // EG_LIGHTING EG_ALPHA_TEST
    PSALPHATESTSTART PSSTART "if (xdiffuse.a != alphaRef) discard;" PSEND, // EG_LIGHTING EG_ALPHA_TEST
    PSALPHATESTSTART PSSTART "if (xdiffuse.a >= alphaRef) discard;" PSEND, // EG_LIGHTING EG_ALPHA_TEST
    PSSTART PSEND, // EG_LIGHTING EG_ALPHA_TEST

    PSSTART_NOLIT PSEND_NOLIT, // !EG_LIGHTING !EG_ALPHA_TEST
    PSALPHATESTSTART PSSTART_NOLIT "discard;" PSEND_NOLIT, // !EG_LIGHTING EG_ALPHA_TEST
    PSALPHATESTSTART PSSTART_NOLIT "if (xdiffuse.a < alphaRef) discard;" PSEND_NOLIT, // !EG_LIGHTING EG_ALPHA_TEST
    PSALPHATESTSTART PSSTART_NOLIT "if (xdiffuse.a == alphaRef) discard;" PSEND_NOLIT, // !EG_LIGHTING EG_ALPHA_TEST
    PSALPHATESTSTART PSSTART_NOLIT "if (xdiffuse.a <= alphaRef) discard;" PSEND_NOLIT, // !EG_LIGHTING EG_ALPHA_TEST
    PSALPHATESTSTART PSSTART_NOLIT "if (xdiffuse.a > alphaRef) discard;" PSEND_NOLIT, // !EG_LIGHTING EG_ALPHA_TEST
    PSALPHATESTSTART PSSTART_NOLIT "if (xdiffuse.a != alphaRef) discard;" PSEND_NOLIT, // !EG_LIGHTING EG_ALPHA_TEST
    PSALPHATESTSTART PSSTART_NOLIT "if (xdiffuse.a >= alphaRef) discard;" PSEND_NOLIT, // !EG_LIGHTING EG_ALPHA_TEST
    PSSTART_NOLIT PSEND_NOLIT, // !EG_LIGHTING EG_ALPHA_TEST
};

const char *g_vsPassThrough = MULTILINE(
    struct sInput
    {
        float2 position:POSITION;
        float2 texCoord:TEXCOORD;
        float4 color:COLOR;
    };

    struct sOutput
    {
        float4 position:SV_POSITION;
        float2 texCoord:TEXCOORD;
        float4 color:COLOR;
    };

    sOutput main(sInput input)
    {
        sOutput output;
        output.position = float4(input.position, 0, 1);
        output.texCoord = input.texCoord;
        output.color = input.color;
        return output;
    }
);

const char *g_psPassThrough = MULTILINE(
    Texture2D xDiffuse:register(t0);
    SamplerState sSampler:register(s0);

    struct sInput
    {
        float4 position:SV_POSITION;
        float2 texCoord:TEXCOORD;
        float4 color:COLOR;
    };

    float4 main(sInput input):SV_TARGET
    {
        float4 xdiffuse = xDiffuse.Sample(sSampler, input.texCoord);
        return xdiffuse * input.color;
    }
);

const char *g_psLDR = MULTILINE(
    Texture2D xDiffuse:register(t0);
    SamplerState sSampler:register(s0);

    struct sInput
    {
        float4 position:SV_POSITION;
        float2 texCoord:TEXCOORD;
        float4 color:COLOR;
    };

    float4 main(sInput input):SV_TARGET
    {
        float4 xdiffuse = xDiffuse.Sample(sSampler, input.texCoord);
        xdiffuse -= 1;
        return xdiffuse * input.color;
    }
    );

#define GAUSSIAN_BLUR

#ifdef GAUSSIAN_BLUR
#define BLUR_HEADER \
"cbuffer BlurCB : register(b3)" \
"{" \
"    float4 blurSpread;" \
"}" \
"Texture2D xDiffuse : register(t0);" \
"SamplerState sSampler : register(s0);" \
"struct sInput" \
"{" \
"    float4 position : SV_POSITION;" \
"    float2 texCoord : TEXCOORD;" \
"    float4 color : COLOR;" \
"};" \
"static const float BlurWeights[17] =" \
"{" \
"    0.003924, 0.008962, 0.018331, 0.033585, 0.055119, 0.081029, 0.106701, 0.125858, 0.13298, 0.125858, 0.106701, 0.081029, 0.055119, 0.033585, 0.018331, 0.008962, 0.003924" \
"};"

const char *g_psBlurH = BLUR_HEADER MULTILINE(
    float4 main(sInput input):SV_TARGET
    {
        float4 xdiffuse = float4(0, 0, 0, 0);
        for (int i = -8; i <= 8; ++i)
        {
            xdiffuse += xDiffuse.Sample(sSampler, input.texCoord + float2(blurSpread.x * i, 0)) * BlurWeights[i + 8];
        }
        return xdiffuse * input.color;
    }
);

const char *g_psBlurV = BLUR_HEADER MULTILINE(
    float4 main(sInput input):SV_TARGET
    {
        float4 xdiffuse = float4(0, 0, 0, 0);
        for (int i = -8; i <= 8; ++i)
        {
            xdiffuse += xDiffuse.Sample(sSampler, input.texCoord + float2(0, blurSpread.y * i)) * BlurWeights[i + 8];
        }
        return xdiffuse * input.color;
    }
);
#else
const char *g_psBlurH = MULTILINE(
    cbuffer BlurCB : register(b3)
    {
        float4 blurSpread;
    }

    Texture2D xDiffuse:register(t0);
    SamplerState sSampler:register(s0);

    struct sInput
    {
        float4 position:SV_POSITION;
        float2 texCoord:TEXCOORD;
        float4 color:COLOR;
    };

    float4 main(sInput input):SV_TARGET
    {
        float4 xdiffuse = xDiffuse.Sample(sSampler, input.texCoord);
        for (int i = 1; i <= 8; ++i)
        {
            xdiffuse += xDiffuse.Sample(sSampler, input.texCoord + float2(blurSpread.x * i, 0));
            xdiffuse += xDiffuse.Sample(sSampler, input.texCoord - float2(blurSpread.x * i, 0));
        }
        xdiffuse /= 17;
        return xdiffuse * input.color;
    }
);

const char *g_psBlurV = MULTILINE(
    cbuffer BlurCB : register(b3)
    {
        float4 blurSpread;
    }

    Texture2D xDiffuse:register(t0);
    SamplerState sSampler:register(s0);

    struct sInput
    {
        float4 position:SV_POSITION;
        float2 texCoord:TEXCOORD;
        float4 color:COLOR;
    };

    float4 main(sInput input):SV_TARGET
    {
        float4 xdiffuse = xDiffuse.Sample(sSampler, input.texCoord);
        for (int i = 1; i <= 8; ++i)
        {
            xdiffuse += xDiffuse.Sample(sSampler, input.texCoord + float2(0, blurSpread.y * i));
            xdiffuse += xDiffuse.Sample(sSampler, input.texCoord - float2(0, blurSpread.y * i));
        }
        xdiffuse /= 17;
        return xdiffuse * input.color;
    }
);
#endif

const char *g_psPostProcess[2] = {
    MULTILINE(
    Texture2D xHdr : register(t0);
    Texture2D xBloom : register(t1);
    SamplerState sSampler : register(s0);

    struct sInput
    {
        float4 position : SV_POSITION;
        float2 texCoord : TEXCOORD;
        float4 color : COLOR;
    };

    float4 main(sInput input) :SV_TARGET
    {
        float4 xhdr = xHdr.Sample(sSampler, input.texCoord);
        float4 xbloom = xBloom.Sample(sSampler, input.texCoord);

        // Mix HDR and bloom
        float4 color = lerp(xhdr, xbloom, 0.4);

        // Exposure level
        color *= 1.0;

        return color * input.color;
    }),
    MULTILINE(
    Texture2D xHdr : register(t0);
    Texture2D xBloom : register(t1);
    SamplerState sSampler : register(s0);

    cbuffer AlphaTestRefCB : register(b2)
    {
        float alphaRef;
        float vignetteExponent;
        float2 AlphaTestRefCB_padding;
    }

    struct sInput
    {
        float4 position : SV_POSITION;
        float2 texCoord : TEXCOORD;
        float4 color : COLOR;
    };

    float4 main(sInput input) :SV_TARGET
    {
        float4 xhdr = xHdr.Sample(sSampler, input.texCoord);
        float4 xbloom = xBloom.Sample(sSampler, input.texCoord);

        // Mix HDR and bloom
        float4 color = lerp(xhdr, xbloom, 0.4);

        // Vignette
        input.texCoord -= 0.5;
        float vignette = abs(1 - dot(input.texCoord, input.texCoord));
        color *= pow(vignette, vignetteExponent);

        // Exposure level
        color *= 1.0;

        return color * input.color;
    })
};

const char *g_psAmbient = MULTILINE(
    Texture2D xDiffuse:register(t0);
    Texture2D xMaterial:register(t3);
    SamplerState sSampler:register(s0);

    struct sInput
    {
        float4 position:SV_POSITION;
        float2 texCoord:TEXCOORD;
        float4 color:COLOR;
    };

    float4 main(sInput input):SV_TARGET
    {
        float4 xdiffuse = xDiffuse.Sample(sSampler, input.texCoord);
        float4 xmaterial = xMaterial.Sample(sSampler, input.texCoord);
        return xdiffuse * input.color + xdiffuse * xmaterial.b * 2;
    }
);

const char *g_psOmni = MULTILINE(
    cbuffer InvViewProjCB:register(b0)
    {
        matrix invViewProjection;
    }

    cbuffer OmniCB:register(b1)
    {
        float4 lPos;
        float lRadius, lMultiply, ls, lt;
        float4 lColor;
    }

    Texture2D xDiffuse:register(t0);
    Texture2D xDepth:register(t1);
    Texture2D xNormal:register(t2);
    Texture2D xMaterial:register(t3);
    SamplerState sSampler:register(s0);

    struct sInput
    {
        float4 position:SV_POSITION;
        float2 texCoord:TEXCOORD;
        float4 color:COLOR;
    };

    float4 main(sInput input):SV_TARGET
    {
        float4 xdiffuse = xDiffuse.Sample(sSampler, input.texCoord);
        float xdepth = xDepth.Sample(sSampler, input.texCoord).r;
        float3 xnormal = xNormal.Sample(sSampler, input.texCoord).xyz;
        float4 xmaterial = xMaterial.Sample(sSampler, input.texCoord);
    
        float4 finalColor = float4(0, 0, 0, 0);
    
        // Position
        float4 position;
        position.xy = float2(input.texCoord.x * 2 - 1, -(input.texCoord.y * 2 - 1));
        position.z = xdepth;
        position.w = 1.0f;
        position = mul(position, invViewProjection);
        position /= position.w;
    
        // Normal
        float3 normal = normalize(xnormal * 2 - 1);
        float3 lightDir = lPos.xyz - position.xyz;
    
        // Attenuation stuff
        float dis = length(lightDir);
        lightDir /= dis;
        dis /= lRadius;
        dis = saturate(1 - dis);
        dis = pow(dis, 1);
        float dotNormal = dot(normal, lightDir);
        dotNormal = saturate(dotNormal);
        float intensity = dis * dotNormal * lMultiply;
        finalColor += xdiffuse * lColor * intensity;
    
        // Calculate specular
        float3 v = normalize(float3(-5, -5, 5));
        float3 h = normalize(lightDir + v);
        finalColor += xmaterial.r * lColor * intensity * (pow(saturate(dot(normal, h)), xmaterial.g * 100));
    
        return finalColor;
    }
);
