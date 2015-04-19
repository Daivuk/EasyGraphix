/*! \file eg.h */

#ifdef WIN32
#include <Windows.h>
#endif /* WIN32 */
#include <inttypes.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
    /*! \typedef EGDevice
        ID to a graphics device
    */
    typedef uint32_t EGDevice;

    /*! \typedef EGTexture
        ID to a texture
    */
    typedef uint32_t EGTexture;

    /*! \enum EG_CLEAR_BITFIELD
        Bitmask representing which part of the frame buffer to clear
    */
    typedef enum
    {
        EG_CLEAR_COLOR      = 0x01, /*!< Color buffer */
        EG_CLEAR_DEPTH      = 0x02, /*!< Depth buffer value */
        EG_CLEAR_STENCIL    = 0x04  /*!< Stencil buffer */
    } EG_CLEAR_BITFIELD;

    typedef enum
    {
        EG_POINTS           = 0,
        EG_LINES            = 1,
        EG_LINE_STRIP       = 2,
        EG_LINE_LOOP        = 3,
        EG_TRIANGLES        = 4,
        EG_TRIANGLE_STRIP   = 5,
        EG_TRIANGLE_FAN     = 6,
        EG_POLYGON          = EG_TRIANGLE_FAN,
        EG_QUADS            = 7,
        EG_QUAD_STRIP       = 8,
        EG_OMNIS            = 9,
        EG_SPOTS            = 10,
        EG_AMBIENTS         = 11,
        EG_DIRECTIONALS     = 12,
        EG_SPRITES          = 13
    } EG_TOPOLOGY;

    typedef enum
    {
        EG_R    = 0x10,
        EG_RG   = 0x20,
        EG_RGB  = 0x30,
        EG_RGBA = 0x40
    } EG_DATA_COMPONENT_COUNT;

    typedef enum
    {
        EG_U8   = 0x01,
        EG_U16  = 0x02,
        EG_U32  = 0x03,
        EG_U64  = 0x04,
        EG_S8   = 0x05,
        EG_S16  = 0x06,
        EG_S32  = 0x07,
        EG_S64  = 0x08,
        EG_F32  = 0x09,
        EG_F64  = 0x0a
    } EG_DATA_TYPE;
    
    typedef uint32_t EG_FORMAT;

    typedef enum
    {
        EG_GENERATE_MIPMAPS     = 0x01,
        EG_RENDER_TARGET        = 0x02,
        EG_DEPTH_STENCIL        = 0x04,
        EG_GENERATE_NORMAL_MAP  = 0x08,
        EG_NORMAL_MAP_OCCLUSION = 0x10,
        EG_PRE_MULTIPLIED       = 0x20
    } EG_TEXTURE_FLAGS;

    typedef enum
    {
        EG_BLEND                = 0,
        EG_CULL                 = 1,
        EG_DEPTH_TEST           = 2,
        EG_STENCIL_TEST         = 3,
        EG_ALPHA_TEST           = 4,
        EG_DIFFUSE_MAP          = 5,
        EG_NORMAL_MAP           = 6,
        EG_MATERIAL_MAP         = 7,
        EG_SCISSOR              = 8,
        EG_BLOOM                = 9,
        EG_HDR                  = 10,
        EG_BLUR                 = 11,
        EG_WIREFRAME            = 11,
        EG_SHADOW               = 12,
        EG_DISTORTION           = 13,
        EG_AMBIENT_OCCLUSION    = 14
    } EG_ENABLE_BITS;

    // Device related functions
#ifdef WIN32
    EGDevice    egCreateDevice(HWND windowHandle);
#else /* WIN32 */
#endif /* WIN32 */
    void        egDestroyDevice(EGDevice *pDevice);
    void        egBindDevice(EGDevice device);
    void        egSwap();

    // Clear
    void        egClearColor(float r, float g, float b, float a);
    void        egClearColorv(const float *pColor);
    void        egClearDepth(float d);
    void        egClearStencil(uint8_t s);
    void        egClear(EG_CLEAR_BITFIELD clearBitFields);

    // View setup
    void        egSet2DViewProj(float nearClip, float farClip);
    void        egSet3DViewProj(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ, float fov, float nearClip, float farClip);
    void        egSetIsometricViewProj();
    void        egSetViewProj(const float *pView, const float *pModel);

    void        egViewPort(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    void        egScissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

    void        egModelIdentity();
    void        egModelTranslate(float x, float y, float z);
    void        egModelTranslatev(const float *pAxis);
    void        egModelMult(const float *pMatrix);
    void        egModelRotate(float angle, float x, float y, float z);
    void        egModelRotatev(float angle, const float *pAxis);
    void        egModelScale(float x, float y, float z);
    void        egModelScalev(const float *pAxis);
    void        egModelPush();
    void        egModelPop();

    void        egBegin(EG_TOPOLOGY topology);
    void        egEnd();

    void        egColor3(float r, float g, float b);
    void        egColor3v(const float *pRGB);
    void        egColor4(float r, float g, float b, float a);
    void        egColor4v(const float *pRGBA);
    void        egNormal(float nx, float ny, float nz);
    void        egNormalv(const float *pNormal);
    void        egTexCoord(float u, float v);
    void        egTexCoordv(const float *pTexCoord);
    void        egPosition2(float x, float y);
    void        egPosition2v(const float *pPos);
    void        egPosition3(float x, float y, float z);
    void        egPosition3v(const float *pPos);
    void        egTarget2(float x, float y);
    void        egTarget2v(const float *pPos);
    void        egTarget3(float x, float y, float z);
    void        egTarget3v(const float *pPos);
    void        egRadius(float radius);
    void        egRadius2(float innerRadius, float outterRadius);
    void        egRadius2v(const float *pRadius);
    void        egFalloutExponent(float exponent);
    void        egMultiply(float multiply);
    void        egSpecular(float intensity, float shininess);
    void        egSelfIllum(float intensity);

    EGTexture   egCreateTexture1D(uint32_t dimension, const void *pData, EG_FORMAT dataFormat);
    EGTexture   egCreateTexture2D(uint32_t width, uint32_t height, const void *pData, uint32_t dataType, EG_TEXTURE_FLAGS flags);
    EGTexture   egCreateTexture3D(uint32_t width, uint32_t height, uint32_t depth, const void *pData, uint32_t dataType);
    EGTexture   egCreateCubeMap(uint32_t dimension, const void *pData, EG_FORMAT dataFormat, EG_TEXTURE_FLAGS flags);
    void        egDestroyTexture(EGTexture *pTexture);
    void        egBindDiffuse(EGTexture texture);
    void        egBindNormal(EGTexture texture);
    void        egBindMaterial(EGTexture texture);

    void        egEnable(EG_ENABLE_BITS stateBits);
    void        egDisable(EG_ENABLE_BITS stateBits);
    void        egStatePush();
    void        egStatePop();

    void        egCube(float size);
    void        egSphere(float radius, uint32_t slices, uint32_t stacks);
    void        egCylinder(float bottomRadius, float topRadius, float height, uint32_t slices);
    void        egTube(float outterRadius, float innerRadius, float height, uint32_t slices);
    void        egTorus(float radius, float innerRadius, uint32_t slices, uint32_t stacks);
#ifdef __cplusplus
}
#endif /* __cplusplus */
