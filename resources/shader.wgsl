struct VertexInput
{
    @location(0) position: vec2<f32>,
    @location(1) color: vec3<f32>,
};

struct VertexOutput
{
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec3<f32>,
};

struct MyUniforms
{
    color: vec4<f32>,
    time: f32,
};

@group(0) @binding(0) var<uniform> myUniforms: MyUniforms;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput
{
    var out: VertexOutput;
    let offset = vec2<f32>(-0.6875, -0.463) + 0.3 * vec2<f32>(cos(myUniforms.time), sin(myUniforms.time));
    out.position = vec4<f32>(in.position + offset, 0.0, 1.0);
    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32>
{
    let linear_color = pow(in.color * myUniforms.color.rgb, vec3<f32>(2.2));
    return vec4<f32>(linear_color, 1.0);
}
