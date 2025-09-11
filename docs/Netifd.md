
## Netifd là gì?

`netifd` là một **daemon có khả năng RPC** được viết bằng ngôn ngữ C để truy cập tốt hơn vào các API của kernel với khả năng lắng nghe các **netlink event**.

## Tại sao chúng ta cần netifd?

Một điều mà `netifd` làm tốt hơn nhiều so với **các script cấu hình mạng cũ của OpenWrt** là xử lý các thay đổi cấu hình. Với `netifd`, khi file `/etc/config/network` thay đổi, bạn không còn phải khởi động lại tất cả các giao diện nữa. Đơn giản chỉ cần chạy lệnh `/etc/init.d/network reload`. Lệnh này sẽ thực hiện một `ubus-call` tới `netifd`, yêu cầu nó tìm ra sự khác biệt giữa trạng thái runtime và cấu hình mới, sau đó chỉ áp dụng phần thay đổi đó.

## Thiết kế của `netifd`

![alt text](image/netifd_layer.png)

`Netifd` hoạt động chính từ **Layer 2** tới **Layer 4** trong **Network management stack**:
- **Layer 2 - Device Layer**: Netifd quản lý device abstraction thông qua `struct device` (**device.h**) - đại diện cho các physical và virtual network interface tại link layer (bridges, VLANs, tunnels).
- **Layer 3 - Interface Layer**: Trung tâm core của netifd ở trong `struct interface` (**interface.h**) - đại diện cho các logical network connection (Layer này xử lý cấu hình IP,địa chỉ, định tuyến và cài đặt DNS settings).
- **Layer 4 - Protocol Layer**: Netifd quản lý các **protocol hander** thông qua **proto.c** và hỗ trợ nhiều protocol như static IP, DHCP và PPP.


## Một số flow hoạt động trong netifd
### 1. Main execution flow (main.c)
- Xử lý command-line: Handle debug mask, socket path, config path...
- Khởi tạo core system: Thiết lập signal, uloop event system và udebug.
- Thiết lập Ubus (cơ chế IPC chính).
- Khởi tạo subsystem: Khởi tạo protocol shell, external device và wireless.
- System backend: Khởi tạo Linux system interface.
- Tải configuration: Load tất cả các config mạng.
### 2. Interface state flow (interface.c)
Sự chuyển tiếp của interface state được xử lý bởi `interface_proto_event_cb()`:
- `IFPEV_UP`: Chuyển state từ `IFS_SETUP` sang `IFS_UP`, cho phép IP settings và gửi event **interface up**
- `IFPEV_DOWN`: Đánh dấu **interface down**, giải phóng device và xử lý các thay đổi config
- `IFPEV_LINK_LOST`: Xử lý các mất kết nối trong khi đang duy trì setup state
### 3. Ubus communication flow (ubus.c)
Netifd cung cấp nhiều ubus object cho giao tiếp ngoại:
- Main network object ("network"): Handle các global method (restart, reload, protocol handler)
- Device object ("network.device"): Quản lý device state và configuration
- Per-interface objects ("network.interface.<name>): Kiểm soát các interface riêng lẻ

Ubus interface còn hỗ trợ cả synchronous method calls và asynchronous event notifications.
### 4. Wireless device flow (wireless.c)
Các Wireless device tuân theo lifecycle riêng của chúng, được quản lý bởi các script ngoại:
- Setup: Thực thi script với cấu hình device cụ thể
- Monitoring: Theo dõi process và kiểm tra status
- Retry logic: Tự động thử lại khi failure
- Teardown: Shutdown và terminate process
### 5. Procss management flow (main.c)
Netifd quản lý các external process thông qua 1 cách tiếp cận có cấu trúc:
- Mô hình Fork/exec với pipe/based logging
- Track process trong 1 global list
- Tự động terminate trên daemon shutdown.
### 6. Configuration loading flow (config.c)
Quá trình loading config được handle bởi `config_init_all()`. Luồng xử lý thực hiện;
- Load UCI package: Khởi tạo cấu hình 2 package **network, wireless**
- Khởi tạo các component: Device, VLAN, interface, IP setting, rule, global, wireless.
- Kích hoạt các cấu hình: Reset các device cũ, khởi tạo các pending device và bắt đầu các interface
