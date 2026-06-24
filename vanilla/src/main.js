const vertexShaderSource = `#version 300 es
in vec2 a_position; //input to a vertex shader. will recieve data from a bugger
out vec2 v_texCoord; //Pass this to fragment shader
void main(){
  gl_Position = vec4(a_position, 0.0, 1.0);
  //map clip space (-1 to 1) to texture space (0 to 1)
  vec2 texCoord = a_position * 0.5 + 0.5;
  v_texCoord = vec2(1.0 - texCoord.x, 1.0 - texCoord.y); //flipped so that its similar to what you see irl.
}
`;
const fragmentShaderSource = `#version 300 es
precision highp float; //fragement shaders don't have a default precision so we need to pick one. highp is a good default. means "high precision".
in vec2 v_texCoord; //received from vertex shader
out vec4 outColor;

uniform float u_time;
uniform sampler2D u_cameraTexture;
void main(){
  vec4 color = texture(u_cameraTexture, v_texCoord);
  float gray = dot(color.rgb, vec3(0.299,0.587, 0.114));
  outColor = vec4(vec3(gray), color.a);
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
//GPU UI overlay, the GPU timer tells you how long it takes for you GPU to run the shaders and get an ouput. 
let currentQuery = null;
const statsDiv = document.getElementById('gpu-stats');
const extTimer = gl.getExtension('EXT_disjoint_timer_query_webgl2');
if (!extTimer) {
  console.log('GPU Timer extension not supported on this browser/hardware.');
  statsDiv.innerHTML = "GPU timer not supported. sorry!";
  statsFiv.style.color = '#ff3333';
}
let frameCount = 0;


function render(time){

  //convert millisecond to seconds for the uniform
  const timeInSeconds = time * 0.001;

  gl.viewport(0,0, canvas.width, canvas.height);
  gl.clear(gl.COLOR_BUFFER_BIT);

  //update webcamTexture
  if(video.readyState >= video.HAVE_CURRENT_DATA){
    gl.bindTexture(gl.TEXTURE_2D, texture);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, video);
  }
  
  //GPU STUFF
  // Process completed GPU timing queries
  if (extTimer && currentQuery) {
    const available = gl.getQueryParameter(currentQuery, gl.QUERY_RESULT_AVAILABLE);
    const disjoint = gl.getParameter(extTimer.GPU_DISJOINT_EXT);

    if (available && !disjoint) {
      const timeElapsedNanos = gl.getQueryParameter(currentQuery, gl.QUERY_RESULT);
      const timeElapsedMillis = timeElapsedNanos / 1000000;
      
      // Update screen text every 10 frames to avoid unreadable flickering
      frameCount++;
      if (frameCount % 10 === 0) {
        statsDiv.innerHTML = `GPU Time: ${timeElapsedMillis.toFixed(3)} ms`;
      }
    }

    if (available || disjoint) {
      gl.deleteQuery(currentQuery);
      currentQuery = null;
    }
  }

  // Start timing right before drawing
  if (extTimer && !currentQuery) {
    currentQuery = gl.createQuery();
    gl.beginQuery(extTimer.TIME_ELAPSED_EXT, currentQuery);
  }

  gl.useProgram(program);
  gl.uniform1f(timeLocation, time); // send time to the GPU
  gl.bindVertexArray(vao);
  gl.drawArrays(gl.TRIANGLES,0,6);

  //GPU STUFF
  if (extTimer && currentQuery) {
    gl.endQuery(extTimer.TIME_ELAPSED_EXT);
  }

  requestAnimationFrame(render);
}

requestAnimationFrame(render);

//to print out what GPU the computer is using?
const ext = gl.getExtension('WEBGL_debug_renderer_info');
if(ext){
  console.log(gl.getParameter(ext.UNMASKED_VENDOR_WEBGL));
  console.log(gl.getParameter(ext.UNMASKED_RENDERER_WEBGL));
}


//camera
const video = document.createElement('video');
video.autoplay = true;
video.playsInline = true;

navigator.mediaDevices.getUserMedia({video : true}).then(stream => {
  video.srcObject = stream;
}).catch(err => console.error("Camera error: ", err));

//web gl2 texture
const texture = gl.createTexture();
gl.bindTexture(gl.TEXTURE_2D, texture);
gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAX_FILTER, gl.LINEAR);
//s and t mean x and y, clamp makes it stop at the last pixel 
//gl_repeate makes it repeat when you go past (0,0) to (1,1)
//gl_clamp_to_edge_ h
gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);

