import cv2
import os
import sys
import glob
import numpy as np
import matplotlib.pyplot as plt

def get_edge(img_pth):
    img = cv2.imread(img_pth)
    img = cv2.flip(cv2.GaussianBlur(img,(5,5),0),0)
    img = cv2.Canny(img, 209, 300)
    return img


if __name__ == "__main__":
    
    name = sys.argv[1]
    out_dir = sys.argv[2]
    
    for i in range(0,360,15):
       
        dpth = os.path.join("data_depth","%s_%s_%d.png"%(name,"depth",i))
        npth = os.path.join("data_normal","%s_%s_%d.png"%(name,"normal",i))
        de = get_edge(dpth)
        ne = get_edge(npth)
        fe = de+ne
        #_,fe = cv2.threshold(fe,1,255,cv2.THRESH_BINARY)
        
        kernel = np.ones((3,3))
        #fe = cv2.dilate(fe, kernel)
        fe = 255-fe
        opth = os.path.join(out_dir,"%s_%d.png"%(name,i))
        #os.remove(dpth)
        #os.remove(npth)
        cv2.imwrite(opth,fe)
        cv2.imshow('dst',fe)
        cv2.waitKey(0)

#img_depth = cv2.imread(imgs_pth[0])
#img_normal = cv2.imread(img_pth[1])_


