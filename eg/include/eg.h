/*! \file eg.h */ 

#pragma once

#ifndef EG_H_INCLUDED
#define EG_H_INCLUDED

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

    /*! \typedef EGState
        ID of a state object
    */
    typedef uint32_t EGState;

    /*! \typedef EGFormat
        Representing a mix of EG_DATA_COMPONENT_COUNT and EG_DATA_TYPE
    */
    typedef uint32_t EGFormat;

    /*! \typedef EGEnable
        Holds enabled bits
    */
    typedef uint32_t EGEnable;

    /*! \enum EG_CLEAR_BITFIELD
        Bitmask representing which part of the frame buffer to clear
    */
    typedef enum
    {
        /*! Clear accumulation buffer and g-buffer's diffuse */
        EG_CLEAR_COLOR          = 0x01, 

        /*! Clear Depth stencil buffer */
        EG_CLEAR_DEPTH_STENCIL  = 0x02, 

        /*! Clear g-buffer's diffuse, depth, normal and material */
        EG_CLEAR_G_BUFFER       = 0x04, 

        /*! Clear all of the above */
        EG_CLEAR_ALL            = 0x07  

    } EG_CLEAR_BITFIELD;

    /*! \enum EG_TOPOLOGY
        The primitive, primitives or effects that will be created from vertices
        presented between glBegin and the subsequent glEnd. 
    */
    typedef enum
    {
        /*! Treats each vertex as a single point. Vertex n defines point n. N 
            points are drawn. */
        EG_POINTS           = 0,

        /*! Treats each pair of vertices as an independent line segment. 
            Vertices 2n - 1 and 2n define line n. N/2 lines are drawn. */
        EG_LINES            = 1,

        /*! Draws a connected group of line segments from the first vertex to 
            the last. Vertices n and n+1 define line n. N - 1 lines are 
            drawn. */
        EG_LINE_STRIP       = 2,

        /*! Draws a connected group of line segments from the first vertex to 
            the last, then back to the first. Vertices n and n + 1 define line 
            n. The last line, however, is defined by vertices N and 1. N lines 
            are drawn. */
        EG_LINE_LOOP        = 3,

        /*! Treats each triplet of vertices as an independent triangle. 
            Vertices 3n - 2, 3n - 1, and 3n define triangle n. N/3 triangles 
            are drawn. */
        EG_TRIANGLES        = 4,

        /*! Draws a connected group of triangles. One triangle is defined for 
            each vertex presented after the first two vertices. For odd n, 
            vertices n, n + 1, and n + 2 define triangle n. For even n, 
            vertices n + 1, n, and n + 2 define triangle n. N - 2 triangles 
            are drawn. */
        EG_TRIANGLE_STRIP   = 5,

        /*! Draws a connected group of triangles. one triangle is defined for 
            each vertex presented after the first two vertices. Vertices 1, 
            n + 1, n + 2 define triangle n. N - 2 triangles are drawn. */
        EG_TRIANGLE_FAN     = 6,

        /*! Same as EG_TRIANGLE_FAN */
        EG_POLYGON          = EG_TRIANGLE_FAN,

        /*! Treats each group of four vertices as an independent quadrilateral.
            Vertices 4n - 3, 4n - 2, 4n - 1, and 4n define quadrilateral n.
            N/4 quadrilaterals are drawn. */
        EG_QUADS            = 7,

        /*! Draws a connected group of quadrilaterals. One quadrilateral is 
            defined for each pair of vertices presented after the first pair. 
            Vertices 2n - 1, 2n, 2n + 2, and 2n + 1 define quadrilateral n. 
            N/2 - 1 quadrilaterals are drawn. Note that the order in which 
            vertices are used to construct a quadrilateral from strip data is 
            different from that used with independent data. */
        EG_QUAD_STRIP       = 8,

        /*! Treats each vertex as a single omni directional light. Vertex n 
            defines light n. N lights are drawn. They are accumulated with 
            additive blending. */
        EG_OMNIS = 9,

        /*! Treats each vertex as a single spot light. Vertex n defines light
            n. N lights are drawn. They are accumulated with additive 
            blending. */
        EG_SPOTS = 10,

        /*! Treats each vertex as a single ambient pass. Vertex n defines 
            ambient n. N ambient pass are drawn. They are accumulated with 
            additive blending. */
        EG_AMBIENTS = 11,

        /*! Treats each vertex as a single directional spot light. Vertex n 
            defines light n. N lights are drawn. They are accumulated with 
            additive blending. */
        EG_DIRECTIONALS     = 12,

        /*! Treats each vertex as a single sprite. Vertex n defines sprite n. N
            sprites are drawn. */
        EG_SPRITES = 13

    } EG_MODE;

    /*! \enum EG_DATA_COMPONENT_COUNT
        Represent component count
    */
    typedef enum
    {
        /*! 1 component. Red */
        EG_R    = 0x01,

        /*! 2 component. Red and Green */
        EG_RG   = 0x02,

        /*! 3 component. Red, Green and Blue */
        EG_RGB  = 0x03,

        /*! 4 component. Red, Green, Blue and Alpha */
        EG_RGBA = 0x04

    } EG_DATA_COMPONENT_COUNT;

    /*! \enum EG_DATA_TYPE
        Represent data type information
    */
    typedef enum
    {
        /*! 8 bits unsigned integer. [0, 255] */
        EG_U8   = 0x10,

        /*! 16 bits unsigned integer. [0, 65535] */
        EG_U16  = 0x20,

        /*! 32 bits unsigned integer. [0, 4294967295] */
        EG_U32  = 0x30,

        /*! 64 bits unsigned integer. [0, 18446744073709551615] */
        EG_U64 = 0x40,

        /*! 8 bits signed integer. [-128, 127] */
        EG_S8 = 0x50,

        /*! 16 bits signed integer. [-32768, 32767] */
        EG_S16 = 0x60,

        /*! 32 bits signed integer. [-2147483648, 2147483647] */
        EG_S32 = 0x70,
            
        /*! 64 bits signed integer. 
            [-9223372036854775808, 9223372036854775807] */
        EG_S64 = 0x80,

        /*! 32 bits floating point. [0, 1] */
        EG_F32 = 0x90,

        /*! 64 bits floating point. [0, 1] */
        EG_F64 = 0xa0

    } EG_DATA_TYPE;
    
    /*! \enum EG_TEXTURE_FLAGS
        Bitmask flags used at texture creation
    */
    typedef enum
    {
        /*! Generate all mipmap levels for this texture */
        EG_GENERATE_MIPMAPS = 0x01,

        /*! Consider the data as heightmap, and generate a normal map from 
            it. If the incoming is more than one channel, average color will
            be calculated to be used as height map. Alpha channel is ignored */
        EG_GENERATE_NORMAL_MAP = 0x02,

        /*! Consider the data as heightmap, and generate ambient occlusion 
            data based on it. It is stored in the alpha channel. This should be
            used with EG_GENERATE_NORMAL_MAP. */
        EG_NORMAL_MAP_OCCLUSION = 0x04,

        /*! Pre-multiply RGB channels with the alpha channel. Used when 
            rendering 2D with pre-multipled alpha blend. */
        EG_PRE_MULTIPLY = 0x08

    } EG_TEXTURE_FLAGS;

    /*! \enum EG_ENABLE
        A symbolic constant indicating an EasyGraphix capability.
    */
    typedef enum
    {
        /*! If enabled, blend the incoming RGBA color values with the values in
            the color buffers. */
        EG_BLEND                        = 0x00000001,

        /*! If enabled, cull polygons based on their winding in window 
            coordinates. */
        EG_CULL                         = 0x00000002,

        /*! If enabled, do depth comparisons and update the depth buffer. */
        EG_DEPTH_TEST                   = 0x00000004,

        /*! If enabled, do stencil testing and update the stencil buffer. */
        EG_STENCIL_TEST                 = 0x00000008,

        /*! If enabled, do alpha testing. */
        EG_ALPHA_TEST                   = 0x00000010,

        /*! If enabled, discard fragments that are outside the scissor 
            rectangle. */
        EG_SCISSOR                      = 0x00000020,

        /*! If enabled, egTangent and egBinormal will be ignored and be 
            generated based on egTexCoord. */
        EG_GENERATE_TANGENT_BINORMAL    = 0x00000040,

        /*! If enabled, bloom will be applied when egPostProcess is called.
        This will only work if EG_HDR is also enabled. */
        EG_BLOOM                        = 0x00000080,

        /*! If enabled, high dynamic range will be applied when egPostProcess 
            is called. */
        EG_HDR                          = 0x00000100,

        /*! If enabled, screen bluring will be applied when egPostProcess is 
            called. */
        EG_BLUR                         = 0x00000200,

        /*! If enabled, primitives are drawn in wireframe mode. */
        EG_WIREFRAME                    = 0x00000400,

        /*! If enabled, currently drawn primitives or lights will cast 
            shadow. */
        EG_CAST_SHADOW                  = 0x00000800,

        /*! If enabled, primitives are drawn to the distortion map and applied
            when egPostProcess is called. */
        EG_DISTORTION                   = 0x00001000,

        /*! If enabled, screen space ambient occlusion will be applied when 
            egPostProcess is called. */
        EG_AMBIENT_OCCLUSION            = 0x00002000,

        /*! Defines wheter or not pixels will be written to depth buffer */
        EG_DEPTH_WRITE                  = 0x00004000,

        /*! Draw a vignette around corners of the screen. */
        EG_VIGNETTE                     = 0x00008000,

        /*! Draw information needed for lighting */
        EG_LIGHTING                     = 0x00010000,

        /*! Apply depth of field in post process */
        EG_DEPTH_OF_FIELD               = 0x00020000,

        /*! All enable bits */
        EG_ALL                          = 0xffffffff

    } EG_ENABLE;

    /*! \enum EG_BLEND_FACTOR
        Specifies pixel RGBA multiplyer.

        In the table and in subsequent equations, source and destination color 
        components are referred to as (Rs, Gs, Bs, As) and 
        (Rd, Gd, Bd, Ad)

        i = min(As, 1 - Ad)
    */
    typedef enum
    {
        /*! (0,0,0,0) */
        EG_ZERO                 = 0,

        /*! (1,1,1,1) */
        EG_ONE                  = 1,

        /*! (Rs,Gs,Bs,As) */
        EG_SRC_COLOR            = 2,

        /*! (1,1,1,1) - (Rs,Gs,Bs,As) */
        EG_ONE_MINUS_SRC_COLOR  = 3,

        /*! (Rd,Gd,Bd,Ad) */
        EG_DST_COLOR            = 4,

        /*! (1,1,1,1) - (Rd,Gd,Bd,Ad) */
        EG_ONE_MINUS_DST_COLOR  = 5,

        /*! (As,As,As,As) */
        EG_SRC_ALPHA            = 6,

        /*! (1,1,1,1) - (As,As,As,As) */
        EG_ONE_MINUS_SRC_ALPHA  = 7,

        /*! (Ad,Ad,Ad,Ad) */
        EG_DST_ALPHA            = 8,

        /*! (1,1,1,1) - (Ad,Ad,Ad,Ad) */
        EG_ONE_MINUS_DST_ALPHA  = 9,

        /*! (i,i,i, 1) */
        EG_SRC_ALPHA_SATURATE   = 10,

    } EG_BLEND_FACTOR;

    /*! \enum EG_FRONT_FACE
        Define front- and back-facing polygons
    */
    typedef enum
    {
        /*! Clockwise */
        EG_CW = 0,

        /*! Counter clockwise */
        EG_CCW = 1

    } EG_FRONT_FACE;

    /*! \enum EG_FILTER
        Filtering options during texture sampling.
    */
    typedef enum
    {
        /*! Use point sampling for minification, magnification, and mip-level 
        sampling */
        EG_FILTER_MIN_MAG_MIP_POINT = 0x00,

        /*! Use point sampling for minification and magnification; use linear 
        interpolation for mip-level sampling. */
        Eg_FILTER_MIN_MAG_POINT_MIP_LINEAR = 0x01,

        /*! Use point sampling for minification; use linear interpolation for 
        magnification; use point sampling for mip-level sampling. */
        EG_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x04,

        /*! Use point sampling for minification; use linear interpolation for 
        magnification and mip-level sampling. */
        EG_FILTER_MIN_POINT_MAG_MIP_LINEAR = 0x05,

        /*! Use linear interpolation for minification; use point sampling for 
        magnification and mip-level sampling. */
        EG_FILTER_MIN_LINEAR_MAG_MIP_POINT = 0x10,

        /*! Use linear interpolation for minification; use point sampling for 
        magnification; use linear interpolation for mip-level sampling. */
        EG_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x11,

        /*! Use linear interpolation for minification and magnification; use 
        point sampling for mip-level sampling. */
        EG_FILTER_MIN_MAG_LINEAR_MIP_POINT = 0x14,

        /*! Use linear interpolation for minification, magnification, and 
        mip-level sampling. */
        EG_FILTER_MIN_MAG_MIP_LINEAR = 0x15,

        /*! Use point sampling for minification, magnification, and mip-level
        sampling */
        EG_FILTER_NEAREST = EG_FILTER_MIN_MAG_MIP_POINT,

        /*! Use point sampling for minification; use linear interpolation for 
        magnification; use point sampling for mip-level sampling. */
        EG_FILTER_LINEAR = EG_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,

        /*! Use linear interpolation for minification and magnification; use 
        point sampling for mip-level sampling. */
        EG_FILTER_BILINEAR = EG_FILTER_MIN_MAG_LINEAR_MIP_POINT,

        /*! Use linear interpolation for minification, magnification, and 
        mip-level sampling. */
        EG_FILTER_TRILINEAR = EG_FILTER_MIN_MAG_MIP_LINEAR,

        /*! Use 1x anisotropic interpolation for minification, magnification, 
        and mip-level sampling. */
        EG_FILTER_ANISOTROPIC_1X = 0x0155,

        /*! Use 2x anisotropic interpolation for minification, magnification, 
        and mip-level sampling. */
        EG_FILTER_ANISOTROPIC_2X = 0x0255,

        /*! Use 4x anisotropic interpolation for minification, magnification, 
        and mip-level sampling. */
        EG_FILTER_ANISOTROPIC_4X = 0x0455,

        /*! Use 8x anisotropic interpolation for minification, magnification, 
        and mip-level sampling. */
        EG_FILTER_ANISOTROPIC_8X = 0x0855,

        /*! Use 16x anisotropic interpolation for minification, magnification,
        and mip-level sampling. */
        EG_FILTER_ANISOTROPIC_16X = 0x1055,

    } EG_FILTER;

    /*! \enum EG_COMPARE
        Comparison used for depth, stencil and alpha test
    */
    typedef enum
    {
        /*! Never */
        EG_NEVER = 0,

        /*! Less < */
        EG_LESS = 1,

        /*! Equal == */
        EG_EQUAL = 2,

        /*! Less or equal <= */
        EG_LEQUAL = 3,

        /*! Greater > */
        EG_GREATER = 4,

        /*! Not equal != */
        EG_NOTEQUAL = 5,

        /*! Greater or equal >= */
        EG_GEQUAL = 6,

        /*! Always */
        EG_ALWAYS = 7

    } EG_COMPARE;

    typedef enum
    {
        EG_RESOLUTION
    } EGGet;

    /*!
        Return zero terminated string describing the last error.
    */
    const char *egGetError();

    // Device related functions
#ifdef WIN32
    /*!
        Create a device.

        \param windowHandle Handle to the window surface that EasyGraphix
        will display to.

        \return ID of the newly created device. 0 if failed.
    */
    EGDevice egCreateDevice(HWND windowHandle);
#endif /* WIN32 */

    /*!
        Destroy a device.

        \param pDevice Pointer to the device to destroy. The value will be set 
        to 0 if successful.
    */
    void egDestroyDevice(EGDevice *pDevice);

    /*!
        Bind a device. All subsequent calls to eg are applied to the bound 
        device.

        \param device Device ID to bind.
    */
    void egBindDevice(EGDevice device);

    /*!
        Swap the back buffers. Must call this at the end of a frame. The model
        and state stack created using egPush, are set back to zero.
    */
    void egSwap();

    /*!
        Perform post processing. This can be called multiple times per frame.
        This is needed to be called at least once before egSwap otherwise
        nothing will be displayed.
    */
    void egPostProcess();

    /*!
        Specifies clear values for the color buffers.

        \param r The red value that egClear uses to clear the color buffers. 
        The default value is zero.

        \param g The Green value that egClear uses to clear the color buffers.
        The default value is zero.

        \param b The Blue value that egClear uses to clear the color buffers.
        The default value is zero.

        \param a The Alpha value that egClear uses to clear the color buffers.
        The default value is zero.
    */
    void egClearColor(float r, float g, float b, float a);

    /*!
        Sets the current clear color from an already existing array of color 
        values.

        \param pColor A pointer to an array that contains red, green, and blue 
        values.
    */
    void egClearColorv(const float *pColor);

    /*!
        Specifies the clear value for the depth buffer

        \param depth The depth value used when the depth buffer is cleared.
    */
    void egClearDepth(float depth);

    /*!
        Specifies the clear value for the stencil buffer.

        \param s The index used when the stencil buffer is cleared. The default
        value is zero.
    */
    void egClearStencil(uint8_t s);

    /*!
        Clears buffers to preset values.

        \param mask Bitwise OR operators of masks that indicate the buffers to 
        be cleared. EG_CLEAR_BITFIELD
    */
    void egClear(uint32_t mask);

    /*!
        Setup 2D view and projection matrices. The view dimensions are 
        defined by the current viewport dimensions.

        \param nearClip The distances to the nearer depth clipping plane. This
        distance is negative if the plane is to be behind the viewer.

        \param farClip The distances to the farther depth clipping plane. This 
        distance is negative if the plane is to be behind the viewer.
    */
    void egSet2DViewProj(float nearClip, float farClip);

    /*!
        Setup 3D view and projection matrices. Aspect ratio is calculated
        from the current viewport dimensions.

        \param eyeX The position of the eye point.

        \param eyeY The position of the eye point.

        \param eyeZ The position of the eye point.
        
        \param centerX The position of the reference point.

        \param centerY The position of the reference point.

        \param centerZ The position of the reference point.

        \param upX The direction of the up vector.

        \param upY The direction of the up vector.

        \param upZ The direction of the up vector.
        
        \param fovy The field of view angle, in degrees, in the y-direction.
        
        \param nearClip The distance from the viewer to the near clipping plane
        (always positive).
        
        \param farClip The distance from the viewer to the far clipping plane
        (always positive).
    */
    void egSet3DViewProj(float eyeX, float eyeY, float eyeZ, 
                         float centerX, float centerY, float centerZ, 
                         float upX, float upY, float upZ, 
                         float fovy, float nearClip, float farClip);

    /*!
        Unimplemented
    */
    void egSetIsometricViewProj();

    /*!
        Manually set the view and projection matrix.

        \param pView A pointer to a 4x4 view matrix as 16 consecutive values.
        
        \param pProj A pointer to a 4x4 projection matrix as 16 consecutive 
        values.
    */
    void egSetViewProj(const float *pView, const float *pProj);

    void egSetViewProjMerged(const float *pViewProj);

    /*!
        Get the view matrix

        \param pView Pointer to an array of 16 floats that will contain
        the matrix.
    */
    void egGetView(float *pView);

    /*!
        Get the projection matrix

        \param pProjection Pointer to an array of 16 floats that will contain
        the matrix.
    */
    void egGetProjection(float *pProjection);

    /*!
        Get the current model matrix

        \param pModel Pointer to an array of 16 floats that will contain
        the matrix.
    */
    void egGetModel(float *pModel);

    /*!
        Sets the viewport.

        \param x The lower-left corner of the viewport rectangle, in pixels. 
        The default is (0,0).

        \param y The lower-left corner of the viewport rectangle, in pixels.
        The default is (0,0).

        \param width The width of the viewport. When an EasyGraphix context is
        first created, width and height are set to the dimensions of the 
        backbuffer.

        \param height The height of the viewport. When an EasyGraphix context 
        is first created, width and height are set to the dimensions of the 
        backbuffer.
    */
    void egViewPort(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

    void egResize();

    /*!
        Get the current viewport

        \param pViewport Pointer to an array of 4 uint32_t that will contain
        the viewport.
    */
    void egGetViewport(uint32_t *pViewport);

    /*!
        Defines the scissor box.

        \param x The x (vertical axis) coordinate for the upper-left corner of 
        the scissor box.

        \param y The y (horizontal axis) coordinate for the upper-left corner of 
        the scissor box. Together, x and y specify the upper-left corner of the
        scissor box. Initially (0,0).

        \param width The width of the scissor box.

        \param height The height of the scissor box. When an EasyGraphix 
        context is first created, width and height are set to the dimensions of
        the backbuffer.
    */
    void egScissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

    /*!
        Replaces the current model matrix with the identity matrix.
    */
    void egModelIdentity();

    /*!
        Multiplies the current model matrix by a translation matrix

        \param x The x coordinate of a translation vector.

        \param y The y coordinate of a translation vector.

        \param z The z coordinate of a translation vector.
    */
    void egModelTranslate(float x, float y, float z);

    /*!
        Multiplies the current model matrix by a translation matrix

        \param pAxis A pointer to an array that contains x, y, z values.
    */
    void egModelTranslatev(const float *pAxis);

    /*!
        Multiply the current model matrix by an arbitrary matrix

        \param pMatrix A pointer to a 4x4 matrix stored as 16 consecutive 
        values.
    */
    void egModelMult(const float *pMatrix);

    /*!
        Multiplies the current model matrix by a rotation matrix.

        \param angle The angle of rotation, in degrees.

        \param x The x coordinate of a vector.

        \param y The y coordinate of a vector.

        \param z The z coordinate of a vector.
    */
    void egModelRotate(float angle, float x, float y, float z);

    /*!
        Multiplies the current model matrix by a rotation matrix.

        \param angle The angle of rotation, in degrees.

        \param pAxis A pointer to an array that contains x, y, z values.
    */
    void egModelRotatev(float angle, const float *pAxis);

    /*!
        Multiply the current model matrix by a general scaling matrix.

        \param x Scale factors along the x axis.

        \param y Scale factors along the y axis.

        \param z Scale factors along the z axis.
    */
    void egModelScale(float x, float y, float z);

    /*!
        Multiply the current model matrix by a general scaling matrix.

        \param pAxis A pointer to an array that contains x, y, z values.
    */
    void egModelScalev(const float *pAxis);

    /*!
        Push the current model matrix stack.
    */
    void egModelPush();

    /*!
        Pop the current model matrix stack.
    */
    void egModelPop();

    /*!
        Delimits the vertices of a primitive or a group of like primitives

        \param mode The primitive, primitives or effect that will be created
        from vertices presented between egBegin and the subsequent egEnd.
    */
    void egBegin(EG_MODE mode);

    /*!
        Delimits the end of a primitive or a group of like primitives
    */
    void egEnd();

    /*!
        Sets the current color. The alpha will be set to 1

        \param r The new red value for the current color.

        \param g The new green value for the current color.

        \param b The new blue value for the current color.
    */
    void egColor3(float r, float g, float b);

    /*!
        Sets the current color. The alpha will be set to 1

        \param pRGB A pointer to an array that contains red, green, and blue 
        values.
    */
    void egColor3v(const float *pRGB);

    /*!
        Sets the current color.

        \param r The new red value for the current color.

        \param g The new green value for the current color.

        \param b The new blue value for the current color.
        
        \param a The new alpha value for the current color.
    */
    void egColor4(float r, float g, float b, float a);

    /*!
        Sets the current color.

        \param pRGBA A pointer to an array that contains red, green, blue and 
        alpha values.
    */
    void egColor4v(const float *pRGBA);

    /*! 
        Sets the current normal vector

        \param nx Specifies the x-coordinate for the new current normal vector

        \param ny Specifies the y-coordinate for the new current normal vector

        \param nz Specifies the z-coordinate for the new current normal vector
    */
    void egNormal(float nx, float ny, float nz);

    /*!
        Sets the current normal vector.

        \param pNormal A pointer to an array of three elements: the x, y, and z
        coordinates of the new current normal.
    */
    void egNormalv(const float *pNormal);

    /*! 
        Sets the current tangent vector

        \param nx Specifies the x-coordinate for the new current tangent vector

        \param ny Specifies the y-coordinate for the new current tangent vector

        \param nz Specifies the z-coordinate for the new current tangent vector
    */
    void egTangent(float nx, float ny, float nz);

    /*!
        Sets the current tangent vector.

        \param pTangent A pointer to an array of three elements: the x, y, and 
        z coordinates of the new current tangent.
    */
    void egTangentv(const float *pTangent);

    /*! 
        Sets the current binormal vector

        \param nx Specifies the x-coordinate for the new current binormal 
        vector

        \param ny Specifies the y-coordinate for the new current binormal 
        vector

        \param nz Specifies the z-coordinate for the new current binormal 
        vector
    */
    void egBinormal(float nx, float ny, float nz);

    /*!
        Sets the current binormal vector.

        \param pBinormal A pointer to an array of three elements: the x, y, and
        z coordinates of the new current binormal.
    */
    void egBinormalv(const float *pBinormal);

    /*!
        Sets the current texture coordinates.

        \param s The s texture coordinate.

        \param t The t texture coordinate.
    */
    void egTexCoord(float s, float t);

    /*!
        Sets the current texture coordinates.

        \param pTexCoord A pointer to an array of two elements, which in turn 
        specifies the s, and t texture coordinates.
    */
    void egTexCoordv(const float *pTexCoord);

    /*!
        Specifies a vertex. z is set to 0.

        \param x Specifies the x-coordinate of a vertex.

        \param y Specifies the y-coordinate of a vertex.
    */
    void egPosition2(float x, float y);

    /*!
        Specifies a vertex. z is set to 0.

        \param pPos A pointer to an array of two elements. The elements are the
        x and y coordinates of a vertex.
    */
    void egPosition2v(const float *pPos);

    /*!
        Specifies a vertex.

        \param x Specifies the x-coordinate of a vertex.

        \param y Specifies the y-coordinate of a vertex.

        \param z Specifies the z-coordinate of a vertex.
    */
    void egPosition3(float x, float y, float z);

    /*!
        Specifies a vertex.

        \param pPos A pointer to an array of three elements. The elements are 
        the x, y and z coordinates of a vertex.
    */
    void egPosition3v(const float *pPos);

    /*!
        Specifies a target position. z is set to 0.

        \param x Specifies the x-coordinate of a target.

        \param y Specifies the y-coordinate of a target.

        \note Only used by following modes: EG_SPOT, EG_DIRECTIONAL
    */
    void egTarget2(float x, float y);

    /*!
        Specifies a target position. z is set to 0.

        \param pPos A pointer to an array of two elements. The elements are the
        x and y coordinates of a target.

        \note Only used by following modes: EG_SPOT, EG_DIRECTIONAL
    */
    void egTarget2v(const float *pPos);

    /*!
        Specifies a target position.

        \param x Specifies the x-coordinate of a target.

        \param y Specifies the y-coordinate of a target.

        \param z Specifies the z-coordinate of a target.

        \defails Only used by following modes: EG_SPOT, EG_DIRECTIONAL
    */
    void egTarget3(float x, float y, float z);

    /*!
        Specifies a target position.

        \param pPos A pointer to an array of three elements. The elements are 
        the x, y and z coordinates of a target.

        \defails Only used by following modes: EG_SPOT, EG_DIRECTIONAL
    */
    void egTarget3v(const float *pPos);

    /*!
        Specifies a radius.

        \param radius Specifies the radius.

        \defails Only used by following modes: EG_OMNIS, EG_SPOT, 
        EG_DIRECTIONAL
    */
    void egRadius(float radius);

    /*!
        Specifies a radius.

        \param innerRadius Specifies the inner radius. This is the distance the
        light falloff starts.

        \param radius Specifies the outterRadius radius. The final falloff
        distance

        \defails Only used by following modes: EG_OMNIS, EG_SPOT, 
        EG_DIRECTIONAL
    */
    void egRadius2(float innerRadius, float outterRadius);

    /*!
        Specifies a radius.

        \param pRadius A pointer to an array of two elements. The elements are
        the inner and outter radius.

        \defails Only used by following modes: EG_OMNIS, EG_SPOT, 
        EG_DIRECTIONAL
    */
    void egRadius2v(const float *pRadius);

    /*!
        Specifies the falloff exponent of a light.

        \param exponent The falloff exponent

        \details The default value is 1. This is applied on the normalized
        distance of a pixel to the light d, as such: 
        light intensity = pow(d, exponent);
    */
    void egFalloffExponent(float exponent);

    /*!
        Specifies a multiplyer to the light intensity.

        \param multiply Multipyer value. Default 1.

        \details Before a light is applied to pixel, they are multiplied by
        this as such:
        lightColor = lightColor * multiply;
    */
    void egMultiply(float multiply);

    /*!
        Unimplemented

        \return 0
    */
    EGTexture egCreateTexture1D(uint32_t dimension, 
                                const void *pData, 
                                EGFormat dataFormat);

    /*!
        Create a 2D texture.

        \param width Width of the texture.

        \param height Height of the texture

        \param pData Data for the textre.

        \param dataType Format of pData. This is not the format of the final
        texture. Textures are always 32 bits RGBA.

        \param flags Texture flags

        \return The texture ID or 0 if failed.
    */
    EGTexture egCreateTexture2D(uint32_t width, uint32_t height, 
                                const void *pData, EGFormat dataFormat,
                                EG_TEXTURE_FLAGS flags);

    /*!
        Unimplemented

        \return 0
    */
    EGTexture egCreateTexture3D(uint32_t width, uint32_t height, 
                                uint32_t depth, 
                                const void *pData, EGFormat dataFormat);

    /*!
        Unimplemented

        \return 0
    */
    EGTexture egCreateCubeMap(uint32_t dimension, 
                              const void *pData, EGFormat dataFormat, 
                              EG_TEXTURE_FLAGS flags);

    /*!
        Destroys a texture

        \param Pointer to a texture ID. It will be set to 0 upon success.
    */
    void egDestroyTexture(EGTexture *pTexture);

    /*!
        Bind diffuse texture.

        \param texture Texture ID. If 0 is passed, default white texture 
        will be bound.
    */
    void egBindDiffuse(EGTexture texture);

    /*!
        Bind normal map.

        \param texture Texture ID. If 0 is passed, default flat normal map 
        texture will be bound.

        \details The RGB component of the texture represents the normal xyz.
        The alpha component is the ambient occlusion value.
    */
    void egBindNormal(EGTexture texture);

    /*!
        Bind material map.

        \param texture Texture ID. If 0 is passed, default material texture
        will be bound without specular or self illumination.

        \details The red component is the specular intensity. The green 
        component is the shininess. Specular is computed as such:
        finalColor += materialTexture.r * lightColor * 
        pow(specularAmount, materialTexture.g * 100);

        The blue component represents the self illumination.
    */
    void egBindMaterial(EGTexture texture);

    /*!
        Enable EasyGraphix capabilities.

        \param stateBits A symbolic constant indicating an EasyGraphix 
        capability.
    */
    void egEnable(EGEnable stateBits);

    /*!
        Disable EasyGraphix capabilities.

        \param stateBits A symbolic constant indicating an EasyGraphix 
        capability.
    */
    void egDisable(EGEnable stateBits);

    /*!
        Pushes the state stack

        \details Everything that can be enabled/disabled in EG_ENABLE and
        their associated data can be pushed.
    */
    void egStatePush();

    /*!
        Pops the state stack

        \details Everything that can be enabled/disabled in EG_ENABLE and
        their associated data can be poped.
    */
    void egStatePop();

    /*!
        Set the alpha test threashold.
    */

    /*!
        Specifies pixel arithmetic.

        \param sfactor Specifies how the red, green, blue, and alpha 
        source-blending factors are computed.

        \param dfactor Specifies how the red, green, blue, and alpha 
        destination-blending factors are computed.

        \details The equation for blending is as such: 
        finalColor = (sourceRGBA * sfactor) + (destinationRGBA * dfactor)
    */
    void egBlendFunc(EG_BLEND_FACTOR sfactor, EG_BLEND_FACTOR dfactor);

    /*!
        Define front- and back-facing polygons. Default is Counter clockwise.

        \param mode Specifies the orientation of front-facing polygons. EG_CW 
        and EG_CCW are accepted. The initial value is EG_CCW.
    */
    void egFrontFace(EG_FRONT_FACE mode);

    /*!
        Draw a cube primitive.

        \param size The cube will be size*size*size, centered.
    */
    void egCube(float size);

    /*!
        Draw a sphere primitive

        \param radius Radius of the sphere

        \slices Side count, horizontal separations

        \stacks Number of vertical separations

        \sfactor Multiplyer to s texture coordinates
    */
    void egSphere(float radius, uint32_t slices, uint32_t stacks, 
                  float sfactor);

    /*!
        Draw a cylinder primitive

        \param bottomRadius Radius of the bottom cap

        \param bottomRadius Radius of the top cap

        \height Height of the cylinder

        \slices Side count, horizontal separations

        \sfactor Multiplyer to s texture coordinates
    */
    void egCylinder(float bottomRadius, float topRadius, 
                    float height, uint32_t slices, float sfactor);

    /*!
        Draw a tube

        \param outterRadius Radius outside of the tube

        \param innerRadius Radius of the tube's hole

        \height Height of the tube

        \slices Side count, horizontal separations

        \sfactor Multiplyer to s texture coordinates
    */
    void egTube(float outterRadius, float innerRadius,
                float height, uint32_t slices, float sfactor);

    /*!
        Set texture filtering mode. Default is 4x Anysotropic.

        \param filter Filter mode
    */
    void egFilter(EG_FILTER filter);

    /*!
        Specify the alpha test function.

        \param func Specifies the alpha comparison function. Symbolic constants
        EG_NEVER, EG_LESS, EG_EQUAL, EG_LEQUAL, EG_GREATER, EG_NOTEQUAL, 
        EG_GEQUAL, and EG_ALWAYS are accepted. The initial value is EG_LEQUAL.

        \param ref Specifies the reference value that incoming alpha values are
        compared to. This value is clamped to the range [0, 1], where 0 
        represents the lowest possible alpha value and 1 the highest possible 
        value. The initial reference value is 0.5f.
    */
    void egAlphaFunc(EG_COMPARE func, float ref);

    /*!
        Specify the value used for depth buffer comparisons

        \param func Specifies the depth comparison function. Symbolic constants
        EG_NEVER, EG_LESS, EG_EQUAL, EG_LEQUAL, EG_GREATER, EG_NOTEQUAL,
        EG_GEQUAL, and EG_ALWAYS are accepted. The initial value is EG_LESS.
        */
    void egDepthFunc(EG_COMPARE func);

    /*!
        Specify bloom properties.
    */
    void egBloom(int, int samples);

    /*!
        Specify blur properties

        \param spread Spread value. 0 will mean no blur.
    */
    void egBlur(float spread);

    /*!
        Create a state object based on the current state of EasyGraphix.
        This includes everything in EG_ENABLE bits and their properties.

        \return Created state object, or 0 if failed.
    */
    EGState egCreateState();

    /*!
        Destroy a state

        \param pState Pointer to a state object. It will be set to 0 if 
        successful
    */
    void egDestroyState(EGState *pState);

    /*!
        Bind a state

        \param state State object to bind.
    */
    void egBindState(EGState state);

    /*!
        Set vignette exponent

        \param exponent Higher the value, the more screen 
        coverage you get. Default is 4.
    */
    void egVignette(float exponent);

    /*!
        Set depth of field focus points
    */
    void egDepthOfField(float nearStart, float nearEnd,
                        float farStart, float farEnd);

    /*!
        Generic get function for int arrays
    */
    void egGetiv(EGGet what, int *out);

#ifdef __cplusplus
}       /* extern "C" */
#endif  /* __cplusplus */

#endif  /* EG_H_INCLUDED */

//--- Todo
// EGState egCreateState();
// void egDestroyState(EGState *pState);
// void egSetState(EGState state);
