#version 400

in vec2 textureCoord; 
in vec3 norm;
in vec3 fragPos;
in vec2 worldPosXZ; // NEW: receive XZ position

out vec4 fragColor;

uniform sampler2D texture1;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 objectTint;

// NEW: Hazard zone data (we'll pass these as uniforms)
uniform int numHazards;
uniform vec3 hazardPositions[10]; // Max 10 hazard zones
uniform vec3 hazardSizes[10];

void main()
{
    // Sample texture
    vec4 texColor = texture(texture1, textureCoord);
    
    // Apply tint
    vec3 finalColor = texColor.rgb * objectTint;
    
    // NEW: Check if this fragment is inside any hazard zone
    bool inHazard = false;
    for (int i = 0; i < numHazards; i++) {
        vec2 hazardCenter = hazardPositions[i].xz;
        float hazardRadius = hazardSizes[i].x / 2.0;
        
        float dist = distance(worldPosXZ, hazardCenter);
        
        if (dist < hazardRadius) {
            inHazard = true;
            
            // Darken based on distance to center (darker in middle)
            float normalizedDist = dist / hazardRadius; // 0 at center, 1 at edge
            float darkness = 1.0 - normalizedDist * 0.5; // 0.5 at center, 1.0 at edge
            
            finalColor *= darkness * vec3(0.25, 0.15, 0.1); // Very dark brown
            break;
        }
    }
    
    fragColor = vec4(finalColor, texColor.a);
}
