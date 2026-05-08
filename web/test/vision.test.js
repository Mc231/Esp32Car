// Tests for the vision module's pure analysis functions. The wrapper
// (createVisionAnalyzer) needs canvas which jsdom only partly implements
// — exercise it via injectable hooks instead of real <canvas>.
import { describe, it, expect, vi } from 'vitest';
import { clutterScore, analyzeFrame, pickFreeSide, createVisionAnalyzer } from '../js/vision.js';

// Build a synthetic ImageData-like object filled with a colour, then
// stamp a high-contrast vertical stripe across part of it. Useful for
// shaping per-third clutter scores.
function makeFrame(w, h, baseGrey = 128) {
  const data = new Uint8ClampedArray(w * h * 4);
  for (let i = 0; i < w * h; i++) {
    data[i * 4]     = baseGrey;
    data[i * 4 + 1] = baseGrey;
    data[i * 4 + 2] = baseGrey;
    data[i * 4 + 3] = 255;
  }
  return { data, width: w, height: h };
}

// Paints a checkerboard pattern in the rect — produces high clutter score.
function paintCheckerboard(img, x0, y0, x1, y1) {
  for (let y = y0; y < y1; y++) {
    for (let x = x0; x < x1; x++) {
      const i = (y * img.width + x) * 4;
      const v = (x + y) % 2 === 0 ? 0 : 255;
      img.data[i]     = v;
      img.data[i + 1] = v;
      img.data[i + 2] = v;
    }
  }
}

describe('clutterScore', () => {
  it('returns 0 for a uniform region', () => {
    const img = makeFrame(40, 40, 128);
    expect(clutterScore(img, 0, 0, 40, 40)).toBe(0);
  });

  it('returns ~1 for maximum-contrast checkerboard', () => {
    const img = makeFrame(40, 40, 128);
    paintCheckerboard(img, 0, 0, 40, 40);
    const s = clutterScore(img, 0, 0, 40, 40);
    expect(s).toBeGreaterThan(0.9);
    expect(s).toBeLessThanOrEqual(1);
  });

  it('returns 0 for a degenerate region (zero width or height)', () => {
    const img = makeFrame(40, 40, 128);
    expect(clutterScore(img, 0, 0, 0, 40)).toBe(0);
    expect(clutterScore(img, 0, 0, 40, 0)).toBe(0);
    expect(clutterScore(img, 0, 0, 1, 40)).toBe(0);   // < 2 px wide
  });

  it('only measures the requested rect', () => {
    const img = makeFrame(60, 40, 128);
    paintCheckerboard(img, 0, 0, 30, 40);   // left half = busy
    const left  = clutterScore(img, 0,  0, 30, 40);
    const right = clutterScore(img, 30, 0, 60, 40);
    expect(left).toBeGreaterThan(0.5);
    expect(right).toBe(0);
  });
});

describe('analyzeFrame', () => {
  it('partitions the lower half into thirds', () => {
    // 60 wide → thirds at 20, 40. Bottom half is rows 30..59.
    const img = makeFrame(60, 60, 128);
    paintCheckerboard(img, 0, 30, 20, 60);   // left third busy
    const t = analyzeFrame(img);
    expect(t.left).toBeGreaterThan(0.5);
    expect(t.center).toBe(0);
    expect(t.right).toBe(0);
  });

  it('ignores clutter in the upper half', () => {
    const img = makeFrame(60, 60, 128);
    paintCheckerboard(img, 0, 0, 60, 30);    // top half ALL busy
    const t = analyzeFrame(img);
    expect(t.left).toBe(0);
    expect(t.center).toBe(0);
    expect(t.right).toBe(0);
  });

  it('detects center clutter independently from left/right', () => {
    const img = makeFrame(60, 60, 128);
    paintCheckerboard(img, 20, 30, 40, 60);  // center third busy
    const t = analyzeFrame(img);
    expect(t.center).toBeGreaterThan(0.5);
    expect(t.left).toBe(0);
    expect(t.right).toBe(0);
  });
});

describe('pickFreeSide', () => {
  it('returns null when all thirds are below the signal floor', () => {
    expect(pickFreeSide({ left: 0, center: 0, right: 0 })).toBeNull();
    expect(pickFreeSide({ left: 0.01, center: 0.02, right: 0.01 })).toBeNull();
  });

  it('returns null on missing input', () => {
    expect(pickFreeSide(null)).toBeNull();
    expect(pickFreeSide(undefined)).toBeNull();
  });

  it('returns center when scores are within the tie epsilon', () => {
    expect(pickFreeSide({ left: 0.5, center: 0.51, right: 0.5 })).toBe('center');
  });

  it('picks the LOW-clutter (free) side when one side is clearly clearer', () => {
    expect(pickFreeSide({ left: 0.05, center: 0.5, right: 0.6 })).toBe('left');
    expect(pickFreeSide({ left: 0.6,  center: 0.5, right: 0.05 })).toBe('right');
  });

  it('picks an edge over a busy center even when left and right tie', () => {
    // left and right both lower than center, but equal to each other.
    // Either edge is correct; tiebreak resolves to 'left' by check order.
    const pick = pickFreeSide({ left: 0.1, center: 0.6, right: 0.1 });
    expect(['left', 'right']).toContain(pick);
  });

  it('honors a custom tie epsilon', () => {
    // With default eps (0.04), .55 vs .60 is NOT a tie → picks left.
    expect(pickFreeSide({ left: 0.55, center: 0.60, right: 0.65 })).toBe('left');
    // With a wider eps, the same input registers as a tie → center.
    expect(pickFreeSide(
      { left: 0.55, center: 0.60, right: 0.65 },
      { tieEps: 0.2 }
    )).toBe('center');
  });
});

describe('createVisionAnalyzer', () => {
  function makeFakeDoc() {
    const fakeCtx = {
      drawImage: vi.fn(),
      getImageData: vi.fn(() => makeFrame(80, 60, 0)),  // uniform → clutter 0
    };
    const fakeCanvas = { width: 0, height: 0, getContext: () => fakeCtx };
    return {
      createElement: vi.fn(() => fakeCanvas),
      _ctx: fakeCtx,
      _canvas: fakeCanvas,
    };
  }

  it('throws when imgEl is missing', () => {
    expect(() => createVisionAnalyzer({ doc: makeFakeDoc() })).toThrow(/imgEl/);
  });

  it('throws when no document is available', () => {
    expect(() => createVisionAnalyzer({ imgEl: { naturalWidth: 80, naturalHeight: 60 }, doc: null }))
      .toThrow(/document/);
  });

  it('returns null analysis until first sample is taken', () => {
    const v = createVisionAnalyzer({
      imgEl: { naturalWidth: 0, naturalHeight: 0 },
      doc: makeFakeDoc(),
    });
    expect(v.getThirds()).toBeNull();
    expect(v.pickFreeSide()).toBeNull();
  });

  it('skips drawing when the img has not loaded a frame', () => {
    const doc = makeFakeDoc();
    const v = createVisionAnalyzer({
      imgEl: { naturalWidth: 0, naturalHeight: 0 },
      doc,
    });
    expect(v.sampleOnce()).toBe(false);
    expect(doc._ctx.drawImage).not.toHaveBeenCalled();
  });

  it('analyses a frame once the img is ready', () => {
    const doc = makeFakeDoc();
    const v = createVisionAnalyzer({
      imgEl: { naturalWidth: 80, naturalHeight: 60 },
      doc,
    });
    expect(v.sampleOnce()).toBe(true);
    expect(doc._ctx.drawImage).toHaveBeenCalledOnce();
    expect(v.getThirds()).toEqual({ left: 0, center: 0, right: 0 });
  });

  it('captures and exposes errors thrown by getImageData (tainted canvas)', () => {
    const doc = makeFakeDoc();
    doc._ctx.getImageData = vi.fn(() => { throw new Error('SecurityError'); });
    const warn = vi.spyOn(console, 'warn').mockImplementation(() => {});
    const v = createVisionAnalyzer({
      imgEl: { naturalWidth: 80, naturalHeight: 60 },
      doc,
    });
    expect(v.sampleOnce()).toBe(false);
    expect(v.getError()).toBeInstanceOf(Error);
    warn.mockRestore();
  });
});
