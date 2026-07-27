const std = @import("std");
const net = std.net;
const posix = std.posix;

pub fn server() !void {
    const address = try net.Address.parseIp4("127.0.0.1", 8080);

    // 1. Create a listening socket equipped with Non-Blocking flags
    const socket_fd = try posix.socket(
        address.any.family,
        posix.SOCK.STREAM | posix.SOCK.NONBLOCK,
        posix.IPPROTO.TCP,
    );
    defer posix.close(socket_fd);

    // Allow quick reuse of the address port
    try posix.setsockopt(socket_fd, posix.SOL.SOCKET, posix.SO.REUSEADDR, &std.mem.toBytes(@as(c_int, 1)));
    try posix.bind(socket_fd, &address.any, address.getOsSockLen());
    try posix.listen(socket_fd, 128);

    std.debug.print("Polling server listening on 127.0.0.1:8080...\n", .{});

    // 2. Setup the polling array tracking file descriptors
    // Index 0 is always reserved for our listening socket
    var poll_fds = std.ArrayList(posix.pollfd).init(std.heap.page_allocator);
    defer poll_fds.deinit();

    try poll_fds.append(.{
        .fd = socket_fd,
        .events = posix.POLL.IN,
        .revents = 0,
    });

    var buffer: [1024]u64 = undefined;

    // 3. The main Event Loop
    while (true) {
        // Blocks until an event occurs or time out (timeout of -1 means wait indefinitely)
        const ready_count = try posix.poll(poll_fds.items, -1);
        if (ready_count == 0) continue;

        var i: usize = 0;
        while (i < poll_fds.items.len) {
            const pfd = &poll_fds.items[i];

            // If nothing happened on this file descriptor, move on
            if (pfd.revents == 0) {
                i += 1;
                continue;
            }

            if (pfd.fd == socket_fd) {
                // Handle new incoming connections on the listening socket
                if ((pfd.revents & posix.POLL.IN) != 0) {
                    while (true) {
                        var client_address: posix.sockaddr = undefined;
                        var client_address_len: posix.socklen_t = @sizeOf(posix.sockaddr);

                        // Accept the new socket connection safely in non-blocking mode
                        const client_fd = posix.accept(socket_fd, &client_address, &client_address_len, posix.SOCK.NONBLOCK) catch |err| {
                            if (err == error.WouldBlock) break; // No more pending connections
                            return err;
                        };

                        // Register the new client socket for data arrival notifications
                        try poll_fds.append(.{
                            .fd = client_fd,
                            .events = posix.POLL.IN,
                            .revents = 0,
                        });
                        std.debug.print("New client connected on fd {}\n", .{client_fd});
                    }
                }
                i += 1;
            } else {
                // Handle data arriving from an existing client socket
                var closed = false;
                if ((pfd.revents & posix.POLL.IN) != 0) {
                    const bytes_read = posix.read(pfd.fd, &buffer) catch |err| {
                        if (err == error.WouldBlock) {
                            i += 1;
                            continue;
                        }
                        0; // Handle other errors as a client disconnect
                    };

                    if (bytes_read == 0) {
                        closed = true;
                    } else {
                        // Echo data back to the sender
                        _ = try posix.write(pfd.fd, buffer[0..bytes_read]);
                    }
                }

                // If client hung up or threw an unrecoverable error
                if (closed or (pfd.revents & (posix.POLL.HUP | posix.POLL.ERR)) != 0) {
                    std.debug.print("Client fd {} disconnected\n", .{pfd.fd});
                    posix.close(pfd.fd);
                    _ = poll_fds.swapRemove(i);
                    // Do not increment i, because swapRemove shifted a new item into index i
                } else {
                    i += 1;
                }
            }
        }
    }
}
