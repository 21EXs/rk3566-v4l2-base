#!/bin/bash

cd /home/xs/桌面/Project/ssh/rk3566-v4l2-base/01-v4l2
make clean
make 
echo "v4l2编译完成"

cd /home/xs/桌面/Project/ssh/rk3566-v4l2-base/02-shm
make clean
make 
echo "shm编译完成"

cd /home/xs/桌面/Project/ssh/rk3566-v4l2-base/03-drm
make clean
make 
echo "drm编译完成"

# 编译 Qt GUI 程序
cd /home/xs/桌面/Project/ssh/rk3566-v4l2-base/06-gui/build/RK3566-Debug
cmake ../.. -DCMAKE_BUILD_TYPE=Debug
make clean
make -j4
echo "gui编译完成"

# 部署到 NFS 共享目录
cp /home/xs/桌面/Project/ssh/rk3566-v4l2-base/01-v4l2/v4l2_app /srv/nfs/share/
echo "v4l2部署完成"
cp /home/xs/桌面/Project/ssh/rk3566-v4l2-base/02-shm/shm_app /srv/nfs/share/
echo "shm部署完成"
cp /home/xs/桌面/Project/ssh/rk3566-v4l2-base/03-drm/drm_app /srv/nfs/share/
echo "drm部署完成"
cp /home/xs/桌面/Project/ssh/rk3566-v4l2-base/06-gui/build/RK3566-Debug/06-gui /srv/nfs/share/
echo "gui部署完成"
