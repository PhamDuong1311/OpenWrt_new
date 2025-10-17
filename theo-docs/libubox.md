# I. Libubox overview
Libubox gồm nhiều subsystem có thể sủ dụng độc lập hoặc cùng nhau:

![](/image/core_component.png)

# II. Core event loop
Đây là 1 system trong libubox cung cấp 1 framework xử lý event nhanh, hiệu quả 1 cách bất đồng bộ trong OpenWrt. Nó cho phép app có thể giám sát và phản ứng với nhiều nguồn event cùng lúc như **file descriptor**, **process** và **signal** mà không bị blocking hay lãng phí chu kỳ CPU thông qua polling liên tục.

## 1. Architecture overview
Uloop event loop được thiết kế phục vụ cho việc xử lý các event nhận được thông qua `epoll()`:

![](/image/event_loop_archi.png)

## 2. Core event loop components
Core của event loop là hàm `uloop_run_events()`:
- Lấy các event từ backend của nền tảng cụ thể (`epoll()` cho Linux)
- Xử lý từng event
- Gọi hàm callback phù hợp cho mỗi 
### a. Event loop initialization and execution
Event loop phải được khởi tạo trước khi sử dụng và phải được dọn dẹp đúng cách sau đó:

![](/image/init_event_loop.png)

### b. File descriptor handling
Event loop cho phép monitor các fd để kiểm tra quyền read/write và điều kiện EOF (Có hàm add và delete fd để monitor). Khi event xảy ra trên 1 fd được monitor, callback function được đăng ký trên `struct uloop_fd` được gọi.

### c. Timeout management
Timeout cho phép lên lịch callback để chạy lại sau thời gian delay cụ thể (có hàm add, set, cancel timeout). Các timeout được lưu trữ trong linked list được sắp xếp theo thời gian hết hạn. 

### d. Process management
Event loop có thể monitor các child process và thông báo khi chúng terminate (Có hàm add và stop các process được monitor). Khi event loop nhận được tín hiệu `SIGCHLD`, nó sẽ gọi `waitpid()` để lấy status của child bị terminate và trigger callback phù hợp.

### e. Signal handling
Event loop có 1 cơ chế để xử lý các tín hiệu UNIX bất đồng bộ (có hàm add và delete 1 signal handler để xử lý các signal). Signal được định tuyến thông qua 1 pipe để tránh **race conditions** và đảm bảo chúng được xử lý trong context của event loop.

### f. Interval timers
Interval timer cung cấp 1 recurring callback theo khoảng thời gian cố định (Có hàm set, cancel các recurring timer)

## 3. Using uloop với file descriptors
### a. The uloop_fd structure
Nền tảng của fd trong uloop là `struct uloop_fd`. Cấu trúc này lưu trữ fd và hàm callback được gọi khi có event xảy ra trên fd:
```c
struct uloop_fd
{
	uloop_fd_handler cb;    // Hàm callback được gọi khi event xảy ra trên fd
	int fd;                 // fd theo dõi
	bool eof;               // Set là true khi EOF được phát hiện ở fd
	bool error;             // Set là true khi lỗi xảy ra ở fd
	bool registered;        // Chỉ ra liệu fd có được đăng ký với uloop không
	uint8_t flags;          // Flags hiện tại (ULOOP_READ, ULOOP_WRITE, ULOOP_EDGE_TRIGGER, ULOOP_BLOCKING)
};
```

Tham số flag nhận 1 trong 4 loại dưới đây:
- `ULOOP_READ`: monitor fd cho read event
- `ULOOP_WRITE`: monitor fd cho write event
- `ULOOP_EDGE_TRIGGER`: sử dụng edge-trigger notify thay vì level-trigger
- `ULOOP_BLOCKING`: khôn set fd thành non-blocking 

### b. Adding file descriptors to uloop
Để bắt đầu giám sát 1 file descriptor, cần khởi tạo 1 `struct uloop_fd` và đăng ký nó với uloop:
![](/image/add_fd.png)

### c. Handling file descriptor events
Khi event xảy ra trên fd đã giám sát, uloop gọi hàm callback đã đăng ký với fd, callback sẽ nhận 2 tham số:
1. Pointer trỏ tới `struct uloop_fd`
2. Bitfield của event xảy ra (ULOOP_READ, ULOOP_WRITE)

![](/image/callback_fd.png)

### d. Remove file descriptor
Khi hoàn thành việc monitor fd, thì phải remove nó từ uloop bằng sử dụng `uloop_fd_delete()`

## 4. Timers và process handling
Timers và process handling trong libubox cung cấp:
1. One-time timers - Thực thi callback sau 1 thời gian delay cụ thể
2. Interval timers - Thực thi callback lặp lại với interval đã định nghĩa
3. Process monitoring - Theo dõi child process và nhận thông báo khi exit
4. Integration with the event loop - Tất cả các hành động sẽ không bị blocking và được tích hợp vào uloop

![](/image/link_component_uloop.png)

### a. Timer management
#### One-time timers
Cơ chế này thực thi callback một lần sau khi hết delay (timeout). Nền tảng của nó là `struct uloop_timeout`. Dưới đây là các bước tạo và sử dụng 1 one-time timers:
1. Định nghĩa 1 `struct uloop_timeout`
2. Định nghĩa callback với tham số `void(*)(struct uloop_timeout *t)`
3. Khởi tao bộ timer bằng việc setting vào trường callback
4. Set bộ timer với `uloop_timeout_set()`

Ví dụ:
```c
// Hàm này sẽ được gọi sau khi loại bỏ timeout ban đầu (uloop_timeout_cancel(t))
static void timer_cb(struct uloop_timeout *t) {
    uloop_timeout_set(t, 1000); // Hàm này sẽ gọi lại để set lại timeout mới => nó sẽ giống cơ chế interval timers (nhưng bản chất của nó là xóa timeout và tạo timeout mới)
}

// In initialization code:
struct uloop_timeout timer;
timer.cb = timer_cb;
uloop_timeout_set(&timer, 1000); // Lần đầu nó sẽ gọi timer ở đây
```

#### Interval timers
Cơ chế này thực thi callback lặp lại với interval cụ thể. Nền tảng của nó là `struct uloop_interval` và sử dụng các cơ chế khác nhau dựa trên platform (epoll() cho Linux).

### b. Process handling
Cơ chế này cho phép theo dõi các child process và nhận thông báo khi nó exit. Nền tảng của nó là `struct uloop_process`. Dưới đây là các bước tạo và sử dụng cơ chế này:
1. Định nghĩa 1 `struct uloop_process`
2. Định nghĩa callback với tham số `void(*)(struct uloop_process *p, int ret)`
3. Gán callback và child fork() vào field của struct vừa tạo
4. Thêm process muốn monitor vào `uloop_process_add()`

### Integration with the event loop
Cả timers và process handlers được tích hợp với hệ thống event loop. Sự tích hợp này xuất hiện trong main loop function `uloop_run_timeout()`
![](/image/integration_uloop.png)


