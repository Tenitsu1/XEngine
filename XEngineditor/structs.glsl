struct Ray
{
    vec3 origin;
    vec3 direction;
    vec3 inverseDirection;
};

struct triangle
{
    vec4 PositionUvX0;
    vec4 PositionUvX1;
    vec4 PositionUvX2;
    
    vec4 NormalUvY0; 
    vec4 NormalUvY1; 
    vec4 NormalUvY2;
    
    vec4 Tangent0;
    vec4 Tangent1;  
    vec4 Tangent2;
    
    vec3 Centroid;
    float padding3; 
};

struct bvhNode
{
    vec3 AABBMin;
    float LeftChildOrFirst;
    vec3 AABBMax;
    float TriangleCount; 
};

struct indexData
{
    uint triangleDataStartInx;
    uint IndicesDataStartInx;
    uint BVHNodeDataStartInx;
    uint TriangleCount;
};