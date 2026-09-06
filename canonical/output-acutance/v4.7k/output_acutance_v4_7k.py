from __future__ import annotations
import numpy as np
import cv2

LUMA = np.array([0.2126,0.7152,0.0722],np.float32)

def smoothstep01(x):
    x=np.clip(x,0.0,1.0)
    return x*x*(3.0-2.0*x)

def luminance(rgb):
    return np.tensordot(np.maximum(np.asarray(rgb,np.float32),0),LUMA,axes=([-1],[0])).astype(np.float32)

def _pad_edge(y):
    return np.pad(np.asarray(y,np.float32),((1,1),(1,1)),mode='edge')

def blur3(y):
    p=_pad_edge(y); k=(0.20,0.60,0.20)
    h=k[0]*p[1:-1,0:-2]+k[1]*p[1:-1,1:-1]+k[2]*p[1:-1,2:]
    ph=np.pad(h,((1,1),(0,0)),mode='edge')
    return (k[0]*ph[0:-2,:]+k[1]*ph[1:-1,:]+k[2]*ph[2:,:]).astype(np.float32)

def sobel_grad(y):
    p=_pad_edge(y)
    gx=(-p[0:-2,0:-2]+p[0:-2,2:]-2*p[1:-1,0:-2]+2*p[1:-1,2:]-p[2:,0:-2]+p[2:,2:])*0.25
    gy=(-p[0:-2,0:-2]-2*p[0:-2,1:-1]-p[0:-2,2:]+p[2:,0:-2]+2*p[2:,1:-1]+p[2:,2:])*0.25
    return np.sqrt(gx*gx+gy*gy).astype(np.float32)

def choose_output_acutance_plan(noise_sigma_at_2pct: float, resize_ratio: float, profile: str='adaptive_detail'):
    # Output acutance compensates only for final-resize/display softening. It must not
    # attempt to recover optical/sensor detail and must remain weaker for noisy scenes.
    n=float(noise_sigma_at_2pct)
    q=float(1.0-smoothstep01((n-0.00125)/(0.00180-0.00125))) if n>0 else 0.65
    rr=max(float(resize_ratio),1.0)
    resize_need=float(smoothstep01((rr-1.0)/1.25))
    # A very small base remains even near 1:1 to compensate output/JPEG/display blur.
    base=(0.050 + 0.110*resize_need)*q
    if profile=='skin_safe':
        base*=0.86
    elif profile=='neutral':
        base*=0.72
    return {
        'schema':'TruthRawOutputAcutancePlan/4.7k',
        'noise_sigma_at_2pct':n,
        'noise_confidence':q,
        'resize_ratio':rr,
        'resize_need':resize_need,
        'strength':float(np.clip(base,0.012,0.130)),
        'blur_kernel':'separable_0.20_0.60_0.20',
        'delta_cap':float(0.0045+0.0025*q),
        'hard_edge_norm_start':0.035,
        'hard_edge_norm_full':0.18,
        'shadow_start':0.012,
        'shadow_full':0.07,
        'highlight_start':0.80,
        'highlight_full':1.03,
        'scientific_master_modified':False,
        'color_policy':'luminance_only_rgb_direction_preserved',
    }

def apply_output_acutance(rgb_linear: np.ndarray, plan: dict):
    rgb=np.maximum(np.asarray(rgb_linear,np.float32),0)
    Y=luminance(rgb)
    blur=blur3(Y)
    detail=Y-blur
    grad=sobel_grad(Y)
    # Normalize edge magnitude by local brightness so white/black edges are treated similarly.
    edge_norm=grad/np.maximum(blur,0.035)
    e0=float(plan['hard_edge_norm_start']); e1=float(plan['hard_edge_norm_full'])
    hard=smoothstep01((edge_norm-e0)/max(e1-e0,1e-8))
    edge_guard=1.0-0.75*hard
    # Suppress deep shadow and near-highlight acutance. This avoids raising noise and ringing.
    s0=float(plan['shadow_start']); s1=float(plan['shadow_full'])
    shadow_gate=0.18+0.82*smoothstep01((Y-s0)/max(s1-s0,1e-8))
    h0=float(plan['highlight_start']); h1=float(plan['highlight_full'])
    high_gate=1.0-0.82*smoothstep01((Y-h0)/max(h1-h0,1e-8))
    # Flat/noise regions receive less boost. Threshold is scene-noise aware and scales
    # approximately with the target resize ratio because downsampling reduces white noise.
    n=float(plan['noise_sigma_at_2pct'])/max(float(plan['resize_ratio']),1.0)**0.5
    dsnr=np.abs(detail)/max(n,2e-4)
    texture_conf=0.35+0.65*smoothstep01((dsnr-0.7)/2.0)
    strength=float(plan['strength'])
    # Noise-aware soft threshold: output acutance should not amplify a detail coefficient
    # until it rises above a fraction of the expected post-resize noise floor.
    shrink=np.maximum(np.abs(detail)-0.35*max(n,2e-4),0.0)/np.maximum(np.abs(detail),1e-8)
    detail_eff=detail*shrink
    delta=strength*detail_eff*edge_guard*shadow_gate*high_gate*texture_conf
    cap=float(plan['delta_cap'])
    delta=np.clip(delta,-cap,cap)
    Yout=np.maximum(Y+delta,0)
    scale=np.ones_like(Y,dtype=np.float32)
    np.divide(Yout,Y,out=scale,where=Y>1e-8)
    out=rgb*scale[...,None]
    return out.astype(np.float32), {
        'mean_abs_delta_y':float(np.mean(np.abs(delta))),
        'p99_abs_delta_y':float(np.percentile(np.abs(delta),99)),
        'max_abs_delta_y':float(np.max(np.abs(delta))),
        'mean_edge_guard':float(np.mean(edge_guard)),
        'mean_texture_conf':float(np.mean(texture_conf)),
    }
