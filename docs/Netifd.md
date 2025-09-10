
## Netifd là gì?

`netifd` là một **daemon có khả năng RPC** được viết bằng ngôn ngữ C để truy cập tốt hơn vào các API của kernel với khả năng lắng nghe các **sự kiện netlink**.

## Tại sao chúng ta cần netifd?

Một điều mà `netifd` làm tốt hơn nhiều so với **các script cấu hình mạng cũ của OpenWrt** là xử lý các thay đổi cấu hình. Với `netifd`, khi file `/etc/config/network` thay đổi, bạn không còn phải khởi động lại tất cả các giao diện nữa. Đơn giản chỉ cần chạy lệnh `/etc/init.d/network reload`. Lệnh này sẽ thực hiện một `ubus-call` tới `netifd`, yêu cầu nó tìm ra sự khác biệt giữa trạng thái runtime và cấu hình mới, sau đó chỉ áp dụng phần thay đổi đó.

## Thiết kế của `netifd`

Các thành phần chính trong `trạng thái của netifd` bao gồm devices (thiết bị) và interfaces (giao diện):

### Device (Thiết bị)

Một device đại diện cho một **giao diện vật lý** (eth0, wlan0...), hoặc một **liên kết ảo** (tunnel, static routes, kết nối dựa trên socket)