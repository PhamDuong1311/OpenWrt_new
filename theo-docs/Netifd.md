
## I. Netifd là gì?

`netifd` là một **daemon có khả năng RPC** được viết bằng ngôn ngữ C để truy cập tốt hơn vào các API của kernel với khả năng lắng nghe các **netlink event**.

## II. Tại sao chúng ta cần netifd?

Một điều mà `netifd` làm tốt hơn nhiều so với **các script cấu hình mạng cũ của OpenWrt** là xử lý các thay đổi cấu hình. Với `netifd`, khi file `/etc/config/network` thay đổi, bạn không còn phải khởi động lại tất cả các giao diện nữa. Đơn giản chỉ cần chạy lệnh `/etc/init.d/network reload`. Lệnh này sẽ thực hiện một `ubus-call` tới `netifd`, yêu cầu nó tìm ra sự khác biệt giữa trạng thái runtime và cấu hình mới, sau đó chỉ áp dụng phần thay đổi đó.

## III. Thiết kế của `netifd`

![alt text](/image/netifd_layer.png)

`Netifd` hoạt động chính từ **Layer 2** tới **Layer 4** trong **Network management stack**:
- **Layer 2 - Device Layer**: Netifd quản lý device abstraction thông qua `struct device` (**device.h**) - đại diện cho các physical và virtual network interface tại link layer (bridges, VLANs, tunnels).
    ![alt text](/image/struct_device.png)

- **Layer 3 - Interface Layer**: Trung tâm core của netifd ở trong `struct interface` (**interface.h**) - đại diện cho các logical network connection (Layer này xử lý cấu hình IP,địa chỉ, định tuyến và cài đặt DNS settings).
- **Layer 4 - Protocol Layer**: Netifd quản lý các **protocol hander** thông qua **proto.c** và hỗ trợ nhiều protocol như static IP, DHCP và PPP.


## IV. Một số flow hoạt động trong netifd

### 1. Main execution flow (main.c)
![alt text](/image/main.flow.png)

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
![alt text](/image/config_flow.png)

Quá trình loading config được handle bởi `config_init_all()`. Luồng xử lý thực hiện;
- Load UCI package: Khởi tạo cấu hình 2 package **network, wireless**
- Khởi tạo các component: Device, VLAN, interface, IP setting, rule, global, wireless.
- Kích hoạt các cấu hình: Reset các device cũ, khởi tạo các pending device và bắt đầu các interface


## V. Chi tiết về các layer trong kiến trúc core system (Phân tích từ source code)
### 1. System interface layer
![alt text](/image/system_layer_archi.png)

Layer này xây dựng dựa trên 2 cơ chế giao tiếp chính với **Linux kernel** là **netlink sockets** và **ioctl calls**. System layer được khởi tạo thông qua `system_init()` ( gồm 4 setup chính phục vụ cho việc setup những socket cần thiết và các event handler):

![alt text](/image/system_init.png)

#### Netlink communication
Netlink là cơc chế giao tiếp chính với networking subsystem của Linux kernel. Như đã tìm hiểu netlink ở phần trước, Netlinks dùng giao tiếp giữa kernel và userspace. Trong trường hợp này, nó phục vụ cho việc monitor các event đến từ kernel thông qua các **callback function**:
- `cb_rtnl_event()`: Xử lý những routing netlink event (như thay đổi interface state)
- `handle_hotplug_event()`: Xử lý những device hotplug event

![alt text](/image/system_event_handler.png)

### 2. Device management layer
![alt text](/image/device_layer_archi.png)

Layer này là abstract của tập các loại device được tìm thấy trong Openwrt system, nó cung cấp 1 interface thống nhất phục vụ cho việc cấu hình và điều khiển 

#### Core data structures
Cái concept cốt lõi của layer này dựa trên 1 cấu trúc `struct device` - nơi đại diện của 1 device mạng (physical hoặc virtual). Mỗi device thuộc về 1 kiểu `struct device_type` cụ thể:

![alt text](/image/device_struct.png)

Device layer hỗ trợ nhiều `device_type`thông qua cơ chế đăng ký `device_type`. Mỗi `device_type` được đăng ký với hệ thống thông qua `device_type_add()` (còn mỗi `device` được tạo thông qua `device_create()`). Dưới đây là các `device_type`:
- simple_device_type;
- bridge_device_type;
- tunnel_device_type;
- vlan_device_type;
- macvlan_device_type;

#### Device state
Device states hoạt động the luồng dưới đây:

![alt text](/image/device_state.png)


Ngoài ra, layer cung cấp 1 cấu trúc `struct device_user` - giúp các thành phần khác có thể đăng ký người dùng với thiết bị cụ thể để nhạn thông báo về device event.

### 3. Interface management layer
![alt text](/image/interface_layer.png)

Layer này nằm giữa device layer và protocol handler. Nó cung cấp APi nhất quán phục vụ cho việc điều khiển và giám sát các interface mạng.

#### Core data structures
Core data structure trong layer này là `struct interface` - chứa tất cả thông tin về interface mạng (config, state, relationship)

![alt text](/image/interface_struct.png)

#### Interface state machine
![alt text](/image/interface_state.png)

Sự chuyển dịch interface state được trigger bởi:
- External request (settting interface up/down)
- Device state changes (link up/down, device removal)
- Protocol handler events (config success/failure)
- Configuration changes

#### Interface events và dependencies
Layer này sử dụng thông báo event để quảng bá sự thay đổi state tới các thành phần dependent:

![alt text](/image/interface_event.png)

Khi 1 interface thay đổi state, thông báo được gửi tới:
1. Interface users - thành phần dependence với interface
2. Protocol handler - để xử lý các action cụ thể của protocol
3. External system - thông qua hotplug event
4. Ubus - phục vụ cho việc giám sát và điều khiển

Interface user là thành phần dependence với interface. Nó đăng ký callback thông qua `interface_add_user()` để nhận được thông báo về interface event (IFEV_UP. IFEV_DOWN):
![alt text](/image/interface_dependence.png)
