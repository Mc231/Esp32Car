// Tests for the live-input → motor-command translator.
import { describe, it, expect } from 'vitest';
import {
  computeMotorCommands,
  FORWARD, BACKWARD, STOP, ARC_INNER_RATIO,
} from '../js/drive.js';

const SPEED = 200;

describe('computeMotorCommands — nothing held', () => {
  it('returns STOP/STOP/0/0 when no inputs are held', () => {
    expect(computeMotorCommands({}, SPEED)).toEqual({
      leftAction: STOP, rightAction: STOP, leftPwm: 0, rightPwm: 0,
    });
  });

  it('accepts a Set as input', () => {
    expect(computeMotorCommands(new Set(), SPEED).leftAction).toBe(STOP);
  });

  it('accepts a null/undefined held argument', () => {
    expect(computeMotorCommands(null,      SPEED).leftAction).toBe(STOP);
    expect(computeMotorCommands(undefined, SPEED).leftAction).toBe(STOP);
  });
});

describe('computeMotorCommands — pure linear', () => {
  it('forward only: both motors forward at full speed', () => {
    expect(computeMotorCommands({ fwd: true }, SPEED)).toEqual({
      leftAction: FORWARD, rightAction: FORWARD, leftPwm: SPEED, rightPwm: SPEED,
    });
  });

  it('backward only: both motors backward at full speed', () => {
    expect(computeMotorCommands({ back: true }, SPEED)).toEqual({
      leftAction: BACKWARD, rightAction: BACKWARD, leftPwm: SPEED, rightPwm: SPEED,
    });
  });

  it('forward AND backward held → stop (cancellation)', () => {
    expect(computeMotorCommands({ fwd: true, back: true }, SPEED)).toEqual({
      leftAction: STOP, rightAction: STOP, leftPwm: 0, rightPwm: 0,
    });
  });
});

describe('computeMotorCommands — pure turn (tank pivot)', () => {
  it('left only: left motor backward, right motor forward', () => {
    expect(computeMotorCommands({ left: true }, SPEED)).toEqual({
      leftAction: BACKWARD, rightAction: FORWARD, leftPwm: SPEED, rightPwm: SPEED,
    });
  });

  it('right only: left motor forward, right motor backward', () => {
    expect(computeMotorCommands({ right: true }, SPEED)).toEqual({
      leftAction: FORWARD, rightAction: BACKWARD, leftPwm: SPEED, rightPwm: SPEED,
    });
  });

  it('left AND right held → stop (cancellation)', () => {
    expect(computeMotorCommands({ left: true, right: true }, SPEED)).toEqual({
      leftAction: STOP, rightAction: STOP, leftPwm: 0, rightPwm: 0,
    });
  });
});

describe('computeMotorCommands — arc (linear + turn)', () => {
  const innerPwm = Math.round(SPEED * ARC_INNER_RATIO);

  it('forward + left: both forward, LEFT wheel reduced', () => {
    expect(computeMotorCommands({ fwd: true, left: true }, SPEED)).toEqual({
      leftAction: FORWARD, rightAction: FORWARD,
      leftPwm: innerPwm, rightPwm: SPEED,
    });
  });

  it('forward + right: both forward, RIGHT wheel reduced', () => {
    expect(computeMotorCommands({ fwd: true, right: true }, SPEED)).toEqual({
      leftAction: FORWARD, rightAction: FORWARD,
      leftPwm: SPEED, rightPwm: innerPwm,
    });
  });

  it('backward + left: both backward, LEFT wheel reduced (arc continues)', () => {
    expect(computeMotorCommands({ back: true, left: true }, SPEED)).toEqual({
      leftAction: BACKWARD, rightAction: BACKWARD,
      leftPwm: innerPwm, rightPwm: SPEED,
    });
  });

  it('backward + right: both backward, RIGHT wheel reduced', () => {
    expect(computeMotorCommands({ back: true, right: true }, SPEED)).toEqual({
      leftAction: BACKWARD, rightAction: BACKWARD,
      leftPwm: SPEED, rightPwm: innerPwm,
    });
  });

  it('forward + left + right: turns cancel → straight forward', () => {
    expect(computeMotorCommands({ fwd: true, left: true, right: true }, SPEED)).toEqual({
      leftAction: FORWARD, rightAction: FORWARD,
      leftPwm: SPEED, rightPwm: SPEED,
    });
  });
});

describe('computeMotorCommands — Set form', () => {
  it('mirrors the object form for forward + right', () => {
    const innerPwm = Math.round(SPEED * ARC_INNER_RATIO);
    const set = new Set(['forward', 'right']);
    expect(computeMotorCommands(set, SPEED)).toEqual({
      leftAction: FORWARD, rightAction: FORWARD,
      leftPwm: SPEED, rightPwm: innerPwm,
    });
  });
});

describe('computeMotorCommands — speed clamping', () => {
  it('clamps negative speed to 0', () => {
    expect(computeMotorCommands({ fwd: true }, -50).leftPwm).toBe(0);
  });

  it('clamps over-255 speed to 255', () => {
    expect(computeMotorCommands({ fwd: true }, 9999).leftPwm).toBe(255);
  });

  it('treats non-numeric speed as 0', () => {
    expect(computeMotorCommands({ fwd: true }, NaN).leftPwm).toBe(0);
  });

  it('inner wheel scales with the (clamped) base speed', () => {
    const cmd = computeMotorCommands({ fwd: true, left: true }, 100);
    expect(cmd.rightPwm).toBe(100);
    expect(cmd.leftPwm).toBe(Math.round(100 * ARC_INNER_RATIO));
  });
});
