#version 120

varying vec2 vUV;
varying vec3 vN;
varying vec3 vPos;

uniform sampler2D colorMap;
uniform sampler2D normalMap;
uniform sampler2D specMap;
uniform sampler2D envMap;
uniform int hasColor;
uniform int hasNormal;
uniform int hasSpec;
uniform int hasEnv;
uniform vec3 lightDir;
uniform float envStrength;
uniform float brightness;
uniform int useStudio;

float studioStructure(vec3 R, bool glints) {
    float up = clamp(R.y * 0.5 + 0.5, 0.0, 1.0);
    float grad = mix(0.05, 1.0, pow(up, 1.5));
    float g = 0.0;
    if (glints) {
        vec3 l0 = normalize(vec3( 0.3,  0.8,  0.5));
        vec3 l1 = normalize(vec3(-0.6,  0.4,  0.6));
        vec3 l2 = normalize(vec3( 0.1, -0.3, -0.9));
        g = pow(max(dot(R, l0), 0.0), 200.0) * 3.0
          + pow(max(dot(R, l1), 0.0), 120.0) * 1.5
          + pow(max(dot(R, l2), 0.0), 300.0) * 2.0;
    }
    return grad + g;
}

mat3 cotangentFrame(vec3 N, vec3 p, vec2 uv) {
    vec3 dp1 = dFdx(p); vec3 dp2 = dFdy(p);
    vec2 duv1 = dFdx(uv); vec2 duv2 = dFdy(uv);
    vec3 dp2perp = cross(dp2, N);
    vec3 dp1perp = cross(N, dp1);
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
    float invmax = inversesqrt(max(dot(T, T), dot(B, B)));
    return mat3(T * invmax, B * invmax, N);
}

void main() {
    vec3 albedo = (hasColor == 1) ? texture2D(colorMap, vUV).rgb : vec3(0.8);
    albedo = pow(albedo, vec3(2.2));

    vec3 N = normalize(vN);
    if (hasNormal == 1) {
        vec4 nt = texture2D(normalMap, vUV);
        vec3 nm; nm.x = nt.a * 2.0 - 1.0; nm.y = nt.g * 2.0 - 1.0;
        nm.z = sqrt(max(0.0, 1.0 - nm.x * nm.x - nm.y * nm.y));
        mat3 TBN = cotangentFrame(N, vPos, vUV);
        N = normalize(TBN * nm);
    }

    vec3 V = normalize(-vPos);
    vec3 F0 = (hasSpec == 1) ? pow(texture2D(specMap, vUV).rgb, vec3(2.2)) : vec3(0.04);
    float gloss = clamp(dot(F0, vec3(0.3333)), 0.0, 1.0);

    vec3 L[3];
    L[0] = normalize(lightDir);
    L[1] = normalize(vec3(-0.7, 0.2, 0.6));
    L[2] = normalize(vec3(0.2, -0.4, -0.9));
    float Li[3];
    Li[0] = 1.00; Li[1] = 0.45; Li[2] = 0.75;

    float specPow = mix(16.0, 220.0, gloss);
    float diff = 0.0;
    vec3  spec = vec3(0.0);
    for (int i = 0; i < 3; i++) {
        diff += max(dot(N, L[i]), 0.0) * Li[i];
        vec3 H = normalize(L[i] + V);
        float nh = max(dot(N, H), 0.0);
        vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);
        spec += pow(nh, specPow) * F * Li[i];
    }

    float kd = 1.0 - gloss;
    vec3 color = albedo * kd * (0.30 + 0.70 * diff);
    color += spec * 3.0;

    vec3 R = reflect(-V, N);
    vec3 envColor = vec3(1.0);
    if (hasEnv == 1) {
        float m = 2.0 * sqrt(R.x * R.x + R.y * R.y + (R.z + 1.0) * (R.z + 1.0));
        vec2 euv = R.xy / m + 0.5;
        envColor = pow(texture2D(envMap, euv).rgb, vec3(2.2));
    }
    float structure = studioStructure(R, useStudio == 1);
    vec3 Fr = F0 + (1.0 - F0) * pow(1.0 - max(dot(N, V), 0.0), 5.0);
    color += envColor * structure * Fr * envStrength;

    color *= brightness;
    float lum = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float toned = lum / (1.0 + lum);
    color *= toned / max(lum, 0.0001);
    color = pow(color, vec3(1.0 / 2.2));

    gl_FragColor = vec4(color, 1.0);
}
