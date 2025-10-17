## I. Overview lại một số cái
### A. Về cấu trúc source code
Source code chính OpenWrt sẽ gọi đến tất cả các source code phụ (procd, uci, ubus, rpcd...) thông qua các Makefile ở trong `package/system`. 

### B. Về init script (/etc/init.d/*)
Mỗi file trong init script tương ứng với 1 service 
```sh
start_service() {
    procd_open_instance
    procd_set_param command /usr/sbin/mydaemon
    procd_close_instance

    procd_add_reload_trigger network firewall
}
```
Service này muốn theo dõi thay đổi file cấu hình UCI `network` và `firewall` thông qua hàm `procd_add_reload_trigger`. Hàm này thực hiện gì ? Khi có sự thay đổi cấu hình (`uci commit ...`), thì nó chỉ ghi cấu hình từ RAM xuống file `/etc/config/network`. Nó không áp dụng thay đổi đó ngay và không sinh ra event nào cho `procd`. Luc nay, no sẽ gọi hàm `reload_config` để chạy lệnh `/sbin/reload_config`:
```sh
reload_config() {
    uci_apply_defaults
    ubus call service event '{ "type": "config.change", "data": {} }'
}
```
Hàm này phát đi event `config.change` qua ubus (không cần biết file uci có thay đổi không, chỉ cần uci commit thì nó sẽ phát event):
```sh
ubus call service event '{ "type": "config.change", "data": { "package": "network" } }'
```
`procd` sẽ lắng nghe event này (lắng nghe như nào sẽ giải thích ở phần sau), khi nhận được event thì nó sẽ tra bảng trigger đã đăng ký `procd_add_reload_trigger`, trong đó service nào quan tâm thì procd sẽ xét tiếp. Trước khi gọi reload service, procd sẽ so sánh checksum (md5) lần commit cũ với mới. Nếu checksum khác nhau, procd sẽ gọi init script của service đó với action `reload`.
### C. Về uci (/etc/config/*)
Mỗi file là 1 UCI config file - cấu hình cho của từng service, dạng file 


### D. Về ubus
Giải thích về câu chuyện lắng nghe event ở trên, khi `reload_config` phát ra event `config.change` thì procd sẽ lắng nghe kiểu gì ? Polling hay không Polling ?

Giải thích: Mỗi client (procd, reload_config) ở 1 UNIX socket với ubusd (`/var/run/ubus.sock`), khi procd đăng ký lắng nghe (`ubus_register_event_handler`). ubus sẽ ghi nhận filter event mà procd quan tâm. Khi reload_config gửi `ubus_send_event("config.change", blob);`, ubusd duyệt client đăng ký filter `config.change` và push msg qua socket cho client đó (procd). Vậy thì procd nhận được event như nào ? Procd sử dụng `uloop` (`epoll`) để lắng nghe trên socket fd. Khi có dữ liệu mới, `uloop` gọi callback tương ứng. (Ngoài ra còn 1 cách nữa để biết event tới đó chính là sử dụng timer callback)

### E. Về hotplug
OpenWrt có hệ thống hotplug, nó dùng để gọi tự động script trong firmware tại `/etc/hotplug.d/<subsystem>/` khi subsystem đó gửi event. Các subsytem đó là:
- **ntp**: Vì các board khi boot sẽ tự set time là 00:00 (sai với time thực tế).Khi `ntpd` phát hiện ra thời gian hợp lệ (hoặc thay đổi `stratum` - thời gian hợp lệ được đồng bộ) => nó sẽ gọi tất cả các scipt trong `/etc/hotplug.d/ntp/` (gửi event `ACTION=stratum`).
- **block**: Gọi khi thiết bị (USB, SD, SATA) được cắm mở.
- **button**: Gọi khi nút được nhấn.
- **iface**: Gọi khi interface mạng thay đổi.
- **ipsec**:
- **neigh**:
- **net**: 
- **tftp**:
- **usb**:
- **dhcp**: 
- **firmware**:

### F. Về procd
Trong `/lib/functions/procd.sh ` là nơi các hàm `procd_*` được định nghĩa - cũng chính là các API mà init script dùng để nói chuyện với tiến trình **procd**. Các hàm tiền tố `_procd_*` làm việc nội bộ và `_procd_wrapper` cuối file sẽ sinh ra các hàm công khai `procd_*` mà init script gọi. Nội dung được đóng gói bằng **JSON (jshn.sh)** và gửi qua `ubus call service...` tới `procd`. (init script -> procd.sh -> ubus -> )


## II. Truy cập ubus qua HTTP
Đây chính là cơ chế giúp **LuCI (frontend JS)** nói chuyện được với **ubus** và **rpcd** ở backend:
- **ubusd** - daemon IPC nội bộ, quản lý message bus.
- **rpcd** - daemon plugin, đăng ký API vào ubus.
- **uhttpd** (web server) + `uhttpd-mod-ubus` (đây là 1 plugin) - cổng HTTP để browser JS gọi vào ubus.

Bình thường ubus chỉ IPC nội bộ qua socket. Muốn browser truy cập, phải qua HTTP JSON API do `uhttpd-mod-ubus` cung cấp. Dưới đây là luồng hoạt động:
1. Frontend JS gửi request HTTP POST đến `/ubus` (endpoint do plugin tạo ra).
2. `uhttpd` nhận request, chuyển cho `uhttpd-mod-ubus`.
3. Plugin này chuyển request thành message tới `ubusd`.
4. `ubusd` gọi đúng service trong `rpcd` hoặc daemon khác.
5. Kết quả trả về dạng JSON cho frontend.

### A. ACLs
Khi đăng nhập thông qua ssh, bạn có thể truy cập trực tiếp tới ubus. Nhưng khi bạn truy cập thông qua `/ubus` endpoint ở uhttpd, uhttpd không cho phép gọi ubus trực tiếp, nó sẽ hỏi 1 thành phần **ACL** xem request này có được phép không (thành phần này chính là **rpcd** (qua `ubus session.*`) dùng để kiểm soát session và quyền). DƯới đây là cơ chế hoạt động:
1. Browser (qua LuCI mới) gọi HTTP-JSON-RPC đến `/ubus`
2. uhttpd chuyển lời gọi này thành `ubus call session access` để hỏi rpcd:
```sh
ubus call session access '{ 
  "ubus_rpc_session": "xxxxx", 
  "object": "requested-object", 
  "function": "requested-method" 
}'
```
3. rpcd kiểm tra file ACL trong `/usr/share/rpcd/acl.d/*.json` để xem session đó có quyền hay không


## III. Tạo 1 package riêng
### A. Package basic
Thuật ngữ package có thể hiểu theo 2 nghĩa:
1. **Source package**: Đây là dịnh nghĩa 1 software package trong OpenWrt, thường là 1 thư mục chứa Makefile, file/, src/, patches/:

```bash
mypackage/
├── Makefile # Mô tả cách lấy source code (từ Git,...), cách build, cách đóng gói, footer đặc thù Openwrt (required)
├── files/ # Chứa các script init (`/etc/init.d/...`), cấu hình mặc định (`/etc/config/...`) (optinal)
│   └── etc/
│       ├── config/
│       │   └── mypackage
│       └── init.d/
│           └──mypackage
├── patches/ # Chứa các file `.patch` để vá lại source code gốc nhằm tương thích với OpenWrt (optional)
│   ├── 001-fix-build.patch
│   └── 010-fix-runtime-bug.patch
└── src/ # Dùng khi không tải source code từ ngoài mà chứa source trực tiếp (optional)
    ├── main.c
    └── helper.c
```

2. **Binary package**: đây là file `.ipk` là phần **source package** đã build ra (có thể cài binary package riêng vào board mà không cần nạp lại toàn bộ firm vào board)

### B. Tạo package
- files:
    - UCI config file: viết theo cấu trúc đã định
    - init script: viết 2 hàm start và stop. start sẽ chạy bin của source code và chạy hàm `procd_add_reload_trigger`
- source code: xử lý việc đọc UCI config và convert nó sang crontab
- Makefile:


## IV. Guild về LuCI.js
Trước đây, LuCI cũ của OpenWrt dùng **Lua** và framework **CBI**. Khi mở 1 trang trên LuCI, router chạy code **Lua** (ở backend). Lua kết hợp dữ liệu từ UCI (cấu hình hệ thống) và reder thành HTML hoàn chỉnh, sau đó HTML được gửi về browser => Router tốn tài nguyên để render HTML làm router bị chậm, lag.

Bây giờ, LuCI loại bỏ dần Lua đi, Thay vào đó router chỉ trả về dữ liệu thô (JSON, UCI values...) thông qua ubus/rpcd. Phần giao diện, hiển thị, logic được chuyển sang browser của client bằng **LuCI-JavaScript API** (Code LuCI giờ chủ yếu là **JS chạy trên browser** thay vì Lua render HTML nữa) - là **Client Side Rendering** - render ở browser.

### A. Một số thành phần tạo lên trang web
#### 1. LuCI.view
`LuCI.view` là class cơ sở của tất cả các view trong LuCI. Mọi page trong LuCI đều là 1 module JS được định nghĩa bằng cách kế thừa từ `LuCI.view` (kế thừa các phương thức của LuCI.view). Mỗi file trong `luci-static/resources/view/...` đều có bắt đầu bằng `return view.extend({...});` chứa các phương thức override lại (kế thừa lại các phương thức):
1. `load()`: Load dữ liệu trước khi render, thường để lấy: UCI file (`uci.load()`), RPC từ hệ thống (`rpc.declare`), đọc file (`fs.read`), kết hợp nhiều Promise (`Promise.all([...])`) hoặc bất kỳ thao tác async (promise là 1 giá trị sẽ có trong tương lai. Ví dụ các hàm `uci.load` sẽ trả về promise thay vì 1 giá trị bình thường vì chúng cần đọc dữ liệu từ hệ thống, mất thời gian (hứa sẽ trả về )) . Giá trị `return` trong `load()` sẽ trở thành tham số của `render()`. Lời gọi hàm này được wrapped bởi `Promise.resolve()` - biến 1 giá trị hoặc promise thành 1 promise. 
2. `render()`: Hiển thị nội dung (DOM, form, table...), được gọi sau hàm `load()`.Lời gọi hàm này được wrapped bởi `Promise.resolve()` - biến 1 giá trị hoặc promise thành 1 promise. Giá trị `return` của `render()` được chèn vào main content 
3. `handleSave()`: Xử lý khi nhấn nút Save
4. `handleSaveApply()`: Xử lý khi nhấn nút Save & Apply
5. `handleReset()`: Xử lý khi nhấn Reset
6. `addFooter()`: Tạo footer với các nút

#### 2. LuCI.form

### B. Cấu trúc thư mục của LuCI
#### 1. Cấu trúc bên trong feeds/luci/
```bash
feeds/luci/
├── applications/     # Chứa các app mở rộng mà user có thể bật/tắt trong web UI
├── modules/          # Chứa các module chính của giao diện LuCI (phần khung quản trị) 
├── collections/      # 
├── libs/             # Chứa các thư viện dùng chung của LuCI (giống SDK của LuCI)
├── themes/           # Chứa các theme frontend (HTML, CSS, JS) cho UI (màu sắc, bố cục, )
└── protocols/        # Chứa các plugin mạng cho LuCI (PPPoE, 6in4, WireGuard...)
```
#### 2. Cấu trúc bên trong feeds/luci/modules/
```bash
feeds/luci/modules/
├── luci-base/              # Lõi LuCI: cung cấp thư viện chung, template, dispatcher, i18n...
├── luci-compat/            # Cho phép các app viết theo API cũ hoạt động trên LuCI mới
├── luci-mod-network/       # Quản lý mạng: các trang cấu hình mạng (interface, DHCP, wireless)
├── luci-mod-status/        # Trạng thái hệ thống
├── luci-mod-system/        # Cấu hình hệ thống: startup, reboot...
└── luci-mod-rpc/           # RPC API (JSON-RPC, ubus): cung cấp endpoint cho frontend JS gọi qua 
```

```
                ┌────────────────────┐
                │   Web Browser UI   │
                │ (HTML, JS, CSS)    │
                └────────┬───────────┘
                         │  JSON-RPC, HTTP
                         ▼
                ┌────────────────────┐
                │ luci-mod-network   │ ← Sub-module: Network
                │ luci-mod-system    │ ← Sub-module: System
                │ luci-mod-status    │ ← Sub-module: Status
                ├────────────────────┤
                │ luci-base          │ ← Nền tàng để tất cả luci-mod-* hoạt động 
                │ luci-compat        │ ← Compatibility
                │ luci-mod-rpc       │ ← RPC endpoint
                └────────────────────┘
```

#### 3. Cấu trúc bên trong 1 module cụ thể (luci-mod-system)
```bash
feeds/luci/modules/luci-mod-system/
├── Makefile
├── htdocs/
│   └── luci-static/resources/view/system/
│       ├── reboot.js
│       ├── admin.js
│       └── startup.js
├── luasrc/
│   ├── controller/
│   │   └── system.lua
│   ├── model/
│   │   └── cbi/system-admin.lua
│   └── view/
│       └── system/
│           └── reboot.htm
└── root/
    └── etc/
        └── luci-uploads/
```

## Reference
[1] https://openwrt.org/docs/techref/ubus#acls

[2] https://forum.openwrt.org/t/description-of-the-json-in-menu-d-and-acl-d/104013/3

[3] https://openwrt.org/docs/guide-developer/package-policies

[4] https://openwrt.org/docs/guide-developer/packages

[5] https://blog.freifunk.net/2023/07/01/gsoc-23-migrating-luci-apps-to-javascript-a-comprehensive-guide/?utm_source=chatgpt.com
