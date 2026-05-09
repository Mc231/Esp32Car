// Captures the camera <img> as a JPEG Blob at a usable resolution
// (320×240 default) for after-the-fact event review. Separate from
// `vision.js` because vision's analysis canvas is intentionally tiny
// (80×60) for speed — too small to debug from visually.
//
// CORS: requires the <img> to have crossorigin="anonymous" so the
// canvas isn't tainted (already set in control.html).

const SNAPSHOT_W = 320;
const SNAPSHOT_H = 240;
const QUALITY    = 0.6;     // sweet spot — ~8-15 KB per frame

export function createSnapshotter({
  imgEl,
  width  = SNAPSHOT_W,
  height = SNAPSHOT_H,
  quality = QUALITY,
  doc = (typeof document !== 'undefined' ? document : null),
} = {}) {
  if (!imgEl) throw new Error('createSnapshotter: imgEl required');
  if (!doc)   throw new Error('createSnapshotter: no document available');

  const canvas = doc.createElement('canvas');
  canvas.width = width;
  canvas.height = height;
  const ctx = canvas.getContext('2d');

  // Returns a Promise<Blob | null>. Null when the camera frame isn't
  // ready yet (page just loaded, stream broken, tainted canvas).
  function captureJpeg() {
    if (!imgEl.naturalWidth || !imgEl.naturalHeight) return Promise.resolve(null);
    try {
      ctx.drawImage(imgEl, 0, 0, width, height);
    } catch {
      return Promise.resolve(null);    // tainted canvas — give up
    }
    return new Promise(resolve => {
      try {
        canvas.toBlob(b => resolve(b ?? null), 'image/jpeg', quality);
      } catch {
        resolve(null);
      }
    });
  }

  return { captureJpeg, width, height };
}
