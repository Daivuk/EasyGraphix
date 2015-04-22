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

const char *g_ps = MULTILINE(
    Texture2D xDiffuse:register(t0);
    Texture2D xNormal:register(t1);
    Texture2D xMaterial:register(t2);
    SamplerState sSampler:register(s0);

    struct sInput
    {
        float4 position:SV_POSITION;
        float3 normal:NORMAL;
        float3 tangent:TANGENT;
        float3 binormal:BINORMAL;
        float2 texCoord:TEXCOORD;
        float4 color:COLOR;
        float2 depth:TEXCOORD1;
    };

    struct sOutput
    {
        float4 diffuse:SV_Target0;
        float4 depth:SV_Target1;
        float4 normal:SV_Target2;
        float4 material:SV_Target3;
    };

    sOutput main(sInput input)
    {
        float4 xdiffuse = xDiffuse.Sample(sSampler, input.texCoord);
        float4 xnormal = xNormal.Sample(sSampler, input.texCoord);
        float4 xmaterial = xMaterial.Sample(sSampler, input.texCoord);
        sOutput output;
        output.diffuse = xdiffuse * input.color;
        output.depth = input.depth.x / input.depth.y;
        xnormal.xyz = xnormal.xyz * 2 - 1;
        output.normal.xyz = normalize(xnormal.x * input.tangent + xnormal.y * input.binormal + xnormal.z * input.normal);
        output.normal.xyz = output.normal.xyz * .5 + .5;
        output.normal.a = 0;
        output.material = xmaterial;
        return output;
    }
);

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
        return xdiffuse * input.color + xdiffuse * xmaterial.b;
    }
);

const char *g_psOmni = MULTILINE(
    cbuffer InvViewProjCB:register(b0)
    {
        matrix invViewProjection;
    }

    cbuffer OmniCB:register(b1)
    {
        float3 lPos;
        float lRadius;
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
        float3 lightDir = lPos - position.xyz;
    
        // Attenuation stuff
        float dis = length(lightDir);
        lightDir /= dis;
        dis /= lRadius;
        dis = saturate(1 - dis);
        dis = pow(dis, 1);
        float dotNormal = dot(normal, lightDir);
        dotNormal = saturate(dotNormal);
        float intensity = dis * dotNormal;
        finalColor += xdiffuse * lColor * intensity;
    
        // Calculate specular
        float3 v = normalize(float3(-5, -5, 5));
        float3 h = normalize(lightDir + v);
        finalColor += xmaterial.r * lColor * intensity * (pow(saturate(dot(normal, h)), xmaterial.g * 100));
    
        return finalColor;
    }
);
