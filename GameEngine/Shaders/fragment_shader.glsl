#version 400

in vec2 textureCoord; 
in vec3 norm;
in vec3 fragPos;
in vec2 worldPosXZ;

out vec4 fragColor;

uniform sampler2D texture1;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 objectTint;

uniform int numHazards;
uniform vec3 hazardPositions[10];
uniform vec3 hazardSizes[10];

void main()
{
    vec4 texColor = texture(texture1, textureCoord);
    
    vec3 finalColor = texColor.rgb * objectTint;
    
    bool inHazard = false;
    for (int i = 0; i < numHazards; i++) {
        vec2 hazardCenter = hazardPositions[i].xz;
        float hazardRadius = hazardSizes[i].x / 2.0;
        
        float dist = distance(worldPosXZ, hazardCenter);
        
        if (dist < hazardRadius) {
            inHazard = true;
            
            float normalizedDist = dist / hazardRadius;
            float darkness = 1.0 - normalizedDist * 0.5;
            
            finalColor *= darkness * vec3(0.25, 0.15, 0.1);
            break;
        }
    }
    
    fragColor = vec4(finalColor, texColor.a);
}
