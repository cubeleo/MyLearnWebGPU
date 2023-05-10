struct VertexInput
{
    @location(0) position: vec3<f32>,
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
    let s = sin(myUniforms.time);
    let c = cos(myUniforms.time);
    let R = transpose(mat3x3<f32>(
        1.0, 0.0, 0.0,
        0.0,   c,   s,
        0.0,  -s,   c,
    ));

    let viewPosition = vec3<f32>(R * in.position + vec3<f32>(0., 0., 0.));

    out.position = vec4<f32>(viewPosition.xy, viewPosition.z * .5 + .5, 1.0);
    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32>
{
    let linear_color = pow(in.color * myUniforms.color.rgb, vec3<f32>(2.2));
    return vec4<f32>(linear_color, 1.0);
}
