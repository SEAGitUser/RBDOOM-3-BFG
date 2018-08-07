/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2016 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#pragma hdrstop
#include "precompiled.h"

#include "tr_local.h"

// RB begin
#if defined(_WIN32)

// Vista OpenGL wrapper check
#include "../sys/win32/win_local.h"
#endif
// RB end

// foresthale 2014-03-01: fixed custom screenshot resolution by doing a more direct render path
#define BUGFIXEDSCREENSHOTRESOLUTION 1
#ifdef BUGFIXEDSCREENSHOTRESOLUTION
#include "../framework/Common_local.h"
#endif

// DeviceContext bypasses RenderSystem to work directly with this
idRenderModelGui* tr_guiModel;

// functions that are not called every frame
glconfig_t	glConfig;

idCVar r_requestStereoPixelFormat( "r_requestStereoPixelFormat", "1", CVAR_RENDERER, "Ask for a stereo GL pixel format on startup" );
idCVar r_debugContext( "r_debugContext", "2", CVAR_RENDERER, "Enable various levels of context debug." );
idCVar r_glDriver( "r_glDriver", "", CVAR_RENDERER, "\"opengl32\", etc." );
idCVar r_skipIntelWorkarounds( "r_skipIntelWorkarounds", "0", CVAR_RENDERER | CVAR_BOOL, "skip workarounds for Intel driver bugs" );
// RB: disabled 16x MSAA
idCVar r_antiAliasing( "r_antiAliasing", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, " 0 = None\n 1 = SMAA 1x\n 2 = MSAA 2x\n 3 = MSAA 4x\n 4 = MSAA 8x\n", 0, ANTI_ALIASING_MSAA_8X );
// RB end
idCVar r_vidMode( "r_vidMode", "0", CVAR_ARCHIVE | CVAR_RENDERER | CVAR_INTEGER, "fullscreen video mode number" );
idCVar r_displayRefresh( "r_displayRefresh", "0", CVAR_RENDERER | CVAR_INTEGER | CVAR_NOCHEAT, "optional display refresh rate option for vid mode", 0.0f, 240.0f );
#ifdef WIN32
idCVar r_fullscreen( "r_fullscreen", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "0 = windowed, 1 = full screen on monitor 1, 2 = full screen on monitor 2, etc" );
#else
// DG: add mode -2 for SDL, also defaulting to windowed mode, as that causes less trouble on linux
idCVar r_fullscreen( "r_fullscreen", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "-2 = use current monitor, -1 = (reserved), 0 = windowed, 1 = full screen on monitor 1, 2 = full screen on monitor 2, etc" );
// DG end
#endif
idCVar r_customWidth( "r_customWidth", "1280", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "custom screen width. set r_vidMode to -1 to activate" );
idCVar r_customHeight( "r_customHeight", "720", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "custom screen height. set r_vidMode to -1 to activate" );
idCVar r_windowX( "r_windowX", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "Non-fullscreen parameter" );
idCVar r_windowY( "r_windowY", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "Non-fullscreen parameter" );
idCVar r_windowWidth( "r_windowWidth", "1280", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "Non-fullscreen parameter" );
idCVar r_windowHeight( "r_windowHeight", "720", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "Non-fullscreen parameter" );

idCVar r_useViewBypass( "r_useViewBypass", "1", CVAR_RENDERER | CVAR_INTEGER, "bypass a frame of latency to the view" );
idCVar r_useLightPortalFlow( "r_useLightPortalFlow", "1", CVAR_RENDERER | CVAR_BOOL, "use a more precise area reference determination" );
idCVar r_singleTriangle( "r_singleTriangle", "0", CVAR_RENDERER | CVAR_BOOL, "only draw a single triangle per primitive" );
idCVar r_checkBounds( "r_checkBounds", "0", CVAR_RENDERER | CVAR_BOOL, "compare all surface bounds with precalculated ones" );
idCVar r_useConstantMaterials( "r_useConstantMaterials", "1", CVAR_RENDERER | CVAR_BOOL, "use pre-calculated material registers if possible" );
idCVar r_useSilRemap( "r_useSilRemap", "1", CVAR_RENDERER | CVAR_BOOL, "consider verts with the same XYZ, but different ST the same for shadows" );
idCVar r_useNodeCommonChildren( "r_useNodeCommonChildren", "1", CVAR_RENDERER | CVAR_BOOL, "stop pushing reference bounds early when possible" );
idCVar r_useShadowSurfaceScissor( "r_useShadowSurfaceScissor", "1", CVAR_RENDERER | CVAR_BOOL, "scissor shadows by the scissor rect of the interaction surfaces" );
idCVar r_useCachedDynamicModels( "r_useCachedDynamicModels", "1", CVAR_RENDERER | CVAR_BOOL, "cache snapshots of dynamic models" );
idCVar r_useSeamlessCubeMap( "r_useSeamlessCubeMap", "1", CVAR_RENDERER | CVAR_BOOL, "use ARB_seamless_cube_map if available" );
idCVar r_useSRGB( "r_useSRGB", "0", CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE, "1 = both texture and framebuffer, 2 = framebuffer only, 3 = texture only" );
idCVar r_maxAnisotropicFiltering( "r_maxAnisotropicFiltering", "8", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "limit aniso filtering", 0.0, 16.0 );
idCVar r_useTrilinearFiltering( "r_useTrilinearFiltering", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "Extra quality filtering if no AnisotropicFiltering available else Bilinear" );
// RB: not used anymore
idCVar r_lodBias( "r_lodBias", "0.5", CVAR_RENDERER | CVAR_ARCHIVE, "UNUSED: image lod bias" );
// RB end

idCVar r_ignoreGLErrors( "r_ignoreGLErrors", "0", CVAR_RENDERER | CVAR_BOOL, "ignore GL errors" );
idCVar r_swapInterval( "r_swapInterval", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "0 = tear, 1 = swap-tear where available, 2 = always v-sync" );

idCVar r_gamma( "r_gamma", "1.0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "changes gamma tables", 0.5f, 3.0f );
idCVar r_brightness( "r_brightness", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "changes gamma tables", 0.5f, 2.0f );

idCVar r_skipStaticInteractions( "r_skipStaticInteractions", "0", CVAR_RENDERER | CVAR_BOOL, "skip interactions created at level load" );
idCVar r_skipDynamicInteractions( "r_skipDynamicInteractions", "0", CVAR_RENDERER | CVAR_BOOL, "skip interactions created after level load" );
idCVar r_skipSuppress( "r_skipSuppress", "0", CVAR_RENDERER | CVAR_BOOL, "ignore the per-view suppressions" );
idCVar r_skipPostProcess( "r_skipPostProcess", "0", CVAR_RENDERER | CVAR_BOOL, "skip all post-process renderings" );
idCVar r_skipInteractions( "r_skipInteractions", "0", CVAR_RENDERER | CVAR_BOOL, "skip all light/surface interaction drawing" );
idCVar r_skipDynamicTextures( "r_skipDynamicTextures", "0", CVAR_RENDERER | CVAR_BOOL, "don't dynamically create textures" );
idCVar r_skipCopyTexture( "r_skipCopyTexture", "0", CVAR_RENDERER | CVAR_BOOL, "do all rendering, but don't actually copyTexSubImage2D" );
idCVar r_skipBackEnd( "r_skipBackEnd", "0", CVAR_RENDERER | CVAR_BOOL, "don't draw anything" );
idCVar r_skipRender( "r_skipRender", "0", CVAR_RENDERER | CVAR_BOOL, "skip 3D rendering, but pass 2D" );
// RB begin
idCVar r_skipRenderContext( "r_skipRenderContext", "0", CVAR_RENDERER | CVAR_BOOL, "DISABLED: NULL the rendering context during backend 3D rendering" );
// RB end
idCVar r_skipTranslucent( "r_skipTranslucent", "0", CVAR_RENDERER | CVAR_BOOL, "skip the translucent interaction rendering" );
idCVar r_skipAmbient( "r_skipAmbient", "0", CVAR_RENDERER | CVAR_BOOL, "bypasses all non-interaction drawing" );
idCVar r_skipNewAmbient( "r_skipNewAmbient", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "bypasses all vertex/fragment program ambient drawing" );
idCVar r_skipDeforms( "r_skipDeforms", "0", CVAR_RENDERER | CVAR_BOOL, "leave all deform materials in their original state" );
idCVar r_skipFrontEnd( "r_skipFrontEnd", "0", CVAR_RENDERER | CVAR_BOOL, "bypasses all front end work, but 2D gui rendering still draws" );
idCVar r_skipUpdates( "r_skipUpdates", "0", CVAR_RENDERER | CVAR_BOOL, "1 = don't accept any entity or light updates, making everything static" );
idCVar r_skipDecals( "r_skipDecals", "0", CVAR_RENDERER | CVAR_BOOL, "skip decal surfaces" );
idCVar r_skipOverlays( "r_skipOverlays", "0", CVAR_RENDERER | CVAR_BOOL, "skip overlay surfaces" );
idCVar r_skipSpecular( "r_skipSpecular", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_CHEAT | CVAR_ARCHIVE, "use black for specular1" );
idCVar r_skipBump( "r_skipBump", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "uses a flat surface instead of the bump map" );
idCVar r_skipDiffuse( "r_skipDiffuse", "0", CVAR_RENDERER | CVAR_BOOL, "use black for diffuse" );
idCVar r_skipSubviews( "r_skipSubviews", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = don't render any gui elements on surfaces" );
idCVar r_skipGuiShaders( "r_skipGuiShaders", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = skip all gui elements on surfaces, 2 = skip drawing but still handle events, 3 = draw but skip events", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_skipParticles( "r_skipParticles", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = skip all particle systems", 0, 1, idCmdSystem::ArgCompletion_Integer<0, 1> );
idCVar r_skipShadows( "r_skipShadows", "0", CVAR_RENDERER | CVAR_BOOL  | CVAR_ARCHIVE, "disable shadows" );

idCVar r_useLightPortalCulling( "r_useLightPortalCulling", "1", CVAR_RENDERER | CVAR_INTEGER, "0 = none, 1 = cull frustum corners to plane, 2 = exact clip the frustum faces", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_useLightAreaCulling( "r_useLightAreaCulling", "1", CVAR_RENDERER | CVAR_BOOL, "0 = off, 1 = on" );
idCVar r_useLightScissors( "r_useLightScissors", "3", CVAR_RENDERER | CVAR_INTEGER, "0 = no scissor, 1 = non-clipped scissor, 2 = near-clipped scissor, 3 = fully-clipped scissor", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_useEntityPortalCulling( "r_useEntityPortalCulling", "1", CVAR_RENDERER | CVAR_INTEGER, "0 = none, 1 = cull frustum corners to plane, 2 = exact clip the frustum faces", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_logFile( "r_logFile", "0", CVAR_RENDERER | CVAR_INTEGER, "number of frames to emit GL logs" );
idCVar r_clear( "r_clear", "2", CVAR_RENDERER, "force screen clear every frame, 1 = purple, 2 = black, 'r g b' = custom" );

idCVar r_offsetFactor( "r_offsetFactor", "0", CVAR_RENDERER | CVAR_FLOAT, "polygon offset scale parameter" );
// RB: offset factor was 0, and units were -600 which caused some very ugly polygon offsets on Android so I reverted the values to the same as in Q3A
#if defined(__ANDROID__)
idCVar r_offsetUnits( "r_offsetUnits", "-2", CVAR_RENDERER | CVAR_FLOAT, "polygon offset bias parameter" );
#else
idCVar r_offsetUnits( "r_offsetUnits", "-600", CVAR_RENDERER | CVAR_FLOAT, "polygon offset bias parameter" );
#endif
// RB end

idCVar r_shadowPolygonOffset( "r_shadowPolygonOffset", "-1", CVAR_RENDERER | CVAR_FLOAT, "bias value added to depth test for stencil shadow drawing" );
idCVar r_shadowPolygonFactor( "r_shadowPolygonFactor", "0", CVAR_RENDERER | CVAR_FLOAT, "scale value for stencil shadow drawing" );
idCVar r_subviewOnly( "r_subviewOnly", "0", CVAR_RENDERER | CVAR_BOOL, "1 = don't render main view, allowing subviews to be debugged" );
idCVar r_testGamma( "r_testGamma", "0", CVAR_RENDERER | CVAR_FLOAT, "if > 0 draw a grid pattern to test gamma levels", 0, 195 );
idCVar r_testGammaBias( "r_testGammaBias", "0", CVAR_RENDERER | CVAR_FLOAT, "if > 0 draw a grid pattern to test gamma levels" );
idCVar r_lightScale( "r_lightScale", "3", CVAR_ARCHIVE | CVAR_RENDERER | CVAR_FLOAT, "all light intensities are multiplied by this", 0, 100 );
idCVar r_flareSize( "r_flareSize", "1", CVAR_RENDERER | CVAR_FLOAT, "scale the flare deforms from the material def" );

idCVar r_skipPrelightShadows( "r_skipPrelightShadows", "0", CVAR_RENDERER | CVAR_BOOL, "skip the dmap generated static shadow volumes" );
idCVar r_useLightDepthBounds( "r_useLightDepthBounds", "1", CVAR_RENDERER | CVAR_BOOL, "use depth bounds test on lights to reduce both shadow and interaction fill" );
idCVar r_useShadowDepthBounds( "r_useShadowDepthBounds", "1", CVAR_RENDERER | CVAR_BOOL, "use depth bounds test on individual shadow volumes to reduce shadow fill" );
// RB begin
idCVar r_useHalfLambertLighting( "r_useHalfLambertLighting", "1", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "use Half-Lambert lighting instead of classic Lambert, requires reloadShaders" );
// RB end

idCVar r_screenFraction( "r_screenFraction", "100", CVAR_RENDERER | CVAR_INTEGER, "for testing fill rate, the resolution of the entire screen can be changed" );
idCVar r_usePortals( "r_usePortals", "1", CVAR_RENDERER | CVAR_BOOL, " 1 = use portals to perform area culling, otherwise draw everything" );
idCVar r_singleLight( "r_singleLight", "-1", CVAR_RENDERER | CVAR_INTEGER, "suppress all but one light" );
idCVar r_singleEntity( "r_singleEntity", "-1", CVAR_RENDERER | CVAR_INTEGER, "suppress all but one entity" );
idCVar r_singleSurface( "r_singleSurface", "-1", CVAR_RENDERER | CVAR_INTEGER, "suppress all but one surface on each entity" );
idCVar r_singleArea( "r_singleArea", "0", CVAR_RENDERER | CVAR_BOOL, "only draw the portal area the view is actually in" );
idCVar r_orderIndexes( "r_orderIndexes", "1", CVAR_RENDERER | CVAR_BOOL, "perform index reorganization to optimize vertex use" );
idCVar r_lightAllBackFaces( "r_lightAllBackFaces", "0", CVAR_RENDERER | CVAR_BOOL, "light all the back faces, even when they would be shadowed" );

// visual debugging info
idCVar r_showPortals( "r_showPortals", "0", CVAR_RENDERER | CVAR_BOOL, "draw portal outlines in color based on passed / not passed" );
idCVar r_showUnsmoothedTangents( "r_showUnsmoothedTangents", "0", CVAR_RENDERER | CVAR_BOOL, "if 1, put all nvidia register combiner programming in display lists" );
idCVar r_showSilhouette( "r_showSilhouette", "0", CVAR_RENDERER | CVAR_BOOL, "highlight edges that are casting shadow planes" );
idCVar r_showVertexColor( "r_showVertexColor", "0", CVAR_RENDERER | CVAR_BOOL, "draws all triangles with the solid vertex color" );
idCVar r_showUpdates( "r_showUpdates", "0", CVAR_RENDERER | CVAR_BOOL, "report entity and light updates and ref counts" );
idCVar r_showDemo( "r_showDemo", "0", CVAR_RENDERER | CVAR_BOOL, "report reads and writes to the demo file" );
idCVar r_showDynamic( "r_showDynamic", "0", CVAR_RENDERER | CVAR_BOOL, "report stats on dynamic surface generation" );
idCVar r_showTrace( "r_showTrace", "0", CVAR_RENDERER | CVAR_INTEGER, "show the intersection of an eye trace with the world", idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_showIntensity( "r_showIntensity", "0", CVAR_RENDERER | CVAR_BOOL, "draw the screen colors based on intensity, red = 0, green = 128, blue = 255" );
idCVar r_showLights( "r_showLights", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = just print volumes numbers, highlighting ones covering the view, 2 = also draw planes of each volume, 3 = also draw edges of each volume", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_showShadows( "r_showShadows", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = visualize the stencil shadow volumes, 2 = draw filled in", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_showLightScissors( "r_showLightScissors", "0", CVAR_RENDERER | CVAR_BOOL, "show light scissor rectangles" );
idCVar r_showLightCount( "r_showLightCount", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = colors surfaces based on light count, 2 = also count everything through walls, 3 = also print overdraw", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_showViewEntitys( "r_showViewEntitys", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = displays the bounding boxes of all view models, 2 = print index numbers" );
idCVar r_showTris( "r_showTris", "0", CVAR_RENDERER | CVAR_INTEGER, "enables wireframe rendering of the world, 1 = only draw visible ones, 2 = draw all front facing, 3 = draw all, 4 = draw with alpha", 0, 4, idCmdSystem::ArgCompletion_Integer<0, 4> );
idCVar r_showSurfaceInfo( "r_showSurfaceInfo", "0", CVAR_RENDERER | CVAR_BOOL, "show surface material name under crosshair" );
idCVar r_showNormals( "r_showNormals", "0", CVAR_RENDERER | CVAR_FLOAT, "draws wireframe normals" );
idCVar r_showMemory( "r_showMemory", "0", CVAR_RENDERER | CVAR_BOOL, "print frame memory utilization" );
idCVar r_showCull( "r_showCull", "0", CVAR_RENDERER | CVAR_BOOL, "report sphere and box culling stats" );
idCVar r_showAddModel( "r_showAddModel", "0", CVAR_RENDERER | CVAR_BOOL, "report stats from tr_addModel" );
idCVar r_showDepth( "r_showDepth", "0", CVAR_RENDERER | CVAR_BOOL, "display the contents of the depth buffer and the depth range" );
idCVar r_showSurfaces( "r_showSurfaces", "0", CVAR_RENDERER | CVAR_BOOL, "report surface/light/shadow counts" );
idCVar r_showPrimitives( "r_showPrimitives", "0", CVAR_RENDERER | CVAR_INTEGER, "report drawsurf/index/vertex counts" );
idCVar r_showEdges( "r_showEdges", "0", CVAR_RENDERER | CVAR_BOOL, "draw the sil edges" );
idCVar r_showTexturePolarity( "r_showTexturePolarity", "0", CVAR_RENDERER | CVAR_BOOL, "shade triangles by texture area polarity" );
idCVar r_showTangentSpace( "r_showTangentSpace", "0", CVAR_RENDERER | CVAR_INTEGER, "shade triangles by tangent space, 1 = use 1st tangent vector, 2 = use 2nd tangent vector, 3 = use normal vector", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_showDominantTri( "r_showDominantTri", "0", CVAR_RENDERER | CVAR_BOOL, "draw lines from vertexes to center of dominant triangles" );
idCVar r_showTextureVectors( "r_showTextureVectors", "0", CVAR_RENDERER | CVAR_FLOAT, " if > 0 draw each triangles texture (tangent) vectors" );
idCVar r_showOverDraw( "r_showOverDraw", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = geometry overdraw, 2 = light interaction overdraw, 3 = geometry and light interaction overdraw", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
// RB begin
idCVar r_showShadowMaps( "r_showShadowMaps", "0", CVAR_RENDERER | CVAR_BOOL, "" );
idCVar r_showShadowMapLODs( "r_showShadowMapLODs", "0", CVAR_RENDERER | CVAR_INTEGER, "" );
// RB end

idCVar r_useEntityCallbacks( "r_useEntityCallbacks", "1", CVAR_RENDERER | CVAR_BOOL, "if 0, issue the callback immediately at update time, rather than defering" );

idCVar r_showSkel( "r_showSkel", "0", CVAR_RENDERER | CVAR_INTEGER, "draw the skeleton when model animates, 1 = draw model with skeleton, 2 = draw skeleton only", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_jointNameScale( "r_jointNameScale", "0.02", CVAR_RENDERER | CVAR_FLOAT, "size of joint names when r_showskel is set to 1" );
idCVar r_jointNameOffset( "r_jointNameOffset", "0.5", CVAR_RENDERER | CVAR_FLOAT, "offset of joint names when r_showskel is set to 1" );

idCVar r_debugLineDepthTest( "r_debugLineDepthTest", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "perform depth test on debug lines" );
idCVar r_debugLineWidth( "r_debugLineWidth", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "width of debug lines" );
idCVar r_debugArrowStep( "r_debugArrowStep", "120", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "step size of arrow cone line rotation in degrees", 0, 120 );
idCVar r_debugPolygonFilled( "r_debugPolygonFilled", "1", CVAR_RENDERER | CVAR_BOOL, "draw a filled polygon" );

idCVar r_materialOverride( "r_materialOverride", "", CVAR_RENDERER, "overrides all materials", idCmdSystem::ArgCompletion_Decl<DECL_MATERIAL> );

idCVar r_debugRenderToTexture( "r_debugRenderToTexture", "0", CVAR_RENDERER | CVAR_INTEGER, "" );

idCVar stereoRender_enable( "stereoRender_enable", "0", CVAR_INTEGER | CVAR_ARCHIVE, "1 = side-by-side compressed, 2 = top and bottom compressed, 3 = side-by-side, 4 = 720 frame packed, 5 = interlaced, 6 = OpenGL quad buffer" );
idCVar stereoRender_swapEyes( "stereoRender_swapEyes", "0", CVAR_BOOL | CVAR_ARCHIVE, "reverse eye adjustments" );
idCVar stereoRender_deGhost( "stereoRender_deGhost", "0.05", CVAR_FLOAT | CVAR_ARCHIVE, "subtract from opposite eye to reduce ghosting" );

idCVar r_useVirtualScreenResolution( "r_useVirtualScreenResolution", "1", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "do 2D rendering at 640x480 and stretch to the current resolution" );

// RB: shadow mapping parameters
idCVar r_useShadowMapping( "r_useShadowMapping", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "use shadow mapping instead of stencil shadows" );
idCVar r_shadowMapFrustumFOV( "r_shadowMapFrustumFOV", "92", CVAR_RENDERER | CVAR_FLOAT, "oversize FOV for point light side matching" );
idCVar r_shadowMapSingleSide( "r_shadowMapSingleSide", "-1", CVAR_RENDERER | CVAR_INTEGER, "only draw a single side (0-5) of point lights" );
idCVar r_shadowMapImageSize( "r_shadowMapImageSize", "1024", CVAR_RENDERER | CVAR_INTEGER, "", 128, 2048 );
idCVar r_shadowMapJitterScale( "r_shadowMapJitterScale", "3", CVAR_RENDERER | CVAR_FLOAT, "scale factor for jitter offset" );
idCVar r_shadowMapBiasScale( "r_shadowMapBiasScale", "0.0001", CVAR_RENDERER | CVAR_FLOAT, "scale factor for jitter bias" );
idCVar r_shadowMapRandomizeJitter( "r_shadowMapRandomizeJitter", "1", CVAR_RENDERER | CVAR_BOOL, "randomly offset jitter texture each draw" );
idCVar r_shadowMapSamples( "r_shadowMapSamples", "1", CVAR_RENDERER | CVAR_INTEGER, "0, 1, 4, or 16" );
idCVar r_shadowMapSplits( "r_shadowMapSplits", "3", CVAR_RENDERER | CVAR_INTEGER, "number of splits for cascaded shadow mapping with parallel lights", 0, 4 );
idCVar r_shadowMapSplitWeight( "r_shadowMapSplitWeight", "0.9", CVAR_RENDERER | CVAR_FLOAT, "" );
idCVar r_shadowMapLodScale( "r_shadowMapLodScale", "1.4", CVAR_RENDERER | CVAR_FLOAT, "" );
idCVar r_shadowMapLodBias( "r_shadowMapLodBias", "0", CVAR_RENDERER | CVAR_INTEGER, "" );
idCVar r_shadowMapPolygonFactor( "r_shadowMapPolygonFactor", "2", CVAR_RENDERER | CVAR_FLOAT, "polygonOffset factor for drawing shadow buffer" );
idCVar r_shadowMapPolygonOffset( "r_shadowMapPolygonOffset", "3000", CVAR_RENDERER | CVAR_FLOAT, "polygonOffset units for drawing shadow buffer" );
idCVar r_shadowMapOccluderFacing( "r_shadowMapOccluderFacing", "2", CVAR_RENDERER | CVAR_INTEGER, "0 = front faces, 1 = back faces, 2 = twosided" );
idCVar r_shadowMapRegularDepthBiasScale( "r_shadowMapRegularDepthBiasScale", "0.999", CVAR_RENDERER | CVAR_FLOAT, "shadowmap bias to fight shadow acne for point and spot lights" );
idCVar r_shadowMapSunDepthBiasScale( "r_shadowMapSunDepthBiasScale", "0.999991", CVAR_RENDERER | CVAR_FLOAT, "shadowmap bias to fight shadow acne for cascaded shadow mapping with parallel lights" );

/*
========================
glBindMultiTextureEXT

As of 2011/09/16 the Intel drivers for "Sandy Bridge" and "Ivy Bridge" integrated graphics do not support this extension.
========================
*/
/*void APIENTRY glBindMultiTextureEXT( GLenum texunit, GLenum target, GLuint texture )
{
	glActiveTextureARB( texunit );
	glBindTexture( target, texture );
}*/


/*
=================
R_CheckExtension
=================
*/
// RB begin
static bool R_CheckExtension( const char* name )
// RB end
{
	if( !strstr( glConfig.extensions_string, name ) )
	{
		common->Printf( "X..%s not found\n", name );
		return false;
	}

	common->Printf( "...using %s\n", name );
	return true;
}


/*
========================
DebugCallback

For ARB_debug_output
========================
*/
// RB: added const to userParam
static void CALLBACK DebugCallback( unsigned int source, unsigned int type,
									unsigned int id, unsigned int severity, int length, const char* message, const void* userParam )
{
	// it probably isn't safe to do an idLib::Printf at this point

	// RB: printf should be thread safe on Linux
#if defined(_WIN32)
	::OutputDebugString( message );
	::OutputDebugString( "\n" );
#else
	printf( "%s\n", message );
#endif
	// RB end
}

/*
=================
R_CheckExtension
=================
*/
bool R_CheckExtension( char* name )
{
	if( !strstr( glConfig.extensions_string, name ) )
	{
		common->Printf( "X..%s not found\n", name );
		return false;
	}

	common->Printf( "...using %s\n", name );
	return true;
}

#define CHECK_EXT( _EXT_ ) ( GLEW_##_EXT_ != 0 )

/*
==================
R_CheckPortableExtensions
==================
*/
// RB: replaced QGL with GLEW
static void R_CheckPortableExtensions()
{
	glConfig.glVersion = atof( glConfig.version_string );
	const char* badVideoCard = idLocalization::GetString( "#str_06780" );
	if( glConfig.glVersion < 2.0f )
	{
		idLib::FatalError( "%s", badVideoCard );
	}

	if( idStr::Icmpn( glConfig.renderer_string, "ATI ", 4 ) == 0 || idStr::Icmpn( glConfig.renderer_string, "AMD ", 4 ) == 0 )
	{
		glConfig.vendor = VENDOR_AMD;
	}
	else if( idStr::Icmpn( glConfig.renderer_string, "NVIDIA", 6 ) == 0 )
	{
		glConfig.vendor = VENDOR_NVIDIA;
	}
	else if( idStr::Icmpn( glConfig.renderer_string, "Intel", 5 ) == 0 )
	{
		glConfig.vendor = VENDOR_INTEL;
	}

	// RB: Mesa support
	if( idStr::Icmpn( glConfig.renderer_string, "Mesa", 4 ) == 0 ||
		idStr::Icmpn( glConfig.renderer_string, "X.org", 5 ) == 0 ||
		idStr::Icmpn( glConfig.renderer_string, "Gallium", 7 ) == 0 ||
		idStr::Cmp( glConfig.vendor_string, "X.Org" ) == 0 ||
	    idStr::Icmpn( glConfig.renderer_string, "llvmpipe", 8 ) == 0 )
	{
		if( glConfig.driverType == GLDRV_OPENGL32_CORE_PROFILE )
		{
			glConfig.driverType = GLDRV_OPENGL_MESA_CORE_PROFILE;
		}
		else {
			glConfig.driverType = GLDRV_OPENGL_MESA;
		}
	}
	// RB end

	// GL_ARB_multitexture
	if( glConfig.driverType != GLDRV_OPENGL3X )
	{
		glConfig.multitextureAvailable = true;
	}
	else {
		glConfig.multitextureAvailable = CHECK_EXT( ARB_multitexture );
	}

	// GL_EXT_direct_state_access
	glConfig.directStateAccess = CHECK_EXT( EXT_direct_state_access );
	/*if( glConfig.directStateAccess )
	{
		glBindMultiTextureEXT = ( PFNGLBINDMULTITEXTUREEXTPROC ) GLimp_ExtensionPointer( "glBindMultiTextureEXT" );
	}
	else {
		glBindMultiTextureEXT = glBindMultiTextureEXT;
	}*/

	// GL_ARB_texture_storage
	glConfig.ARB_texture_storage = CHECK_EXT( ARB_texture_storage );

	// GL_ARB_buffer_storage
	glConfig.ARB_buffer_storage = CHECK_EXT( ARB_buffer_storage );

	// GL_ARB_texture_compression + GL_S3_s3tc
	// DRI drivers may have GL_ARB_texture_compression but no GL_EXT_texture_compression_s3tc
	if( glConfig.driverType == GLDRV_OPENGL_MESA_CORE_PROFILE )
	{
		glConfig.textureCompressionAvailable = true;
	}
	else {
		glConfig.textureCompressionAvailable = CHECK_EXT( ARB_texture_compression ) && CHECK_EXT( EXT_texture_compression_s3tc );
	}
	// GL_EXT_texture_filter_anisotropic
	glConfig.anisotropicFilterAvailable = CHECK_EXT( EXT_texture_filter_anisotropic );
	if( glConfig.anisotropicFilterAvailable )
	{
		glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.maxTextureAnisotropy );
		common->Printf( "   maxTextureAnisotropy: %f\n", glConfig.maxTextureAnisotropy );
	}
	else {
		glConfig.maxTextureAnisotropy = 1;
	}

	// GL_EXT_texture_lod_bias
	// The actual extension is broken as specificed, storing the state in the texture unit instead
	// of the texture object.  The behavior in GL 1.4 is the behavior we use.
	glConfig.textureLODBiasAvailable = ( glConfig.glVersion >= 1.4 || CHECK_EXT( EXT_texture_lod_bias ) );
	if( glConfig.textureLODBiasAvailable )
	{
		common->Printf( "...using %s\n", "GL_EXT_texture_lod_bias" );
	}
	else {
		common->Printf( "X..%s not found\n", "GL_EXT_texture_lod_bias" );
	}

	// GL_ARB_seamless_cube_map
	glConfig.seamlessCubeMapAvailable = CHECK_EXT( ARB_seamless_cube_map );
	r_useSeamlessCubeMap.SetModified();		// the CheckCvars() next frame will enable / disable it

	// GL_ARB_framebuffer_sRGB
	glConfig.sRGBFramebufferAvailable = CHECK_EXT( ARB_framebuffer_sRGB );
	r_useSRGB.SetModified();		// the CheckCvars() next frame will enable / disable it

	// GL_ARB_vertex_buffer_object
	if( glConfig.driverType == GLDRV_OPENGL_MESA_CORE_PROFILE )
	{
		glConfig.vertexBufferObjectAvailable = true;
	}
	else {
		glConfig.vertexBufferObjectAvailable = CHECK_EXT( ARB_vertex_buffer_object );
	}

	// GL_ARB_map_buffer_range, map a section of a buffer object's data store
	//if( glConfig.driverType == GLDRV_OPENGL_MESA_CORE_PROFILE )
	//{
	//    glConfig.mapBufferRangeAvailable = true;
	//}
	//else
	{
		glConfig.mapBufferRangeAvailable = CHECK_EXT( ARB_map_buffer_range );
	}

	// GL_ARB_vertex_array_object
	//if( glConfig.driverType == GLDRV_OPENGL_MESA_CORE_PROFILE )
	//{
	//    glConfig.vertexArrayObjectAvailable = true;
	//}
	//else
	{
		glConfig.vertexArrayObjectAvailable = CHECK_EXT( ARB_vertex_array_object );
	}

	// GL_ARB_draw_elements_base_vertex
	glConfig.drawElementsBaseVertexAvailable = CHECK_EXT( ARB_draw_elements_base_vertex );

	// GL_ARB_vertex_program / GL_ARB_fragment_program
	glConfig.fragmentProgramAvailable = CHECK_EXT( ARB_fragment_program );
	//if( glConfig.fragmentProgramAvailable )
	{
		glGetIntegerv( GL_MAX_TEXTURE_COORDS, ( GLint* )&glConfig.maxTextureCoords );
		glGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS, ( GLint* )&glConfig.maxTextureImageUnits );
	}

	// GLSL, core in OpenGL > 2.0
	glConfig.glslAvailable = ( glConfig.glVersion >= 2.0f );

	// GL_ARB_uniform_buffer_object
	glConfig.uniformBufferAvailable = CHECK_EXT( ARB_uniform_buffer_object );
	if( glConfig.uniformBufferAvailable )
	{
		glGetIntegerv( GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, ( GLint* )&glConfig.uniformBufferOffsetAlignment );
		if( glConfig.uniformBufferOffsetAlignment < 256 ) {
			glConfig.uniformBufferOffsetAlignment = 256;
		}
		glGetIntegerv( GL_MAX_UNIFORM_BLOCK_SIZE, ( GLint* )&glConfig.uniformBufferMaxSize );
	}
	// RB: make GPU skinning optional for weak OpenGL drivers
	glConfig.gpuSkinningAvailable = glConfig.uniformBufferAvailable &&
		( glConfig.driverType == GLDRV_OPENGL3X || glConfig.driverType == GLDRV_OPENGL32_CORE_PROFILE || glConfig.driverType == GLDRV_OPENGL32_COMPATIBILITY_PROFILE );

	// ATI_separate_stencil / OpenGL 2.0 separate stencil
	glConfig.twoSidedStencilAvailable = ( glConfig.glVersion >= 2.0f ) || CHECK_EXT( ATI_separate_stencil );

	// GL_EXT_depth_bounds_test
	glConfig.depthBoundsTestAvailable = CHECK_EXT( EXT_depth_bounds_test );

	// GL_ARB_sync
	glConfig.syncAvailable = CHECK_EXT( ARB_sync ) &&
							 // as of 5/24/2012 (driver version 15.26.12.64.2761) sync objects
							 // do not appear to work for the Intel HD 4000 graphics
							 ( glConfig.vendor != VENDOR_INTEL || r_skipIntelWorkarounds.GetBool() );

	// GL_ARB_occlusion_query
	glConfig.occlusionQueryAvailable = CHECK_EXT( ARB_occlusion_query );

	// GL_ARB_timer_query
	glConfig.timerQueryAvailable = ( CHECK_EXT( ARB_timer_query ) || CHECK_EXT( EXT_timer_query ) )
		&& ( glConfig.vendor != VENDOR_INTEL || r_skipIntelWorkarounds.GetBool() ) && glConfig.driverType != GLDRV_OPENGL_MESA;

	// GREMEDY_string_marker
	glConfig.gremedyStringMarkerAvailable = CHECK_EXT( GREMEDY_string_marker );
	if( glConfig.gremedyStringMarkerAvailable )
	{
		common->Printf( "...using %s\n", "GL_GREMEDY_string_marker" );
	}
	else {
		common->Printf( "X..%s not found\n", "GL_GREMEDY_string_marker" );
	}

	// GL_ARB_framebuffer_object
	glConfig.framebufferObjectAvailable = CHECK_EXT( ARB_framebuffer_object );
	if( glConfig.framebufferObjectAvailable )
	{
		glGetIntegerv( GL_MAX_RENDERBUFFER_SIZE, &glConfig.maxRenderbufferSize );
		glGetIntegerv( GL_MAX_COLOR_ATTACHMENTS, &glConfig.maxColorAttachments );

		common->Printf( "...using %s\n", "GL_ARB_framebuffer_object" );
	}
	else {
		common->Printf( "X..%s not found\n", "GL_ARB_framebuffer_object" );
	}

	// GL_EXT_framebuffer_blit
	glConfig.framebufferBlitAvailable = CHECK_EXT( EXT_framebuffer_blit );
	if( glConfig.framebufferBlitAvailable )
	{
		common->Printf( "...using %s\n", "GL_EXT_framebuffer_blit" );
	}
	else {
		common->Printf( "X..%s not found\n", "GL_EXT_framebuffer_blit" );
	}

	// GL_ARB_debug_output
	glConfig.debugOutputAvailable = CHECK_EXT( ARB_debug_output );
	if( glConfig.debugOutputAvailable )
	{
		if( r_debugContext.GetInteger() >= 1 )
		{
			glDebugMessageCallbackARB( ( GLDEBUGPROCARB ) DebugCallback, NULL );
		}
		if( r_debugContext.GetInteger() >= 2 )
		{
			// force everything to happen in the main thread instead of in a separate driver thread
			glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB );
		}
		if( r_debugContext.GetInteger() >= 3 )
		{
			// enable all the low priority messages
			glDebugMessageControlARB( GL_DONT_CARE,
									  GL_DONT_CARE,
									  GL_DEBUG_SEVERITY_LOW_ARB,
									  0, NULL, true );
		}
	}

	//////////////////////////////////////////////////////////////

	// The maximum number of texels allowed in the texel array of a texture buffer object. Value must be at least 65536.
	glConfig.max_texture_buffer_size = 0;
	glGetIntegerv( GL_MAX_TEXTURE_BUFFER_SIZE, &glConfig.max_texture_buffer_size );

	// The minimum required alignment for texture buffer sizes and offset. The initial value is 1.
	glConfig.texture_buffer_offset_alignment = 0;
	glGetIntegerv( GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT, &glConfig.texture_buffer_offset_alignment );

	//SEA: renderprog support:

	glConfig.explicit_attrib_location		= CHECK_EXT( ARB_explicit_attrib_location );
	glConfig.explicit_uniform_location		= CHECK_EXT( ARB_explicit_uniform_location );
	glConfig.gpu_shader5					= CHECK_EXT( ARB_gpu_shader5 );
	glConfig.shading_language_420pack		= CHECK_EXT( ARB_shading_language_420pack );
	glConfig.cull_distance					= CHECK_EXT( ARB_cull_distance );
	glConfig.shader_image_load_store		= CHECK_EXT( ARB_shader_image_load_store );
	glConfig.vertex_shader_layer			= CHECK_EXT( AMD_vertex_shader_layer );
	glConfig.vertex_shader_viewport_index	= CHECK_EXT( AMD_vertex_shader_viewport_index );
	glConfig.fragment_layer_viewport		= CHECK_EXT( ARB_fragment_layer_viewport );
	glConfig.shader_draw_parameters			= CHECK_EXT( ARB_shader_draw_parameters );
	glConfig.derivative_control				= CHECK_EXT( ARB_derivative_control );
	glConfig.shading_language_packing		= CHECK_EXT( ARB_shading_language_packing );
	glConfig.conservative_depth				= CHECK_EXT( ARB_conservative_depth );
	glConfig.texture_cube_map_array			= CHECK_EXT( ARB_texture_cube_map_array );
	glConfig.sample_shading					= CHECK_EXT( ARB_sample_shading );

	//////////////////////////////////////////////////////////////

	// GL_ARB_vertex_attrib_binding
	glConfig.ARB_vertex_attrib_binding = CHECK_EXT( ARB_vertex_attrib_binding );
	if( glConfig.ARB_vertex_attrib_binding )
	{
		common->Printf( "...using GL_ARB_vertex_attrib_binding\n");
	}

	//////////////////////////////////////////////////////////////

	// GL_ARB_multitexture
	if( !glConfig.multitextureAvailable )
	{
		idLib::Error( "GL_ARB_multitexture not available" );
	}
	// GL_ARB_texture_compression + GL_EXT_texture_compression_s3tc
	if( !glConfig.textureCompressionAvailable )
	{
		idLib::Error( "GL_ARB_texture_compression or GL_EXT_texture_compression_s3tc not available" );
	}
	// GL_ARB_vertex_buffer_object
	if( !glConfig.vertexBufferObjectAvailable )
	{
		idLib::Error( "GL_ARB_vertex_buffer_object not available" );
	}
	// GL_ARB_map_buffer_range
	if( !glConfig.mapBufferRangeAvailable )
	{
		idLib::Error( "GL_ARB_map_buffer_range not available" );
	}
	// GL_ARB_vertex_array_object
	if( !glConfig.vertexArrayObjectAvailable )
	{
		idLib::Error( "GL_ARB_vertex_array_object not available" );
	}
	// GL_ARB_draw_elements_base_vertex
	if( !glConfig.drawElementsBaseVertexAvailable )
	{
		idLib::Error( "GL_ARB_draw_elements_base_vertex not available" );
	}
	// GL_ARB_vertex_program / GL_ARB_fragment_program
	//if( !glConfig.fragmentProgramAvailable )
	//{
	//	idLib::Warning( "GL_ARB_fragment_program not available" );
	//}
	// GLSL
	if( !glConfig.glslAvailable )
	{
		idLib::Error( "GLSL not available" );
	}
	// GL_ARB_uniform_buffer_object
	if( !glConfig.uniformBufferAvailable )
	{
		idLib::Error( "GL_ARB_uniform_buffer_object not available" );
	}
	// GL_EXT_stencil_two_side
	if( !glConfig.twoSidedStencilAvailable )
	{
		idLib::Error( "GL_ATI_separate_stencil not available" );
	}

	//glClipControl( GL_LOWER_LEFT, GL_ZERO_TO_ONE )
	//glClipControl( GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE )
	//glClipControl( GL_LOWER_LEFT, GL_ZERO_TO_ONE )
	//glFrontFace( GL_CW ) GL_CCW

	glGenVertexArrays( 1, &glConfig.empty_vao );
	glBindVertexArray( glConfig.empty_vao );
	if( GLEW_KHR_debug )
	{
		idStrStatic<128> name;
		name.Format( "VAO( %u, 'empty' )", glConfig.empty_vao );
		glObjectLabel( GL_VERTEX_ARRAY, glConfig.empty_vao, name.Length(), name.c_str() );
	}
	glBindVertexArray( GL_NONE );

	// generate one global Vertex Array Object (VAO)
	glGenVertexArrays( 1, &glConfig.global_vao );
	glBindVertexArray( glConfig.global_vao );
	if( GLEW_KHR_debug )
	{
		idStrStatic<128> name;
		name.Format( "VAO( %u, 'global' )", glConfig.global_vao );
		glObjectLabel( GL_VERTEX_ARRAY, glConfig.global_vao, name.Length(), name.c_str() );
	}
}
// RB end



static bool r_initialized = false;

/*
=============================
R_IsInitialized
=============================
*/
bool R_IsInitialized()
{
	return r_initialized;
}

/*
=============================
 R_SetNewMode

	r_fullScreen -1		borderless window at exact desktop coordinates
	r_fullScreen 0		bordered window at exact desktop coordinates
	r_fullScreen 1		fullscreen on monitor 1 at r_vidMode
	r_fullScreen 2		fullscreen on monitor 2 at r_vidMode
	...

	r_vidMode -1		use r_customWidth / r_customHeight, even if they don't appear on the mode list
	r_vidMode 0			use first mode returned by EnumDisplaySettings()
	r_vidMode 1			use second mode returned by EnumDisplaySettings()
	...

	r_displayRefresh 0	don't specify refresh
	r_displayRefresh 70	specify 70 hz, etc
=============================
*/
void R_SetNewMode( const bool fullInit )
{
	// try up to three different configurations

	for( int i = 0 ; i < 3 ; i++ )
	{
		if( i == 0 && stereoRender_enable.GetInteger() != STEREO3D_QUAD_BUFFER )
		{
			continue;		// don't even try for a stereo mode
		}

		glimpParms_t parms;

		if( r_fullscreen.GetInteger() <= 0 )
		{
			// use explicit position / size for window
			parms.x = r_windowX.GetInteger();
			parms.y = r_windowY.GetInteger();
			parms.width = r_windowWidth.GetInteger();
			parms.height = r_windowHeight.GetInteger();
			// may still be -1 to force a borderless window
			parms.fullScreen = r_fullscreen.GetInteger();
			parms.displayHz = 0;		// ignored
		}
		else {
			// get the mode list for this monitor
			idList<vidMode_t> modeList;
			if( !tr.GetModeListForDisplay( r_fullscreen.GetInteger() - 1, modeList ) )
			{
				idLib::Printf( "r_fullscreen reset from %i to 1 because mode list failed.", r_fullscreen.GetInteger() );
				r_fullscreen.SetInteger( 1 );
				tr.GetModeListForDisplay( r_fullscreen.GetInteger() - 1, modeList );
			}
			if( modeList.Num() < 1 )
			{
				idLib::Printf( "Going to safe mode because mode list failed." );
				goto safeMode;
			}

			parms.x = 0;		// ignored
			parms.y = 0;		// ignored
			parms.fullScreen = r_fullscreen.GetInteger();

			// set the parameters we are trying
			if( r_vidMode.GetInteger() < 0 )
			{
				// try forcing a specific mode, even if it isn't on the list
				parms.width = r_customWidth.GetInteger();
				parms.height = r_customHeight.GetInteger();
				parms.displayHz = r_displayRefresh.GetInteger();
			}
			else {
				if( r_vidMode.GetInteger() >= modeList.Num() )
				{
					idLib::Printf( "r_vidMode reset from %i to 0.\n", r_vidMode.GetInteger() );
					r_vidMode.SetInteger( 0 );
				}

				parms.width = modeList[ r_vidMode.GetInteger() ].width;
				parms.height = modeList[ r_vidMode.GetInteger() ].height;
				parms.displayHz = modeList[ r_vidMode.GetInteger() ].displayHz;
			}
		}

		switch( r_antiAliasing.GetInteger() )
		{
			case ANTI_ALIASING_MSAA_2X: parms.multiSamples = 2; break;
			case ANTI_ALIASING_MSAA_4X: parms.multiSamples = 4; break;
			case ANTI_ALIASING_MSAA_8X: parms.multiSamples = 8; break;

			default: parms.multiSamples = 0; break;
		}

		if( i == 0 )
		{
			parms.stereo = ( stereoRender_enable.GetInteger() == STEREO3D_QUAD_BUFFER );
		}
		else {
			parms.stereo = false;
		}

		if( fullInit )
		{
			// create the context as well as setting up the window
			if( GLimp_Init( parms ) )
			{
				// it worked
				break;
			}
		}
		else {
			// just rebuild the window
			if( GLimp_SetScreenParms( parms ) )
			{
				// it worked
				break;
			}
		}

		if( i == 2 )
		{
			common->FatalError( "Unable to initialize OpenGL" );
		}

		if( i == 0 )
		{
			// same settings, no stereo
			continue;
		}

safeMode:
		// if we failed, set everything back to "safe mode"
		// and try again
		r_vidMode.SetInteger( 0 );
		r_fullscreen.SetInteger( 1 );
		r_displayRefresh.SetInteger( 0 );
		r_antiAliasing.SetInteger( 0 );
	}
}

idStr extensions_string;

/*
==================
R_InitOpenGL

	This function is responsible for initializing a valid OpenGL subsystem
	for rendering.  This is done by calling the system specific GLimp_Init,
	which gives us a working OGL subsystem, then setting all necessary openGL
	state, including images, vertex programs, and display lists.

	Changes to the vertex cache size or smp state require a vid_restart.

	If R_IsInitialized() is false, no rendering can take place, but
	all renderSystem functions will still operate properly, notably the material
	and model information functions.
==================
*/
void R_InitOpenGL()
{
	common->Printf( "----- R_InitOpenGL -----\n" );

	if( R_IsInitialized() )
	{
		common->FatalError( "R_InitOpenGL called while active" );
	}

	// DG: make sure SDL has setup video so getting supported modes in R_SetNewMode() works
	GLimp_PreInit();
	// DG end

	R_SetNewMode( true );

	console->Resize();

	// input and sound systems need to be tied to the new window
	sys->InitInput();

	// get our config strings
	glConfig.vendor_string = ( const char* )glGetString( GL_VENDOR );
	glConfig.renderer_string = ( const char* )glGetString( GL_RENDERER );
	glConfig.version_string = ( const char* )glGetString( GL_VERSION );
	glConfig.shading_language_string = ( const char* )glGetString( GL_SHADING_LANGUAGE_VERSION );
	glConfig.extensions_string = ( const char* )glGetString( GL_EXTENSIONS );

	if( glConfig.extensions_string == NULL )
	{
		// As of OpenGL 3.2, glGetStringi is required to obtain the available extensions
		//glGetStringi = ( PFNGLGETSTRINGIPROC )GLimp_ExtensionPointer( "glGetStringi" );

		// Build the extensions string
		GLint numExtensions;
		glGetIntegerv( GL_NUM_EXTENSIONS, &numExtensions );
		extensions_string.Clear();
		for( int i = 0; i < numExtensions; i++ )
		{
			extensions_string.Append( ( const char* )glGetStringi( GL_EXTENSIONS, i ) );
			// the now deprecated glGetString method usaed to create a single string with each extension separated by a space
			if( i < numExtensions - 1 )
			{
				extensions_string.Append( ' ' );
			}
		}
		glConfig.extensions_string = extensions_string.c_str();
	}


	float glVersion = atof( glConfig.version_string );
	float glslVersion = atof( glConfig.shading_language_string );
	idLib::Printf( "OpenGL Version   : %3.1f\n", glVersion );
	idLib::Printf( "OpenGL Vendor    : %s\n", glConfig.vendor_string );
	idLib::Printf( "OpenGL Renderer  : %s\n", glConfig.renderer_string );
	idLib::Printf( "OpenGL GLSL      : %3.1f\n", glslVersion );
	idLib::Printf( "OpenGL Extensions: %s\n", glConfig.extensions_string );

	// OpenGL driver constants
	GLint temp;
	glGetIntegerv( GL_MAX_TEXTURE_SIZE, &temp );
	glConfig.maxTextureSize = temp;

	// stubbed or broken drivers may have reported 0...
	if( glConfig.maxTextureSize <= 0 )
	{
		glConfig.maxTextureSize = 256;
	}

	r_initialized = true;

	// recheck all the extensions (FIXME: this might be dangerous)
	R_CheckPortableExtensions();

	r_initialized = true;

	// allocate the vertex array range or vertex objects
	vertexCache.Init();

	// allocate the frame data, which may be more if smp is enabled
	allocManager.InitFrameData();

	// Reset our gamma
	R_SetColorMappings();

	// RB begin
#if defined(_WIN32)
	static bool glCheck = false;
	if( !glCheck && win32.osversion.dwMajorVersion == 6 )
	{
		glCheck = true;
		if( !idStr::Icmp( glConfig.vendor_string, "Microsoft" ) && idStr::FindText( glConfig.renderer_string, "OpenGL-D3D" ) != -1 )
		{
			if( cvarSystem->GetCVarBool( "r_fullscreen" ) )
			{
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "vid_restart partial windowed\n" );
				Sys_GrabMouseCursor( false );
			}
			int ret = MessageBox( NULL, "Please install OpenGL drivers from your graphics hardware vendor to run " GAME_NAME ".\nYour OpenGL functionality is limited.",
								  "Insufficient OpenGL capabilities", MB_OKCANCEL | MB_ICONWARNING | MB_TASKMODAL );
			if( ret == IDCANCEL )
			{
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
				cmdSystem->ExecuteCommandBuffer();
			}
			if( cvarSystem->GetCVarBool( "r_fullscreen" ) )
			{
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "vid_restart\n" );
			}
		}
	}
#endif
	// RB end
}

/*
==================
GL_CheckErrors
==================
*/
// RB: added filename, line parms
bool GL_CheckErrors_( const char* filename, int line )
{
	int		err;
	char	s[64];
	int		i;

	if( r_ignoreGLErrors.GetBool() )
	{
		return false;
	}

	// check for up to 10 errors pending
	bool error = false;
	for( i = 0 ; i < 10 ; i++ )
	{
		err = glGetError();
		if( err == GL_NO_ERROR )
		{
			break;
		}

		error = true;
		switch( err )
		{
			case GL_INVALID_ENUM:
				strcpy( s, "GL_INVALID_ENUM" );
				break;
			case GL_INVALID_VALUE:
				strcpy( s, "GL_INVALID_VALUE" );
				break;
			case GL_INVALID_OPERATION:
				strcpy( s, "GL_INVALID_OPERATION" );
				break;
#if !defined(USE_GLES2) && !defined(USE_GLES3)
			case GL_STACK_OVERFLOW:
				strcpy( s, "GL_STACK_OVERFLOW" );
				break;
			case GL_STACK_UNDERFLOW:
				strcpy( s, "GL_STACK_UNDERFLOW" );
				break;
#endif
			case GL_OUT_OF_MEMORY:
				strcpy( s, "GL_OUT_OF_MEMORY" );
				break;
			default:
				idStr::snPrintf( s, sizeof( s ), "%i", err );
				break;
		}

		common->Printf( "caught OpenGL error: %s in file %s line %i\n", s, filename, line );
	}

	return error;
}
// RB end



/*
===============
R_SetColorMappings
===============
*/
void R_SetColorMappings()
{
	float b = r_brightness.GetFloat();
	float invg = 1.0f / r_gamma.GetFloat();

	float j = 0.0f;
	for( int i = 0; i < 256; i++, j += b )
	{
		int inf = idMath::Ftoi( 0xffff * pow( j / 255.0f, invg ) + 0.5f );
		tr.gammaTable[i] = idMath::ClampInt( 0, 0xFFFF, inf );
	}

	GLimp_SetGamma( tr.gammaTable, tr.gammaTable, tr.gammaTable );
}

/*
================
GfxInfo_f
================
*/
void GfxInfo_f( const idCmdArgs& args )
{
	common->Printf( "CPU: %s\n", sys->GetProcessorString() );

	const char* fsstrings[] =
	{
		"windowed",
		"fullscreen"
	};

	common->Printf( "\nGL_VENDOR: %s\n", glConfig.vendor_string );
	common->Printf( "GL_RENDERER: %s\n", glConfig.renderer_string );
	common->Printf( "GL_VERSION: %s\n", glConfig.version_string );
	common->Printf( "GL_EXTENSIONS: %s\n", glConfig.extensions_string );
	if( glConfig.wgl_extensions_string )
	{
		common->Printf( "WGL_EXTENSIONS: %s\n", glConfig.wgl_extensions_string );
	}
	common->Printf( "GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize );
	common->Printf( "GL_MAX_TEXTURE_COORDS_ARB: %d\n", glConfig.maxTextureCoords );
	common->Printf( "GL_MAX_TEXTURE_IMAGE_UNITS_ARB: %d\n", glConfig.maxTextureImageUnits );

	// print all the display adapters, monitors, and video modes
	//void DumpAllDisplayDevices();
	//DumpAllDisplayDevices();

	common->Printf( "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );
	common->Printf( "MODE: %d, %d x %d %s hz:", r_vidMode.GetInteger(), renderSystem->GetWidth(), renderSystem->GetHeight(), fsstrings[r_fullscreen.GetBool()] );
	if( glConfig.displayFrequency )
	{
		common->Printf( "%d\n", glConfig.displayFrequency );
	}
	else
	{
		common->Printf( "N/A\n" );
	}

	common->Printf( "-------\n" );

	// RB begin
#if defined(_WIN32) && !defined(USE_GLES2)
	// WGL_EXT_swap_interval
	typedef BOOL ( WINAPI * PFNWGLSWAPINTERVALEXTPROC )( int interval );
	extern	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

	if( r_swapInterval.GetInteger() && wglSwapIntervalEXT != NULL )
	{
		common->Printf( "Forcing swapInterval %i\n", r_swapInterval.GetInteger() );
	}
	else
	{
		common->Printf( "swapInterval not forced\n" );
	}
#endif
	// RB end

	if( glConfig.stereoPixelFormatAvailable && glConfig.isStereoPixelFormat )
	{
		idLib::Printf( "OpenGl quad buffer stereo pixel format active\n" );
	}
	else if( glConfig.stereoPixelFormatAvailable )
	{
		idLib::Printf( "OpenGl quad buffer stereo pixel available but not selected\n" );
	}
	else
	{
		idLib::Printf( "OpenGl quad buffer stereo pixel format not available\n" );
	}

	idLib::Printf( "Stereo mode: " );
	switch( renderSystem->GetStereo3DMode() )
	{
		case STEREO3D_OFF:
			idLib::Printf( "STEREO3D_OFF\n" );
			break;
		case STEREO3D_SIDE_BY_SIDE_COMPRESSED:
			idLib::Printf( "STEREO3D_SIDE_BY_SIDE_COMPRESSED\n" );
			break;
		case STEREO3D_TOP_AND_BOTTOM_COMPRESSED:
			idLib::Printf( "STEREO3D_TOP_AND_BOTTOM_COMPRESSED\n" );
			break;
		case STEREO3D_SIDE_BY_SIDE:
			idLib::Printf( "STEREO3D_SIDE_BY_SIDE\n" );
			break;
		case STEREO3D_HDMI_720:
			idLib::Printf( "STEREO3D_HDMI_720\n" );
			break;
		case STEREO3D_INTERLACED:
			idLib::Printf( "STEREO3D_INTERLACED\n" );
			break;
		case STEREO3D_QUAD_BUFFER:
			idLib::Printf( "STEREO3D_QUAD_BUFFER\n" );
			break;
		default:
			idLib::Printf( "Unknown (%i)\n", renderSystem->GetStereo3DMode() );
			break;
	}

	idLib::Printf( "%i multisamples\n", glConfig.multisamples );

	common->Printf( "%5.1f cm screen width (%4.1f\" diagonal)\n",
					glConfig.physicalScreenWidthInCentimeters, glConfig.physicalScreenWidthInCentimeters / 2.54f
					* sqrt( ( float )( 16 * 16 + 9 * 9 ) ) / 16.0f );
	extern idCVar r_forceScreenWidthCentimeters;
	if( r_forceScreenWidthCentimeters.GetFloat() )
	{
		common->Printf( "screen size manually forced to %5.1f cm width (%4.1f\" diagonal)\n",
						renderSystem->GetPhysicalScreenWidthInCentimeters(), renderSystem->GetPhysicalScreenWidthInCentimeters() / 2.54f
						* sqrt( ( float )( 16 * 16 + 9 * 9 ) ) / 16.0f );
	}

	if( glConfig.gpuSkinningAvailable )
	{
		common->Printf( S_COLOR_GREEN "GPU skeletal animation available\n" );
	}
	else
	{
		common->Printf( S_COLOR_RED "GPU skeletal animation not available (slower CPU path active)\n" );
	}
}

/*
=================
R_VidRestart_f
=================
*/
void R_VidRestart_f( const idCmdArgs& args )
{
	// if OpenGL isn't started, do nothing
	if( !R_IsInitialized() )
	{
		return;
	}

	// set the mode without re-initializing the context
	R_SetNewMode( false );

#if 0
	bool full = true;
	bool forceWindow = false;
	for( int i = 1 ; i < args.Argc() ; i++ )
	{
		if( idStr::Icmp( args.Argv( i ), "partial" ) == 0 )
		{
			full = false;
			continue;
		}
		if( idStr::Icmp( args.Argv( i ), "windowed" ) == 0 )
		{
			forceWindow = true;
			continue;
		}
	}

	// this could take a while, so give them the cursor back ASAP
	Sys_GrabMouseCursor( false );

	// dump ambient caches
	renderModelManager->FreeModelVertexCaches();

	// free any current world interaction surfaces and vertex caches
	R_FreeDerivedData();

	// make sure the defered frees are actually freed
	R_ToggleSmpFrame();
	R_ToggleSmpFrame();

	// free the vertex caches so they will be regenerated again
	vertexCache.PurgeAll();

	// sound and input are tied to the window we are about to destroy

	if( full )
	{
		// free all of our texture numbers
		sys->ShutdownInput();
		renderImageManager->PurgeAllImages();
		// free the context and close the window
		GLimp_Shutdown();
		r_initialized = false;

		// create the new context and vertex cache
		bool latch = cvarSystem->GetCVarBool( "r_fullscreen" );
		if( forceWindow )
		{
			cvarSystem->SetCVarBool( "r_fullscreen", false );
		}
		R_InitOpenGL();
		cvarSystem->SetCVarBool( "r_fullscreen", latch );

		// regenerate all images
		renderImageManager->ReloadImages( true );
	}
	else
	{
		glimpParms_t parms;
		parms.width = glConfig.nativeScreenWidth;
		parms.height = glConfig.nativeScreenHeight;
		parms.fullScreen = ( forceWindow ) ? false : r_fullscreen.GetInteger();
		parms.displayHz = r_displayRefresh.GetInteger();
		parms.multiSamples = r_multiSamples.GetInteger();
		parms.stereo = false;
		GLimp_SetScreenParms( parms );
	}



	// make sure the regeneration doesn't use anything no longer valid
	tr.viewCount++;
	tr.viewDef = NULL;

	// check for problems
	int err = glGetError();
	if( err != GL_NO_ERROR )
	{
		common->Printf( "glGetError() = 0x%x\n", err );
	}
#endif

}

/*
=================
R_InitMaterials
=================
*/
void R_InitMaterials()
{
	tr.defaultMaterial = declManager->FindMaterial( "_default", false );
	if( !tr.defaultMaterial )
	{
		common->FatalError( "_default material not found" );
	}
	tr.defaultPointLight = declManager->FindMaterial( "lights/defaultPointLight" );
	tr.defaultProjectedLight = declManager->FindMaterial( "lights/defaultProjectedLight" );
	tr.whiteMaterial = declManager->FindMaterial( "_white" );
	tr.charSetMaterial = declManager->FindMaterial( "textures/bigchars" );
}

/*
=================
R_InitCvars
=================
*/
void R_InitCvars()
{
	// update latched cvars here
}

/*
===============
idRenderSystemLocal::Clear
===============
*/
void idRenderSystemLocal::Clear()
{
	registered = false;
	frameCount = 0;
	viewCount = 0;
	frameShaderTime = 0.0f;
	ambientLightVector.Zero();
	worlds.Clear();
	primaryWorld = NULL;
	memset( &primaryRenderView, 0, sizeof( primaryRenderView ) );
	primaryView = NULL;
	defaultMaterial = NULL;
	testImage = NULL;
	ambientCubeImage = NULL;
	viewDef = NULL;
	memset( &pc, 0, sizeof( pc ) );
	memset( &identitySpace, 0, sizeof( identitySpace ) );
	memset( renderCrops, 0, sizeof( renderCrops ) );
	currentRenderCrop = 0;
	currentColorNativeBytesOrder = 0xFFFFFFFF;
	currentGLState = 0;
	guiRecursionLevel = 0;
	guiModel = NULL;
	memset( gammaTable, 0, sizeof( gammaTable ) );
	takingScreenshot = false;

	if( unitSquareTriangles != NULL ) {
		idMem::Free( unitSquareTriangles );
		unitSquareTriangles = NULL;
	}

	if( zeroOneCubeTriangles != NULL ) {
		idMem::Free( zeroOneCubeTriangles );
		zeroOneCubeTriangles = NULL;
	}

	if( testImageTriangles != NULL ) {
		idMem::Free( testImageTriangles );
		testImageTriangles = NULL;
	}

	frontEndJobList = NULL;
}

extern void RB_InitBuffers();
extern void RB_ShutdownBuffers();
extern void R_InitCommands();

/*
===============
idRenderSystemLocal::Init
===============
*/
void idRenderSystemLocal::Init()
{
	common->Printf( "------- Initializing renderSystem --------\n" );

	// clear all our internal state
	viewCount = 1;		// so cleared structures never match viewCount
	// we used to memset tr, but now that it is a class, we can't, so
	// there may be other state we need to reset

	ambientLightVector[0] = 0.5f;
	ambientLightVector[1] = 0.5f - 0.385f;
	ambientLightVector[2] = 0.8925f;
	ambientLightVector[3] = 1.0f;

	backEnd.Clear();

	R_InitCvars();

	R_InitCommands();

	guiModel = new( TAG_RENDER ) idRenderModelGui;
	guiModel->Clear();
	tr_guiModel = guiModel;	// for DeviceContext fast path

	UpdateStereo3DMode();

	RB_InitBuffers();

	renderImageManager->Init();
	renderDestManager.Init();
	renderProgManager.Init();
	renderCinematicManager->Init();

	// build brightness translation tables
	R_SetColorMappings();

	R_InitMaterials();

	renderModelManager->Init();

	// set the identity space
	identitySpace.modelMatrix.Identity();

	// make sure the tr.unitSquareTriangles data is current in the vertex / index cache
	if( unitSquareTriangles == NULL )
	{
		unitSquareTriangles = idTriangles::MakeUnitQuad();
	}
	// make sure the tr.zeroOneCubeTriangles data is current in the vertex / index cache
	if( zeroOneCubeTriangles == NULL )
	{
		zeroOneCubeTriangles = idTriangles::MakeZeroOneCube();
	}
	// make sure the tr.testImageTriangles data is current in the vertex / index cache
	if( testImageTriangles == NULL )
	{
		testImageTriangles = idTriangles::MakeZeroOneQuad();
	}

	frontEndJobList = parallelJobManager->AllocJobList( JOBLIST_RENDERER_FRONTEND, JOBLIST_PRIORITY_MEDIUM, 2048, 0, NULL );

	// make sure the command buffers are ready to accept the first screen update
	SwapCommandBuffers( NULL, NULL, NULL, NULL );

	common->Printf( "renderSystem initialized.\n" );
	common->Printf( "--------------------------------------\n" );
}

/*
===============
idRenderSystemLocal::Shutdown
===============
*/
void idRenderSystemLocal::Shutdown()
{
	common->Printf( "idRenderSystem::Shutdown()\n" );

	fonts.DeleteContents();

	if( R_IsInitialized() )
	{
		renderImageManager->PurgeAllImages();
	}

	renderModelManager->Shutdown();

	renderCinematicManager->Shutdown();

	RB_ShutdownBuffers();

	renderImageManager->Shutdown();
	renderDestManager.Shutdown();

	renderProgManager.Shutdown();

	// free frame memory
	allocManager.ShutdownFrameData();

	UnbindBufferObjects();

	// free the vertex cache, which should have nothing allocated now
	vertexCache.Shutdown();

	RB_ShutdownDebugTools();

	delete guiModel;

	parallelJobManager->FreeJobList( frontEndJobList );

	Clear();

	ShutdownRenderDevice();
}

/*
========================
idRenderSystemLocal::ResetGuiModels
========================
*/
void idRenderSystemLocal::ResetGuiModels()
{
	delete guiModel;
	guiModel = new( TAG_RENDER ) idRenderModelGui;
	guiModel->Clear();
	guiModel->BeginFrame();
	tr_guiModel = guiModel;	// for DeviceContext fast path
}

/*
========================
idRenderSystemLocal::BeginLevelLoad
========================
*/
void idRenderSystemLocal::BeginLevelLoad()
{
	renderImageManager->BeginLevelLoad();
	renderModelManager->BeginLevelLoad();

	// Re-Initialize the Default Materials if needed.
	R_InitMaterials();
}

/*
========================
idRenderSystemLocal::LoadLevelImages
========================
*/
void idRenderSystemLocal::LoadLevelImages()
{
	renderImageManager->LoadLevelImages( false );
}

/*
========================
idRenderSystemLocal::Preload
========================
*/
void idRenderSystemLocal::Preload( const idPreloadManifest& manifest, const char* mapName )
{
	renderImageManager->Preload( manifest, true );
	uiManager->Preload( mapName );
	renderModelManager->Preload( manifest );
}

/*
========================
idRenderSystemLocal::EndLevelLoad
========================
*/
void idRenderSystemLocal::EndLevelLoad()
{
	renderModelManager->EndLevelLoad();
	renderImageManager->EndLevelLoad();
}

/*
========================
idRenderSystemLocal::BeginAutomaticBackgroundSwaps
========================
*/
void idRenderSystemLocal::BeginAutomaticBackgroundSwaps( autoRenderIconType_t icon )
{
}

/*
========================
idRenderSystemLocal::EndAutomaticBackgroundSwaps
========================
*/
void idRenderSystemLocal::EndAutomaticBackgroundSwaps()
{
}

/*
========================
idRenderSystemLocal::AreAutomaticBackgroundSwapsRunning
========================
*/
bool idRenderSystemLocal::AreAutomaticBackgroundSwapsRunning( autoRenderIconType_t* icon ) const
{
	return false;
}

/*
============
idRenderSystemLocal::RegisterFont
============
*/
idFont* idRenderSystemLocal::RegisterFont( const char* fontName )
{
	idStrStatic< MAX_OSPATH > baseFontName = fontName;
	baseFontName.Replace( "fonts/", "" );
	for( int i = 0; i < fonts.Num(); i++ )
	{
		if( idStr::Icmp( fonts[i]->GetName(), baseFontName ) == 0 )
		{
			fonts[i]->Touch();
			return fonts[i];
		}
	}
	idFont* newFont = new( TAG_FONT ) idFont( baseFontName );
	fonts.Append( newFont );
	return newFont;
}

/*
========================
idRenderSystemLocal::ResetFonts
========================
*/
void idRenderSystemLocal::ResetFonts()
{
	fonts.DeleteContents( true );
}
/*
========================
idRenderSystemLocal::InitRenderDevice
========================
*/
void idRenderSystemLocal::InitRenderDevice()
{
	// if OpenGL isn't started, start it now
	if( !R_IsInitialized() )
	{
		R_InitOpenGL();

		// Reloading images here causes the rendertargets to get deleted. Figure out how to handle this properly on 360
		renderImageManager->ReloadImages( true );

		int err = glGetError();
		if( err != GL_NO_ERROR )
		{
			common->Printf( "glGetError() = 0x%x\n", err );
		}
	}
}

/*
========================
idRenderSystemLocal::ShutdownRenderDevice
========================
*/
void idRenderSystemLocal::ShutdownRenderDevice()
{
	// free the context and close the window
	allocManager.ShutdownFrameData();
	GLimp_Shutdown();
	r_initialized = false;
}

/*
========================
idRenderSystemLocal::IsRenderDeviceRunning
========================
*/
bool idRenderSystemLocal::IsRenderDeviceRunning() const
{
	return R_IsInitialized();
}

/*
========================
idRenderSystemLocal::IsFullScreen
========================
*/
bool idRenderSystemLocal::IsFullScreen() const
{
	return glConfig.isFullscreen != 0;
}

/*
========================
idRenderSystemLocal::GetWidth
========================
*/
int idRenderSystemLocal::GetWidth() const
{
	if( glConfig.stereo3Dmode == STEREO3D_SIDE_BY_SIDE || glConfig.stereo3Dmode == STEREO3D_SIDE_BY_SIDE_COMPRESSED )
	{
		return glConfig.nativeScreenWidth >> 1;
	}
	return glConfig.nativeScreenWidth;
}

/*
========================
idRenderSystemLocal::GetHeight
========================
*/
int idRenderSystemLocal::GetHeight() const
{
	if( glConfig.stereo3Dmode == STEREO3D_HDMI_720 )
	{
		return 720;
	}
	extern idCVar stereoRender_warp;
	if( glConfig.stereo3Dmode == STEREO3D_SIDE_BY_SIDE && stereoRender_warp.GetBool() )
	{
		// for the Rift, render a square aspect view that will be symetric for the optics
		return glConfig.nativeScreenWidth >> 1;
	}
	if( glConfig.stereo3Dmode == STEREO3D_INTERLACED || glConfig.stereo3Dmode == STEREO3D_TOP_AND_BOTTOM_COMPRESSED )
	{
		return glConfig.nativeScreenHeight >> 1;
	}
	return glConfig.nativeScreenHeight;
}

/*
========================
idRenderSystemLocal::GetVirtualWidth
========================
*/
int idRenderSystemLocal::GetVirtualWidth() const
{
	if( r_useVirtualScreenResolution.GetBool() )
	{
		return SCREEN_WIDTH;
	}
	return glConfig.nativeScreenWidth;
}

/*
========================
idRenderSystemLocal::GetVirtualHeight
========================
*/
int idRenderSystemLocal::GetVirtualHeight() const
{
	if( r_useVirtualScreenResolution.GetBool() )
	{
		return SCREEN_HEIGHT;
	}
	return glConfig.nativeScreenHeight;
}

/*
========================
idRenderSystemLocal::GetStereo3DMode
========================
*/
stereo3DMode_t idRenderSystemLocal::GetStereo3DMode() const
{
	return glConfig.stereo3Dmode;
}

/*
========================
idRenderSystemLocal::IsStereoScopicRenderingSupported
========================
*/
bool idRenderSystemLocal::IsStereoScopicRenderingSupported() const
{
	return true;
}

/*
========================
idRenderSystemLocal::HasQuadBufferSupport
========================
*/
bool idRenderSystemLocal::HasQuadBufferSupport() const
{
	return glConfig.stereoPixelFormatAvailable;
}

/*
========================
idRenderSystemLocal::UpdateStereo3DMode
========================
*/
void idRenderSystemLocal::UpdateStereo3DMode()
{
	if( glConfig.nativeScreenWidth == 1280 && glConfig.nativeScreenHeight == 1470 )
	{
		glConfig.stereo3Dmode = STEREO3D_HDMI_720;
	}
	else {
		glConfig.stereo3Dmode = GetStereoScopicRenderingMode();
	}
}

/*
========================
idRenderSystemLocal::GetStereoScopicRenderingMode
========================
*/
stereo3DMode_t idRenderSystemLocal::GetStereoScopicRenderingMode() const
{
	return ( !IsStereoScopicRenderingSupported() ) ? STEREO3D_OFF : ( stereo3DMode_t )stereoRender_enable.GetInteger();
}

/*
========================
idRenderSystemLocal::IsStereoScopicRenderingSupported
========================
*/
void idRenderSystemLocal::EnableStereoScopicRendering( const stereo3DMode_t mode ) const
{
	stereoRender_enable.SetInteger( mode );
}

/*
========================
idRenderSystemLocal::GetPixelAspect
========================
*/
float idRenderSystemLocal::GetPixelAspect() const
{
	switch( glConfig.stereo3Dmode )
	{
		case STEREO3D_SIDE_BY_SIDE_COMPRESSED:
			return glConfig.pixelAspect * 2.0f;

		case STEREO3D_TOP_AND_BOTTOM_COMPRESSED:
		case STEREO3D_INTERLACED:
			return glConfig.pixelAspect * 0.5f;

		default:
			return glConfig.pixelAspect;
	}
}

/*
========================
idRenderSystemLocal::GetPhysicalScreenWidthInCentimeters

	This is used to calculate stereoscopic screen offset for a given interocular distance.
========================
*/
idCVar	r_forceScreenWidthCentimeters( "r_forceScreenWidthCentimeters", "0", CVAR_RENDERER | CVAR_ARCHIVE, "Override screen width returned by hardware" );
float idRenderSystemLocal::GetPhysicalScreenWidthInCentimeters() const
{
	if( r_forceScreenWidthCentimeters.GetFloat() > 0 )
	{
		return r_forceScreenWidthCentimeters.GetFloat();
	}
	return glConfig.physicalScreenWidthInCentimeters;
}

/*
========================
 idRenderSystemLocal::GetModeListForDisplay

	the number of displays can be found by itterating this until it returns false
	displayNum is the 0 based value passed to EnumDisplayDevices(), you must add
	1 to this to get an r_fullScreen value.
========================
*/
bool idRenderSystemLocal::GetModeListForDisplay( const int displayNum, vidModes_t & modeList ) const
{
	extern bool R_GetModeListForDisplay( const int, vidModes_t & );
	return R_GetModeListForDisplay( displayNum, modeList );
}
