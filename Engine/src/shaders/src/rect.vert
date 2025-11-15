#version 460 core
#extension GL_EXT_buffer_reference : require
 
 struct Vertex
 {
 	vec4 position;
	vec4 color;
 };

 layout(location = 0) out vec4 outColor;
 layout(buffer_reference, std430) readonly buffer VertexBuffer
 {
	Vertex vertices[];
 } vertexBuffer;

 layout(push_constant) uniform ModelInfo
 {
	mat4 modelMatrix;
	VertexBuffer vBuffer;
 } modelInfo;

 void main()
 {
	Vertex v = modelInfo.vBuffer.vertices[gl_VertexIndex];
	outColor = v.color;
	gl_Position = modelInfo.modelMatrix * vec4(v.position);
 }