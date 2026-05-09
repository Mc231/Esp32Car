// Tests for the snapshot helper. Canvas is mocked because jsdom's
// canvas implementation doesn't produce real Blobs.
import { describe, it, expect, vi } from 'vitest';
import { createSnapshotter } from '../js/snapshot.js';

function makeFakeDoc({ blob = new Blob(['x'], { type: 'image/jpeg' }), drawThrows = false, blobThrows = false } = {}) {
  const ctx = {
    drawImage: vi.fn(() => {
      if (drawThrows) throw new Error('SecurityError');
    }),
  };
  const canvas = {
    width: 0,
    height: 0,
    getContext: () => ctx,
    toBlob: vi.fn((cb /*, mime, quality */) => {
      if (blobThrows) throw new Error('encode failed');
      cb(blob);
    }),
  };
  return {
    createElement: vi.fn(() => canvas),
    _ctx: ctx,
    _canvas: canvas,
  };
}

describe('createSnapshotter', () => {
  it('throws when imgEl is missing', () => {
    expect(() => createSnapshotter({ doc: makeFakeDoc() })).toThrow(/imgEl/);
  });

  it('throws when no document is available', () => {
    expect(() => createSnapshotter({ imgEl: {}, doc: null })).toThrow(/document/);
  });

  it('returns null when the camera frame has not loaded yet', async () => {
    const doc = makeFakeDoc();
    const s = createSnapshotter({
      imgEl: { naturalWidth: 0, naturalHeight: 0 },
      doc,
    });
    const blob = await s.captureJpeg();
    expect(blob).toBeNull();
    expect(doc._ctx.drawImage).not.toHaveBeenCalled();
  });

  it('captures a JPEG blob from a loaded image', async () => {
    const fake = new Blob(['fake-jpeg-bytes'], { type: 'image/jpeg' });
    const doc = makeFakeDoc({ blob: fake });
    const s = createSnapshotter({
      imgEl: { naturalWidth: 320, naturalHeight: 240 },
      doc,
    });
    const blob = await s.captureJpeg();
    expect(blob).toBe(fake);
    expect(doc._ctx.drawImage).toHaveBeenCalledOnce();
    expect(doc._canvas.toBlob).toHaveBeenCalled();
    // Default mime + quality forwarded to toBlob.
    const args = doc._canvas.toBlob.mock.calls[0];
    expect(args[1]).toBe('image/jpeg');
  });

  it('returns null when drawImage taints the canvas', async () => {
    const doc = makeFakeDoc({ drawThrows: true });
    const s = createSnapshotter({
      imgEl: { naturalWidth: 320, naturalHeight: 240 },
      doc,
    });
    expect(await s.captureJpeg()).toBeNull();
  });

  it('returns null when toBlob throws', async () => {
    const doc = makeFakeDoc({ blobThrows: true });
    const s = createSnapshotter({
      imgEl: { naturalWidth: 320, naturalHeight: 240 },
      doc,
    });
    expect(await s.captureJpeg()).toBeNull();
  });

  it('honours custom width/height/quality', () => {
    const doc = makeFakeDoc();
    const s = createSnapshotter({
      imgEl: { naturalWidth: 1, naturalHeight: 1 },
      width: 160, height: 120, quality: 0.3,
      doc,
    });
    expect(s.width).toBe(160);
    expect(s.height).toBe(120);
    expect(doc._canvas.width).toBe(160);
    expect(doc._canvas.height).toBe(120);
  });
});
