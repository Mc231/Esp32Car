// Lightweight visual obstacle hint for autonomous mode.
//
// Periodically draws the camera <img> to a hidden canvas, slices the
// lower half into left / center / right thirds, and computes a
// "clutter" score per third (mean absolute luminance gradient — a cheap
// stand-in for edge density). Most-cluttered third is most likely an
// obstacle; least-cluttered is most likely a free path.
//
// The scores are NOT used as a hard veto on the IR sensor — IR remains
// the primary obstacle signal because it's distance-calibrated. Vision
// is consulted ONLY to seed the pivot direction in `findClearance`,
// turning the alternating-side heuristic into a directed search.
//
// CORS: works because the camera HTTP server (app_httpd.cpp) sets
// `Access-Control-Allow-Origin: *` on the /stream response. The page's
// <img> must have `crossorigin="anonymous"` set BEFORE its src is
// assigned, otherwise getImageData throws on a tainted canvas.

const FRAME_W = 80;
const FRAME_H = 60;
const SAMPLE_EVERY_MS_DEFAULT = 500;
const TIE_EPS_DEFAULT = 0.04;          // |max - min| below this → call it a tie
const SIGNAL_FLOOR    = 0.03;          // below this even the worst third looks blank

// Compute the mean absolute luminance gradient inside [x0,y0)..[x1,y1)
// of an ImageData-like {data, width, height}. Cheap, no allocations
// beyond a single number. Range is normalised to 0..1.
export function clutterScore(img, x0, y0, x1, y1) {
  if (x1 <= x0 + 1 || y1 <= y0) return 0;
  let sum = 0;
  let count = 0;
  const w = img.width;
  for (let y = y0; y < y1; y++) {
    for (let x = x0; x < x1 - 1; x++) {
      const i = (y * w + x) * 4;
      const j = i + 4;
      const l1 = (img.data[i]   + img.data[i+1] + img.data[i+2]) / 3;
      const l2 = (img.data[j]   + img.data[j+1] + img.data[j+2]) / 3;
      sum += Math.abs(l1 - l2);
      count++;
    }
  }
  return count > 0 ? Math.min(1, (sum / count) / 255) : 0;
}

// Slice the lower half of `img` into thirds and clutter-score each.
export function analyzeFrame(img) {
  const w = img.width;
  const h = img.height;
  const halfY = (h / 2) | 0;
  const t1 = (w / 3)       | 0;
  const t2 = ((2 * w) / 3) | 0;
  return {
    left:   clutterScore(img, 0,  halfY, t1, h),
    center: clutterScore(img, t1, halfY, t2, h),
    right:  clutterScore(img, t2, halfY, w,  h),
  };
}

// Decide which side looks the most open. Returns 'left' / 'right' /
// 'center' / null. `null` means "no signal — fall back to caller's
// existing logic"; happens during loading or for fully-blank frames.
export function pickFreeSide(thirds, { tieEps = TIE_EPS_DEFAULT } = {}) {
  if (!thirds) return null;
  const { left, center, right } = thirds;
  const max = Math.max(left, center, right);
  if (max < SIGNAL_FLOOR) return null;
  const min = Math.min(left, center, right);
  if (max - min < tieEps) return 'center';
  // Lowest clutter wins. Ties between two non-center thirds resolve to
  // the side with strictly less clutter; if symmetric, prefer center.
  if (left  < center && left  <= right) return 'left';
  if (right < center && right <= left)  return 'right';
  return 'center';
}

// Wrapper around a hidden canvas — periodically snapshots the imgEl
// and exposes the latest analysis. Returns null from getters until
// the first successful sample.
//
// `logEvery`: how many samples between heartbeat console logs. Default
// 0 disables logging. Set to e.g. 4 for one log every 2 s at the
// default 500 ms cadence.
export function createVisionAnalyzer({
  imgEl,
  sampleEveryMs = SAMPLE_EVERY_MS_DEFAULT,
  frameW = FRAME_W,
  frameH = FRAME_H,
  logEvery = 0,
  onSample = null,                  // called with each {left, center, right}
  doc = (typeof document !== 'undefined' ? document : null),
} = {}) {
  if (!imgEl) throw new Error('createVisionAnalyzer: imgEl required');
  if (!doc)   throw new Error('createVisionAnalyzer: no document available');

  const canvas = doc.createElement('canvas');
  canvas.width = frameW;
  canvas.height = frameH;
  const ctx = canvas.getContext('2d');

  let lastThirds = null;
  let lastAt = 0;
  let lastError = null;
  let timer = null;
  let sampleCount = 0;
  let warnedNotReady = false;

  function sampleOnce() {
    // Bail if the img hasn't loaded a frame yet — drawImage on an empty
    // <img> is undefined behaviour across browsers.
    if (!imgEl.naturalWidth || !imgEl.naturalHeight) {
      if (!warnedNotReady && logEvery > 0) {
        console.log('[vision] waiting for first camera frame…');
        warnedNotReady = true;
      }
      return false;
    }
    try {
      ctx.drawImage(imgEl, 0, 0, frameW, frameH);
      const img = ctx.getImageData(0, 0, frameW, frameH);
      lastThirds = analyzeFrame(img);
      lastAt = (typeof performance !== 'undefined' ? performance.now() : Date.now());
      lastError = null;
      sampleCount++;
      try { onSample?.(lastThirds); } catch { /* logger errors mustn't break vision */ }
      if (logEvery > 0 && sampleCount % logEvery === 0) {
        const pick = pickFreeSide(lastThirds);
        console.log(
          `[vision] sample #${sampleCount} ` +
          `L=${lastThirds.left.toFixed(2)} ` +
          `C=${lastThirds.center.toFixed(2)} ` +
          `R=${lastThirds.right.toFixed(2)} ` +
          `→ ${pick ?? 'no signal'}`
        );
      }
      return true;
    } catch (e) {
      // Most likely SecurityError on tainted canvas — log once, then
      // suppress so we don't spam the console every tick.
      if (!lastError) console.warn('[vision] sample failed:', e.message);
      lastError = e;
      return false;
    }
  }

  function start() {
    stop();
    sampleOnce();
    timer = setInterval(sampleOnce, sampleEveryMs);
  }
  function stop() {
    if (timer) { clearInterval(timer); timer = null; }
  }

  return {
    start, stop, sampleOnce,
    getThirds:      () => lastThirds,
    getError:       () => lastError,
    getFreshnessMs: () => {
      if (!lastAt) return Infinity;
      const now = (typeof performance !== 'undefined' ? performance.now() : Date.now());
      return now - lastAt;
    },
    pickFreeSide: () => pickFreeSide(lastThirds),
  };
}
