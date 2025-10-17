# Mục lục
- [1. Giới thiệu](#1-giới-thiệu)
    - [1.1 Mục đích](#11-mục-đích)
    - [1.2 Phạm vi](#12-phạm-vi)
- [2. Mô tả chung hệ thống](#2-mô-tả-chung-hệ-thống)
- [2. Yêu cầu chức năng](#3-yêu-cầu-chức-năng)
- [4. Yêu cầu khác](#4-yêu-cầu-khác)
- [5. Phụ lục](#5-phụ-lục)
    - [5.1 Tài liệu tham khảo](#51-tài-liệu-tham-khảo)

---

## 1. Giới thiệu
### 1.1 Mục đích
Module **Schedule reboot** cho phép người dùng cấu hình thời gian trên giao diện web, tự động khởi động lại thiết bị theo thời gian định sẵn giúp duy trì độ ổn định của hệ thống.
### 1.2 Phạm vi
Module chạy trên firmware OpenWrt, bao gồm:
- Giao diện cấu hình trên LuCI
- Xử lý lưu cấu hình bằng `uci`
- Tác vụ reboot thực hiện bởi cron job
## 2. Yêu cầu chức năng
|STT | Mô tả chức năng                             | Chi tiết                                               |
|----| ------------------------------------------- | ------------------------------------------------------ |
| 01 | Hiển thị trang cấu hình Schedule Reboot     | Trang nằm tại `System → Schedule Reboot`.              |
| 02 | Cho phép bật/tắt chức năng reboot theo lịch | Tùy chọn “Enable Schedule Reboot”.                     |
| 03 | Cho phép cấu hình thời gian reboot          | Người dùng nhập `hour`, `minute`, `day`, `weekday`.    |
| 04 | Lưu cấu hình qua `uci`                      | Cấu hình được lưu tại `/etc/config/schedule_reboot`.   |
| 05 | Tự động thêm job vào `crontab`              | Khi cấu hình thay đổi, script cập nhật cron tương ứng. |
| 06 | Hỗ trợ reboot thủ công ngay lập tức         | Nút “Reboot Now” gọi `ubus call system reboot`.        |

## 4. Yêu cầu khác

## 5. Phụ lục
### 5.1 Tài liệu tham khảo