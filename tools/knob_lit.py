"""Realistic top-lit knob frames (the approved L'Cronx knob look): black 16-lobe scalloped skirt + domed coloured cap with a painted
pointer. The light is fixed in panel space (from straight above), so only the knob's own parts turn and every frame is lit the same.
frame(albedo_0to1, angle_deg, px) -> RGBA uint8 (px x px), transparent background, soft non-rotating drop shadow."""
import math
import numpy as np
from PIL import Image
from scipy import ndimage as ndi

S = 2; W = 400; N = W * S; C = N / 2.0; LOBES = 16
RS, AMP = 164.7 * S, 6.0 * S; RC = 141.5 * S
yy, xx = np.mgrid[0:N, 0:N].astype(float); X = xx - C + .5; Y = yy - C + .5
TEX = ndi.gaussian_filter(np.random.default_rng(7).standard_normal((N, N)), 1.8 * S / 3 * 1.5); TEX /= TEX.std()
sm_ = lambda t: (lambda u: u * u * (3 - 2 * u))(np.clip(t, 0, 1))


def frame(albedo, angle_deg, px):
    albedo = np.array(albedo, float)
    a = math.radians(angle_deg); ca, sa = math.cos(a), math.sin(a)
    kx = X * ca + Y * sa; ky = -X * sa + Y * ca
    r = np.hypot(X, Y); th = np.arctan2(kx, -ky)
    rim = RS + AMP * np.cos(LOBES * th)
    d_sk = rim - r; t = np.clip(d_sk / (16 * S), 0, 1)
    h_sk = np.where(d_sk > 0, 26 * S * np.sqrt(1 - (1 - t) ** 2), 0)
    d_cp = RC - r; t2 = np.clip(d_cp / (11 * S), 0, 1)
    edge = np.sqrt(1 - (1 - t2) ** 2)
    dome = 7 * S * (1 - np.clip(r / RC, 0, 1) ** 2)
    h_cp = np.where(d_cp > 0, 22 * S * edge + dome, 0)
    rings = .35 * S * np.sin(2 * math.pi * r / (2.6 * S))
    grain = ndi.map_coordinates(TEX, [ky + C, kx + C], order=1, mode='reflect')
    h_cp += np.where(d_cp > 2 * S, rings * .5 + grain * .10 * S, 0)
    h = ndi.gaussian_filter(h_sk + h_cp, .7 * S)
    gy_, gx_ = np.gradient(h)
    nx, ny = -gx_, -gy_; nl = np.sqrt(nx * nx + ny * ny + 1); nx, ny, nz = nx / nl, ny / nl, 1 / nl
    L = np.array([0, -.5, .866]); L /= np.linalg.norm(L)
    Hh = L + np.array([0, 0, 1.]); Hh /= np.linalg.norm(Hh)
    ndl = np.clip(nx * L[0] + ny * L[1] + nz * L[2], 0, 1)
    ndh = np.clip(nx * Hh[0] + ny * Hh[1] + nz * Hh[2], 0, 1)
    amb = .33 + .10 * nz
    blur = ndi.gaussian_filter(h, 10 * S)
    ao = np.clip(1 - .020 * np.clip(blur - h, 0, None) / S * 3, .55, 1)
    ao *= 1 - .45 * np.exp(-(np.abs(d_cp + 1.5 * S) / (5 * S)) ** 2) * (d_cp < 3 * S)
    on_cap = d_cp > 0
    alb = np.where(on_cap[..., None], albedo[None, None, :], np.array([.05, .05, .055])[None, None, :])
    kd = np.where(on_cap, .52, .55); ks = np.where(on_cap, .10, .30); sh = np.where(on_cap, 38., 70.)
    spec = ks * ndh ** sh
    y0, y1 = -RC, -52 * S
    dy = np.clip(ky, y0, y1) - ky; dist = np.hypot(kx, dy)
    ink = sm_((6. * S - dist) / (1.2 * S)) * on_cap
    ink *= sm_((RC - 1.5 * S - r) / (1. * S) + 1)
    alb = alb * (1 - ink[..., None]) + np.array([.045, .045, .05])[None, None, :] * ink[..., None]
    spec = spec * (1 - .6 * ink)
    lit = np.clip(alb * (amb + kd * ndl)[..., None] * ao[..., None] + spec[..., None] * .9 * ao[..., None], 0, 1)
    cov = sm_(d_sk / (1.2 * S) + .5)
    shd = ndi.gaussian_filter((np.hypot(X, Y - 9 * S) < RS + S).astype(float), 9 * S) * .6
    alpha = cov + (1 - cov) * shd
    col = lit * cov[..., None] / np.maximum(alpha, 1e-4)[..., None]
    rim_l = np.clip((-Y / RS) - .55, 0, 1) ** 2 * sm_((rim - r) / (5 * S)) * (1 - sm_((rim - r) / (14 * S)))
    col = np.clip(col + (rim_l * .10)[..., None] * (cov / np.maximum(alpha, 1e-4))[..., None], 0, 1)
    out = np.dstack([col, alpha])
    img = Image.fromarray((out * 255 + .5).astype(np.uint8), "RGBA")
    # knob skirt diameter (2 * 170.7 of the 400 px render) -> fills ~px; crop the render to that square around the centre
    full = px * 400 / (2 * 170.7 + 10)
    img = img.resize((round(full), round(full)), Image.LANCZOS)
    o = (img.size[0] - px) // 2
    return np.array(img.crop((o, o, o + px, o + px)))
