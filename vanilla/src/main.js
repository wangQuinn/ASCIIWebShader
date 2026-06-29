const CHARSETS = {
  default: " .,:;+-?][*#&%@$",        // original dense ramp
  blocks:  " ░▒▓█",                    // block elements
  minimal: " .·o0@",                   // very spare
  symbols: " .-~:;!=|+*#%@$",         // more punctuation variety
};


// Each entry: [brightnessThreshold (0–1), cssColor]
// Characters at or below the threshold get that colour.
const COLOR_PALETTE = [
  [0.15, "#2a1a4a"],   // very dark –
  [0.30, "#6a2faf"],   // shadows
  [0.50, "#c87fff"],   // mid-tones
  [0.70, "#ff9fe8"],   // brighte
  [0.85, "#ffcfa0"],   // highlights
  [1.00, "#ffffff"],   // brightest 
];

//states

let cfg = {
  columns:   100,
  contrast:  1.8,
  gamma:     1.2,
  charset:   "default",
  colorMode: true,
  mirror:    true,
  invert:    false,
};



const asciiCanvas = document.getElementById("ascii-canvas");
const asciiCtx    = asciiCanvas.getContext("2d");
const status      = document.getElementById("status");
const hiddenCanvas = document.getElementById("hidden-canvas");
const hiddenCtx    = hiddenCanvas.getContext("2d");


// Measure a single character so we can size the output canvas exactly.
// Change FONT_SIZE here to scale the whole output up or down.
const FONT_SIZE = 10; // px
const FONT_FACE = "Courier New, Courier, monospace";

asciiCtx.font = `${FONT_SIZE}px ${FONT_FACE}`;
const CHAR_W = asciiCtx.measureText("M").width;   // monospace: all chars same width
const CHAR_H = FONT_SIZE * 1;                   // line-height multiplier //square



function bindRange(id, key, valId, parse = parseFloat) {
  const el  = document.getElementById(id);
  const val = document.getElementById(valId);
  el.addEventListener("input", () => {
    cfg[key] = parse(el.value);
    val.textContent = el.value;
  });
}

bindRange("cols",     "columns",  "val-cols",     parseInt);
bindRange("contrast", "contrast", "val-contrast");
bindRange("gamma",    "gamma",    "val-gamma");

document.getElementById("charset").addEventListener("change", e => {
  cfg.charset = e.target.value;
});
document.getElementById("color-mode").addEventListener("change", e => {
  cfg.colorMode = e.target.checked;
});
document.getElementById("mirror").addEventListener("change", e => {
  cfg.mirror = e.target.checked;
});
document.getElementById("invert").addEventListener("change", e => {
  cfg.invert = e.target.checked;
});

// VIDEO 
const video = document.createElement("video");
video.autoplay = true;
video.playsInline = true;
video.muted = true;

navigator.mediaDevices.getUserMedia({ video: true })
  .then(stream => {
    video.srcObject = stream;
    video.play();
    status.textContent = "Camera active";
    requestAnimationFrame(render);
  })
  .catch(err => {
    status.textContent = "Camera error: " + err.message;
  });

//IMAGE PROCESSING 

/**
 * Apply contrast + optional invert to a raw ImageData pixel array.
 * Returns array of normalised brightness values (0–1) in row-major order.
 */
function extractBrightness(imageData, width, height, contrast, gamma, invert) {
  const data   = imageData.data;
  const result = new Float32Array(width * height);

  for (let i = 0; i < width * height; i++) {
    const px = i * 4;
    const r  = data[px];
    const g  = data[px + 1];
    const b  = data[px + 2];

    // Perceptual luminance
    let grey = (0.299 * r + 0.587 * g + 0.114 * b);

    // Contrast stretch around mid-grey
    grey = (grey - 128) * contrast + 128;
    grey = Math.max(0, Math.min(255, grey));

    // Normalise to 0-1
    let norm = grey / 255;

    // Gamma curve
    norm = Math.pow(norm, gamma);

    if (invert) norm = 1 - norm;

    result[i] = norm;
  }
  return result;
}

/**
 * Map a brightness value (0–1) to a CSS colour from COLOR_PALETTE.
 */
function brightnessToColor(b) {
  for (const [thresh, col] of COLOR_PALETTE) {
    if (b <= thresh) return col;
  }
  return COLOR_PALETTE[COLOR_PALETTE.length - 1][1];
}

// ─── RENDER ─

function render() {
  if (video.readyState < video.HAVE_CURRENT_DATA) {
    requestAnimationFrame(render);
    return;
  }

  const cols = cfg.columns;
  const rows = Math.round(cols / 2);

  // Size the hidden sampling canvas to the grid
  hiddenCanvas.width  = cols;
  hiddenCanvas.height = rows;

  // Draw webcam frame (with optional mirror) into the tiny sampling canvas
  if (cfg.mirror) {
    hiddenCtx.save();
    hiddenCtx.translate(cols, 0);
    hiddenCtx.scale(-1, 1);
  }
  hiddenCtx.drawImage(video, 0, 0, cols, rows);
  if (cfg.mirror) hiddenCtx.restore();

  // Extract brightness values
  const imageData  = hiddenCtx.getImageData(0, 0, cols, rows);
  const brightness = extractBrightness(
    imageData, cols, rows,
    cfg.contrast, cfg.gamma, cfg.invert
  );

  // Size the output canvas to fit all characters exactly
  asciiCanvas.width  = Math.ceil(cols * CHAR_W);
  asciiCanvas.height = Math.ceil(rows * CHAR_H);

  // Clear to background colour
  asciiCtx.fillStyle = "#0d0d0d";
  asciiCtx.fillRect(0, 0, asciiCanvas.width, asciiCanvas.height);

  asciiCtx.font         = `${FONT_SIZE}px ${FONT_FACE}`;
  asciiCtx.textBaseline = "top";

  const chars      = CHARSETS[cfg.charset] || CHARSETS.default;
  const lastI      = chars.length - 1;
  const monoColor  = "#e0c8ff";

  // Draw each character individually — no DOM, no GC, just GPU-bound fillText
  for (let y = 0; y < rows; y++) {
    for (let x = 0; x < cols; x++) {
      const b  = brightness[y * cols + x];
      const ch = chars[Math.floor(b * lastI)];

      asciiCtx.fillStyle = cfg.colorMode ? brightnessToColor(b) : monoColor;
      asciiCtx.fillText(ch, x * CHAR_W, y * CHAR_H);
    }
  }

  requestAnimationFrame(render);
}