"""Phase 2: replace each marker-damaged patch with the best-matching undamaged patch from elsewhere on the same panel.
Matching uses only the untouched pixels around the damage (template match with mask)."""
import sys, numpy as np, cv2
from PIL import Image
orig_p, clean_p, dst = sys.argv[1:4]
import json
OV={int(k):v for k,v in json.loads(sys.argv[4]).items()} if len(sys.argv)>4 else {}
orig = np.array(Image.open(orig_p).convert('RGBA')); cur = np.array(Image.open(clean_p).convert('RGBA'))
H,W = orig.shape[:2]
r,g,b = [orig[...,i].astype(int) for i in range(3)]
core = ((b-r>60)&(b-g>30)).astype(np.uint8); core[:, :2]=0   # ignore edge ticks
soft = ((b-r>12)&(b-g>6)).astype(np.uint8)
dmg = cv2.dilate(((soft>0)&(cv2.dilate(core,np.ones((9,9),np.uint8))>0)).astype(np.uint8)|core, np.ones((5,5),np.uint8))
n,lab,st,_ = cv2.connectedComponentsWithStats(core,8)
patches=[]
for i in range(1,n):
    x,y,w,h,_a=st[i]
    if h>=20 and w<=3:
        for yy in range(y,y+h,20): patches.append((x-6,yy,x+w+6,min(y+h,yy+20)))
    elif w>=8: patches.append((x-3,y-3,x+w+3,y+h+3))
out=cur.copy(); log=[]; idx=-1
gray=cv2.cvtColor(cur[...,:3],cv2.COLOR_RGB2GRAY).astype(np.float32)
for (x0,y0,x1,y1) in patches:
    idx+=1
    x0=max(0,x0);y0=max(0,y0);x1=min(W,x1);y1=min(H,y1)
    pw,ph=x1-x0,y1-y0; pad=5
    X0=max(0,x0-pad);Y0=max(0,y0-pad);X1=min(W,x1+pad);Y1=min(H,y1+pad)
    tpl=cur[Y0:Y1,X0:X1,:3].astype(np.float32)
    tm=np.ones((Y1-Y0,X1-X0),np.float32)
    tm[(Y0<=0)]=1
    # exclude damaged pixels (anywhere) and the patch interior from the template ring
    dm=dmg[Y0:Y1,X0:X1]>0
    tm[dm]=0; tm[y0-Y0:y1-Y0, x0-X0:x1-X0]=0
    if tm.sum()<30: continue
    res=cv2.matchTemplate(cur[...,:3].astype(np.float32),tpl,cv2.TM_SQDIFF,mask=np.dstack([tm]*3))
    res/= tm.sum()*3
    # forbid candidates whose window touches damage, or overlaps the destination
    k=np.ones((Y1-Y0,X1-X0),np.float32)
    bad=cv2.filter2D(dmg.astype(np.float32),-1,k,anchor=(0,0),borderType=cv2.BORDER_CONSTANT)[:res.shape[0],:res.shape[1]]
    res[bad>0]=1e18
    yy,xx=np.mgrid[0:res.shape[0],0:res.shape[1]]
    res[(abs(xx-X0)<(X1-X0)+2)&(abs(yy-Y0)<(Y1-Y0)+2)]=1e18
    sy,sx=np.unravel_index(np.argmin(res),res.shape); score=float(res[sy,sx])**.5
    dy,dx=sy-Y0,sx-X0
    use=OV.get(idx,'auto' if score<=6.5 else 'interp')
    log.append((idx,use,round(score,1)))
    if use=='interp': continue
    m=dmg[y0:y1,x0:x1]>0
    src=cur[y0+dy:y1+dy,x0+dx:x1+dx]
    blk=out[y0:y1,x0:x1]; blk[m]=src[m]
Image.fromarray(out).save(dst)
for l in log: print(l)
ch=(np.abs(out.astype(int)-Image.open(orig_p).convert('RGBA')).__array__().astype(int)).sum(-1)>0
print(dst.split('/')[-1],'total changed vs original %.3f%%'%(100*ch.mean()),'alpha diff',int((out[...,3]!=orig[...,3]).sum()))
