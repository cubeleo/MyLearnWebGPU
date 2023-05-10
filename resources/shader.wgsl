struct VertexInput
{
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) color: vec3<f32>,
};

struct VertexOutput
{
    @builtin(position) position: vec4<f32>,
    @location(0) normal: vec3<f32>,
    @location(1) color: vec3<f32>,
};

struct MyUniforms
{
    clipFromView: mat4x4<f32>,
    viewFromWorld: mat4x4<f32>,
    worldFromObject: mat4x4<f32>,
    color: vec4<f32>,
    time: f32,
};

@group(0) @binding(0) var<uniform> myUniforms: MyUniforms;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput
{
    var out: VertexOutput;

    let viewFromObject = myUniforms.viewFromWorld * myUniforms.worldFromObject;

    let viewPosition = viewFromObject * vec4<f32>(in.position, 1.);

    out.position = myUniforms.clipFromView * viewPosition;
    out.normal = (myUniforms.worldFromObject * vec4<f32>(in.normal, 0.)).xyz;
    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32>
{
    let normal = normalize(in.normal);
    let lightColor1 = vec3<f32>(1.0, 0.9, 0.6);
    let lightColor2 = vec3<f32>(0.6, 0.9, 1.0);
    let lightDirection1 = vec3<f32>(0.5, -0.9, 0.1);
    let lightDirection2 = vec3<f32>(0.2, 0.4, 0.3);
    let shading1 = max(0.0, dot(lightDirection1, normal));
    let shading2 = max(0.0, dot(lightDirection2, normal));
    let shading = shading1 * lightColor1 + shading2 * lightColor2;
    let color = in.color * shading;
    let linear_color = pow(color, vec3<f32>(2.2));
    return vec4<f32>(linear_color, 1.0);
}
