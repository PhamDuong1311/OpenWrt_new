Dưới đây là mapping giữa source tree trong `openwrt/` và filesystem trong thiết bị sau khi sysupgrade:

# Nhóm 1: Cấu hình hệ thống 

| Thư mục trong thiết bị      | Tương ứng trong source code                                    | Vai trò                                                               |
| --------------------------- | -------------------------------------------------------------- | --------------------------------------------------------------------- |
| `/etc/`                     | `package/base-files/files/etc/` <br>hoặc `package/*//etc/` | File cấu hình mặc định, init script, UCI                              |
| `/bin/`                     | `package/base-files/files/bin/`                                | Lệnh cơ bản (`sh`, `login`, `mount`, …)                               |
| `/sbin/`                    | `package/base-files/files/sbin/`                               | Lệnh quản trị (`init`, `reboot`, …)                                   |
| `/usr/bin/`                 | `package/*/root/usr/bin/`                                      | Binary ứng dụng người dùng                                            |
| `/usr/sbin/`                | `package/*/root/usr/sbin/`                                     | Binary dịch vụ nền (daemons)                                          |
| `/usr/lib/`                 | `package/*/root/usr/lib/`                                      | Thư viện dùng chung (shared libraries)                                |
| `/lib/`                     | `package/base-files/files/lib/`                                | Thư viện hệ thống (`libc`, `ld.so`, module kernel loader, shell func) |
| `/lib/modules/$(uname -r)/` | `build_dir/target.../linux-.../linux-*/drivers/`               | Module kernel `.ko` sau khi build                                     |
| `/proc/`, `/sys/`, `/dev/`  | Không có trong source (runtime tạo bởi kernel)                 | Filesystem ảo (procfs, sysfs, devtmpfs)                               |

# Nhóm 2: Dịch vụ hệ thống

| Thư mục runtime   | Nguồn trong source                                                            | Chức năng                                      |
| ----------------- | ----------------------------------------------------------------------------- | ---------------------------------------------- |
| `/etc/init.d/`    | `package/*/root/etc/init.d/`                                                  | Script khởi động dịch vụ (sử dụng `procd`)     |
| `/etc/rc.d/`      | Sinh tự động khi boot                                                         | Liên kết symbolic tới `/etc/init.d/`           |
| `/lib/functions/` | `package/base-files/files/lib/functions/`                                     | Thư viện shell script chung                    |
| `/lib/netifd/`    | `package/network/config/netifd/files/lib/netifd/`                             | Script xử lý cấu hình mạng                     |
| `/lib/preinit/`   | `package/base-files/files/lib/preinit/`                                       | Logic khởi động hệ thống (early boot)          |
| `/etc/hotplug.d/` | `package/base-files/files/etc/hotplug.d/` <br>hoặc từ package device-specific | Xử lý sự kiện kernel (usb, net, ntp, iface, …) |

# Nhóm 3: Giao diện mạng và web

| Thư mục runtime          | Source                                                                        | Vai trò                                 |
| ------------------------ | ----------------------------------------------------------------------------- | --------------------------------------- |
| `/www/`                  | `feeds/luci/modules/luci-base/htdocs/`<br>`feeds/luci/applications/*/htdocs/` | Web root được `uhttpd` phục vụ          |
| `/www/cgi-bin/`          | Sinh từ `luci-base` hoặc package web khác                                     | CGI entrypoint (LuCI hoặc RPC endpoint) |
| `/usr/share/uhttpd/`     | `package/network/services/uhttpd/files/`                                      | File cấu hình mặc định uhttpd           |
| `/usr/share/rpcd/acl.d/` | `feeds/luci/applications/*/root/usr/share/rpcd/acl.d/`                        | Quyền truy cập RPC (LuCI / ubus)        |
| `/usr/libexec/rpcd/`     | `feeds/luci/modules/luci-base/root/usr/libexec/rpcd/`                         | Plugin RPCD Lua/C                       |

# Nhóm 4: Cấu hình và UCI

| Thư mục runtime      | Source                                                                       | Mô tả                                  |
| -------------------- | ---------------------------------------------------------------------------- | -------------------------------------- |
| `/etc/config/`       | `package/base-files/files/etc/config/` <br>hoặc `package/*/files/etc/config/` | File UCI cấu hình dịch vụ              |
| `/etc/uci-defaults/` | `package/base-files/files/etc/uci-defaults/`                                 | Script chạy 1 lần khi cài đặt firmware |
| `/etc/init.d/`       | như trên                                                                     | Script khởi động dịch vụ               |
| `/lib/uci/`          | `package/system/uci/files/lib/uci/`                                          | Thư viện UCI cho shell                 |
| `/usr/lib/lua/uci/`  | `package/system/uci/files/usr/lib/lua/uci/`                                  | Thư viện UCI cho Lua                   |


# Nhóm 5: Quản lý gói và hệ thống

| Thư mục runtime             | Source                                | Vai trò                   |
| --------------------------- | ------------------------------------- | ------------------------- |
| `/usr/lib/opkg/`            | `package/system/opkg/`                | Script hỗ trợ cài đặt gói |
| `/etc/opkg/`                | `package/system/opkg/files/etc/opkg/` | Cấu hình repo và chữ ký   |
| `/var/lock/`, `/tmp/opkg-*` | runtime                               | Dữ liệu tạm khi cài gói   |


# Nhóm 6: Ghi đè và filesystem runtime

| Thư mục runtime | Source                                    | Mô tả                                |
| --------------- | ----------------------------------------- | ------------------------------------ |
| `/rom/`         | Sinh ra từ rootfs squashfs trong firmware | Root filesystem gốc (read-only)      |
| `/overlay/`     | runtime (JFFS2 hoặc UBIFS)                | Layer ghi đè cho cấu hình người dùng |
| `/tmp/`         | runtime (tmpfs)                           | Dữ liệu tạm thời, log, socket        |
| `/var/`         | thường là symlink → `/tmp/`               | Biến tạm, PID, lock                  |
| `/root/`        | `package/base-files/files/root/`          | Home của root                        |
| `/home/`        | ít dùng trên OpenWrt                      | Home user bổ sung (nếu có)           |

