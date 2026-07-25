enum ConnectionState {
    WaitingForServerReady,
    WaitingForOk,
    WaitingToSend,
}

impl ConnectionState {
    fn recv_server_ready(&mut self) {}
    fn send_create(&mut self) {}
    fn send_delete(&mut self) {}
    fn send_remove(&mut self) {}
}
