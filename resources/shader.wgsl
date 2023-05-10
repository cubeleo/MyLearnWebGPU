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
    clipFromView: mat4x4<f32>,
    viewFromWorld: mat4x4<f32>,
    color: vec4<f32>,
    time: f32,
};

@group(0) @binding(0) var<uniform> myUniforms: MyUniforms;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput
{
    var out: VertexOutput;

    // Rotate the model in the XY plane
    let angle1 = myUniforms.time;
    let c1 = cos(angle1);
    let s1 = sin(angle1);
    let R1 = transpose(mat4x4<f32>(
         c1,  s1, 0.0, 0.0,
        -s1,  c1, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    ));

    let pi = 3.14159;
    // Tilt the view point in the YZ plane
    // by three 8th of turn (1 turn = 2 pi)
    let angle2 = 3.0 * pi / 4.0;
    let c2 = cos(angle2);
    let s2 = sin(angle2);
    let R2 = transpose(mat4x4<f32>(
        1.0, 0.0, 0.0, 0.0,
        0.0,  c2,  s2, 0.0,
        0.0, -s2,  c2, 0.0,
        0.0, 0.0, 0.0, 1.0
    ));

    let S = transpose(mat4x4<f32>(
        0.3,  0.0, 0.0, 0.0,
        0.0,  0.3, 0.0, 0.0,
        0.0,  0.0, 0.3, 0.0,
        0.0,  0.0, 0.0, 1.0,
    ));

    // Translate the object
    let OrbitTranslation = transpose(mat4x4<f32>(
        1.0,  0.0, 0.0, 0.5,
        0.0,  1.0, 0.0, 0.0,
        0.0,  0.0, 1.0, 0.0,
        0.0,  0.0, 0.0, 1.0,
    ));

    let viewPosition = myUniforms.viewFromWorld * R1 * R2 * OrbitTranslation * S * vec4<f32>(in.position, 1.);

    out.position = myUniforms.clipFromView * viewPosition;
    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32>
{
    let linear_color = pow(in.color * myUniforms.color.rgb, vec3<f32>(2.2));
    return vec4<f32>(linear_color, 1.0);
}
