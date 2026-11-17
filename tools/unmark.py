"""Remove blue callout markers (line + dot) from a panel screenshot; touches only marker pixels + a 3px halo."""
import sys, numpy as np, cv2
from PIL import Image
src, dst = sys.argv[1], sys.argv[2]
im = np.array(Image.open(src).convert('RGBA'))
H,W = im.shape[:2]
r,g,b = [im[...,i].astype(int) for i in range(3)]
core = ((b-r>60)&(b-g>30)).astype(np.uint8)
# soft (anti-aliased) blue tint near the core
soft = ((b-r>12)&(b-g>6)).astype(np.uint8)
n,lab,st,_ = cv2.connectedComponentsWithStats(core,8)
out = im.copy()
dots = np.zeros((H,W),np.uint8)
for i in range(1,n):
    x,y,w,h,_a = st[i]
    if w<=3 and h>=20:                       # vertical line: interpolate each row across it
        for yy in range(y,y+h):
            xs = np.where(soft[yy,max(0,x-4):x+w+4]>0)[0]
            if len(xs)==0: continue
            lo = max(0,x-4+xs.min()-2); hi = min(W-1,x-4+xs.max()+2)
            l = out[yy,max(0,lo-1),:3].astype(float); rr = out[yy,min(W-1,hi+1),:3].astype(float)
            for xx in range(lo,hi+1):
                t=(xx-lo+1)/(hi-lo+2); out[yy,xx,:3]=(l*(1-t)+rr*t).round()
    elif w>=8 and h>=8:                      # dot
        dots[y:y+h,x:x+w]=1
    else:                                    # stray 1px tick on the image edge
        out[y:y+h,x:x+w,:3]=out[y:y+h,min(W-1,x+w+1):min(W-1,x+w+2),:3]
d = cv2.dilate(((dots>0)&((soft>0)|(core>0))).astype(np.uint8),np.ones((5,5),np.uint8))
d = cv2.dilate(dots,np.ones((5,5),np.uint8))
rgb = cv2.inpaint(out[...,:3][...,::-1].copy(),d,5,cv2.INPAINT_TELEA)[...,::-1]
out[...,:3][d>0]=rgb[d>0]
Image.fromarray(out).save(dst)
ch = (np.abs(out.astype(int)-im.astype(int)).sum(-1)>0)
ys,xs=np.where(ch)
print(src.split('/')[-1],'changed %d px = %.3f%%'%(ch.sum(),100*ch.mean()),'alpha diff',int((out[...,3]!=im[...,3]).sum()),'bbox',xs.min(),ys.min(),xs.max(),ys.max())
