#version $GLSL_VERSION_STR
$GLSL_DEFAULT_PRECISION_FLOAT

#pragma vp_entryPoint "oe_fog_frag"
#pragma vp_location   "fragment_lighting"
#pragma vp_order      "0.7"

varying float oe_fog_fogFactor;

void oe_fog_frag(inout vec4 color)
{        
    color.rgb = mix( gl_Fog.color.rgb, color.rgb, oe_fog_fogFactor);
}