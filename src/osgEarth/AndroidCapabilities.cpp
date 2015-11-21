/* -*-c++-*- */
/* osgEarth - Dynamic map generation toolkit for OpenSceneGraph
 * Copyright 2008-2012 Pelican Mapping
 * http://osgearth.org
 *
 * osgEarth is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */
#include <osgEarth/AndroidCapabilities>
#include <osg/FragmentProgram>
#include <osg/GraphicsContext>
#include <osg/GL>
#include <osg/GLExtensions>
#include <osg/GL2Extensions>
#include <osg/Texture>
#include <osgViewer/Version>

using namespace osgEarth;

#define LC "[AndroidCapabilities] "


// ---------------------------------------------------------------------------

#define SAYBOOL(X) (X?"yes":"no")

AndroidCapabilities::AndroidCapabilities() :
Capabilities()
{
    //if ( mgc.valid() )
    {
        //osg::GraphicsContext* gc = mgc._gc.get();
        unsigned int id = 0;//gc->getState()->getContextID();
        const osg::GL2Extensions* GL2 = osg::GL2Extensions::Get( id, true );

        OE_INFO << LC << "Detected hardware capabilities:" << std::endl;

        _vendor = std::string( reinterpret_cast<const char*>(glGetString(GL_VENDOR)) );
        OE_INFO << LC << "  Vendor = " << _vendor << std::endl;

        _renderer = std::string( reinterpret_cast<const char*>(glGetString(GL_RENDERER)) );
        OE_INFO << LC << "  Renderer = " << _renderer << std::endl;

        _version = std::string( reinterpret_cast<const char*>(glGetString(GL_VERSION)) );
        OE_INFO << LC << "  Version = " << _version << std::endl;

        glGetIntegerv( GL_MAX_TEXTURE_UNITS, &_maxFFPTextureUnits );
        OE_INFO << LC << "  Max FFP texture units = " << _maxFFPTextureUnits << std::endl;

        glGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &_maxGPUTextureUnits );
        OE_INFO << LC << "  Max GPU texture units = " << _maxGPUTextureUnits << std::endl;

        glGetIntegerv( GL_MAX_TEXTURE_COORDS_ARB, &_maxGPUTextureCoordSets );
#if defined(OSG_GLES2_AVAILABLE)
        int maxVertAttributes = 0;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertAttributes);
        _maxGPUTextureCoordSets = maxVertAttributes - 5; //-5 for vertex, normal, color, tangent and binormal
#endif
        OE_INFO << LC << "  Max GPU texture coordinate sets = " << _maxGPUTextureCoordSets << std::endl;


        glGetIntegerv( GL_DEPTH_BITS, &_depthBits );
        OE_INFO << LC << "  Depth buffer bits = " << _depthBits << std::endl;

        
        glGetIntegerv( GL_MAX_TEXTURE_SIZE, &_maxTextureSize );
#if !(defined(OSG_GLES1_AVAILABLE) || defined(OSG_GLES2_AVAILABLE))
        // Use the texture-proxy method to determine the maximum texture size 
        for( int s = _maxTextureSize; s > 2; s >>= 1 )
        {
            glTexImage2D( GL_PROXY_TEXTURE_2D, 0, GL_RGBA8, s, s, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0L );
            GLint width = 0;
            glGetTexLevelParameteriv( GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width );
            if ( width == s )
            {
                _maxTextureSize = s;
                break;
            }
        }
#endif
        OE_INFO << LC << "  Max texture size = " << _maxTextureSize << std::endl;

        //PORT@tom, what effect will this have?
#ifdef OSG_GL_FIXED_FUNCTION_AVAILABLE
        glGetIntegerv( GL_MAX_LIGHTS, &_maxLights );
#else
        _maxLights = 1;
#endif
        OE_INFO << LC << "  Max lights = " << _maxLights << std::endl;

        
        if ( ::getenv("OSGEARTH_NO_GLSL") )
            _supportsGLSL = false;
        else
            _supportsGLSL = GL2->isGlslSupported();
        OE_INFO << LC << "  GLSL = " << SAYBOOL(_supportsGLSL) << std::endl;

        if ( _supportsGLSL )
        {
            _GLSLversion = GL2->getLanguageVersion();
            OE_INFO << LC << "  GLSL Version = " << _GLSLversion << std::endl;
        }

        _supportsTextureArrays = 
            _supportsGLSL &&
            osg::getGLVersionNumber() >= 2.0 && // hopefully this will detect Intel cards
            osg::isGLExtensionSupported( id, "GL_EXT_texture_array" );
        OE_INFO << LC << "  Texture arrays = " << SAYBOOL(_supportsTextureArrays) << std::endl;

        _supportsTexture3D = osg::isGLExtensionSupported( id, "GL_EXT_texture3D" );
        OE_INFO << LC << "  3D textures = " << SAYBOOL(_supportsTexture3D) << std::endl;

        _supportsMultiTexture = 
            osg::getGLVersionNumber() >= 1.3 ||
            osg::isGLExtensionSupported( id, "GL_ARB_multitexture") ||
            osg::isGLExtensionSupported( id, "GL_EXT_multitexture" );
        OE_INFO << LC << "  Multitexturing = " << SAYBOOL(_supportsMultiTexture) << std::endl;

        _supportsStencilWrap = osg::isGLExtensionSupported( id, "GL_EXT_stencil_wrap" );
        OE_INFO << LC << "  Stencil wrapping = " << SAYBOOL(_supportsStencilWrap) << std::endl;

        _supportsTwoSidedStencil = osg::isGLExtensionSupported( id, "GL_EXT_stencil_two_side" );
        OE_INFO << LC << "  2-sided stencils = " << SAYBOOL(_supportsTwoSidedStencil) << std::endl;

        _supportsDepthPackedStencilBuffer = osg::isGLExtensionSupported( id, "GL_EXT_packed_depth_stencil" ) || 
                                            osg::isGLExtensionSupported( id, "GL_OES_packed_depth_stencil" );
        OE_INFO << LC << "  depth-packed stencil = " << SAYBOOL(_supportsDepthPackedStencilBuffer) << std::endl;

        _supportsOcclusionQuery = osg::isGLExtensionSupported( id, "GL_ARB_occlusion_query" );
        OE_INFO << LC << "  occlusion query = " << SAYBOOL(_supportsOcclusionQuery) << std::endl;

        _supportsDrawInstanced = 
            _supportsGLSL &&
            osg::isGLExtensionOrVersionSupported( id, "GL_EXT_draw_instanced", 3.1f );
        OE_INFO << LC << "  draw instanced = " << SAYBOOL(_supportsDrawInstanced) << std::endl;

        glGetIntegerv( GL_MAX_UNIFORM_BLOCK_SIZE, &_maxUniformBlockSize );
        OE_INFO << LC << "  max uniform block size = " << _maxUniformBlockSize << std::endl;

        _supportsUniformBufferObjects = 
            _supportsGLSL &&
            osg::isGLExtensionOrVersionSupported( id, "GL_ARB_uniform_buffer_object", 2.0f );
        OE_INFO << LC << "  uniform buffer objects = " << SAYBOOL(_supportsUniformBufferObjects) << std::endl;

        if ( _supportsUniformBufferObjects && _maxUniformBlockSize == 0 )
        {
            OE_INFO << LC << "  ...but disabled, since UBO block size reports zero" << std::endl;
            _supportsUniformBufferObjects = false;
        }


        //_supportsTexture2DLod = osg::isGLExtensionSupported( id, "GL_ARB_shader_texture_lod" );
        //OE_INFO << LC << "  texture2DLod = " << SAYBOOL(_supportsTexture2DLod) << std::endl;

        // ATI workarounds:
        bool isATI = _vendor.find("ATI ") == 0;

        _supportsMipmappedTextureUpdates = false;//isATI && enableATIworkarounds ? false : true;
        OE_INFO << LC << "  Mipmapped texture updates = " << SAYBOOL(_supportsMipmappedTextureUpdates) << std::endl;

#if 0
        // Intel workarounds:
        bool isIntel = 
            _vendor.find("Intel ")   != std::string::npos ||
            _vendor.find("Intel(R)") != std::string::npos ||
            _vendor.compare("Intel") == 0;
#endif

        _maxFastTextureSize = _maxTextureSize;

        OE_INFO << LC << "  Max Fast Texture Size = " << _maxFastTextureSize << std::endl;
    }
}
