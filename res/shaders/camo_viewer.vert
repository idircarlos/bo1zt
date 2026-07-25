#version 120

varying vec2 vUV;
varying vec3 vN;
varying vec3 vPos;

void main() {
    vUV = gl_MultiTexCoord0.xy;
    vN = normalize(gl_NormalMatrix * gl_Normal);
    vPos = (gl_ModelViewMatrix * gl_Vertex).xyz;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
