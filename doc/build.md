# m1882 构建与刷入教程

从零构建 postmarketOS 并刷入魅族 16th（m1882）。

---

## 前置条件

- `pmbootstrap`,`android-tools`,`python3`

- 魅族 16th（M1882），`已解锁` bootloader
- 能进 fastboot 模式

## 从零构建并刷入

### 0. 克隆 + 把三个包接进 pmaports

pmbootstrap 只从 pmaports 检出里找设备，所以要把本仓库的三个包软链过去：

```sh
git clone https://github.com/GH4NG/meizu-m1882-mainline-linux.git
cd meizu-m1882-mainline-linux

PMAPORTS="$(pmbootstrap -q config aports | tail -1)"
for p in device-meizu-m1882 firmware-meizu-m1882 linux-meizu-m1882; do
    ln -sfn "$PWD/pmos/device/testing/$p" "$PMAPORTS/device/testing/$p"
done
```

（pmaports 升级或切分支后链接可能被清掉，重跑这段即可。）

### 1. 初始化 pmbootstrap

```sh
pmbootstrap init                 # vendor 选 meizu，device 选 m1882
```

### 2. 编内核与设备树

```sh
cd kernel

shopt -s expand_aliases
source ~/pmbootstrap/helpers/envkernel.sh

make Image.gz dtbs modules -j"$(nproc)"

mkdir -p ../out
cp .output/arch/arm64/boot/Image.gz \
   .output/arch/arm64/boot/dts/qcom/sdm845-meizu-m1882.dtb ../out/
```

产物：`out/Image.gz`、`out/sdm845-meizu-m1882.dtb`（增量编译，只改 DTS 时约一分钟）。

- 内核配置改 `config/sdm845-meizu-m1882.config`
- 设备树改 `config/sdm845-meizu-m1882.dts`
- **`shopt -s expand_aliases` 不能省**：别名在非交互 shell 里默认不展开，少了这行 `make`
  会静默改用宿主工具链，在源码树里编出一份 x86_64 内核 —— 而且**不报错**
- 上面几行要**逐行执行**，别塞进 `bash -c "..."`：别名是解析期展开的，一行式会在
  别名定义之前就把 `make` 解析掉了
- 报 `No rule to make target 'olddefconfig'` 时，是 envkernel 的挂载标记残留：

  ```sh
  WORK="$(pmbootstrap -q config work)"
  sudo umount -l "$WORK/chroot_native/mnt/linux" 2>/dev/null
  sudo rm -rf "$WORK/chroot_native/tmp/envkernel"
  ```

### 3. 打成 pmOS 内核包

```sh
pmbootstrap build --envkernel linux-meizu-m1882
```

它**打包刚编好的 `.output`，不重新编译、也不下载源码**，一分钟内出包。
（改了 DTS 或配置后一定要先回第 2 步重编，否则打出来的是旧产物。）

### 4. 生成 rootfs 镜像

```sh
pmbootstrap install --password <给手机用户设的密码>
pmbootstrap export
```

### 5. 打包 boot 镜像

```sh
cp /tmp/postmarketOS-export/initramfs out/initramfs.cpio.gz
cat out/Image.gz out/sdm845-meizu-m1882.dtb > out/kernel-dtb

mkbootimg \
  --base 0x0 --kernel_offset 0x8000 --ramdisk_offset 0x1000000 \
  --tags_offset 0x100 --pagesize 4096 --second_offset 0xf00000 \
  --cmdline "console=ttyMSM0,115200n8 console=tty0 earlycon=msm_geni_serial,0xA84000 androidboot.hardware=qcom androidboot.console=ttyMSM0 androidboot.configfs=true androidboot.usbcontroller=a600000.dwc3 buildvariant=user keep_bootcon loglevel=7 ignore_loglevel pmos.stowaway" \
  --ramdisk out/initramfs.cpio.gz \
  --kernel out/kernel-dtb \
  -o out/mainline-boot.img
```

### 6. 刷入

```sh
fastboot flash system /tmp/postmarketOS-export/meizu-m1882.img
fastboot flash boot out/mainline-boot.img
fastboot erase dtbo
fastboot reboot
```
