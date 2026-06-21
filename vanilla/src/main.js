const vertexShaderSource = `#version 300 es
in vec2 a_position; //input to a vertex shader. will recieve data from a bugger
void main(){
  gl_Position = vec4(a_position, 0.0, 1.0);
}
`;
const fragmentShaderSource = `#version 300 es
precision highp float; //fragement shaders don't have a default precision so we need to pick one. highp is a good default. means "high precision".
out vec4 outColor;
uniform float u_time;
void main(){
  float pulse =  sin(u_time * 0.001);
  outColor = vec4(pulse,0,0,1); //red but from a shader this time!
}`

const canvas = document.getElementById('canvas');
const gl = canvas.getContext('webgl2');

if(!gl){
  alert('Web GL is not supported in this browser, sorry!');
  throw new Error('No WebGL2 context');
}
gl.clearColor(1,0,0,1) ; //red
gl.clear(gl.COLOR_BUFFER_BIT);

console.log('WebGL2 context created successfully:', gl);

function createShader(gl, type, source){
  var shader = gl.createShader(type);
  gl.shaderSource(shader, source);
  gl.compileShader(shader);
  var success = gl.getShaderParameter(shader, gl.COMPILE_STATUS);
  if(success){
    return shader;
  }
  console.log(gl.getShaderInfoLog(shader));
  gl.deleteShader(shader);
}

function createProgram(gl, vertexShader, fragementShader){
  var program = gl.createProgram();
  gl.attachShader(program, vertexShader);
  gl.attachShader(program, fragementShader);
  gl.linkProgram(program);
  var success = gl.getProgramParameter(program, gl.LINK_STATUS);
  if(success){
    return program;
  }
  console.log(gl.getProgramInfoLog(program));
  gl.deleteProgram(program);
}

var vertexShader = createShader(gl, gl.VERTEX_SHADER, vertexShaderSource);
var fragmentShader = createShader(gl, gl.FRAGMENT_SHADER, fragmentShaderSource);
var program = createProgram(gl, vertexShader, fragmentShader);

console.log(vertexShader);
console.log(fragmentShader);
console.log(program);

// end of setting up the vertex and fragment shaders 

const positions = new Float32Array([
  -1,-1,
    1,-1,
    -1,1,
    -1,1,
    1,-1,
    1,1,
]); //two triangles that tile the entire area! clip space -1,1 on both axes 

var positionBuffer = gl.createBuffer();
gl.bindBuffer(gl.ARRAY_BUFFER, positionBuffer);
gl.bufferData(gl.ARRAY_BUFFER, positions, gl.STATIC_DRAW);

var positionLocation = gl.getAttribLocation(program, 'a_position')
const vao = gl.createVertexArray(); //vao = Vertex Array Object
gl.bindVertexArray(vao); 

gl.enableVertexAttribArray(positionLocation);
gl.bindBuffer(gl.ARRAY_BUFFER, positionBuffer);
gl.vertexAttribPointer(
  positionLocation,
  2, //each vertex is 2 components (x,y)
  gl.FLOAT, //data type
  false, // don't normalize
  0, //stride
  0 // offset. 
);

var timeLocation = gl.getUniformLocation(program, 'u_time');

function render(time){
  gl.viewport(0,0, canvas.width, canvas.height);
  gl.useProgram(program);
  gl.uniform1f(timeLocation, time);
  gl.bindVertexArray(vao);
  gl.drawArrays(gl.TRIANGLES,0,6);
  requestAnimationFrame(render);
}

requestAnimationFrame(render);

//to print out what GPU the computer is using?
const ext = gl.getExtension('WEBGL_debug_renderer_info');
if(ext){
  console.log(gl.getParameter(ext.UNMASKED_VENDOR_WEBGL));
  console.log(gl.getParameter(ext.UNMASKED_RENDERER_WEBGL));
}
