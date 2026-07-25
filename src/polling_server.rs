/// Copyright 2026 Theo Lincke
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
use crate::{
    error::Error,
    protocol::{ConnectionStateMachine, HandlerState},
    robin_hood_ht::{AccessRes, InsertRes, RobinHoodHt},
};
use mio::{
    Events, Interest, Poll, Token,
    net::{TcpListener, TcpStream},
};
use std::{io, net::SocketAddr};

/// A Connection Collection is a statically sized
/// collection of connections that you can add and
/// remove on demand
pub struct ConnectionCollection {
    token_to_socket: RobinHoodHt<usize, usize, 10>,
    connections: [Option<ConnectionStateMachine>; 10],
    clock: usize,
    size: usize,
}

impl ConnectionCollection {
    /// Creates a new collection - all
    /// connections are absent at the start
    fn new() -> Self {
        ConnectionCollection {
            token_to_socket: RobinHoodHt::new(0),
            connections: std::array::from_fn(|_| None),
            clock: 0,
            size: 0,
        }
    }

    fn is_full(&self) -> bool {
        self.size == self.connections.len()
    }

    /// Gets a mutable connection if it exists
    /// otherwise None
    fn get_mut(&mut self, token: Token) -> Option<&mut ConnectionStateMachine> {
        match self.token_to_socket.get(token.0) {
            AccessRes::DoesntExist => None,
            AccessRes::Success(idx) => Some(
                self.connections[idx]
                    .as_mut()
                    .expect("conn hash map mismatch"),
            ),
        }
    }

    /// Deletes a connection from the collection
    fn delete(&mut self, token: Token) {
        match self.token_to_socket.delete(token.0) {
            AccessRes::DoesntExist => {
                unreachable!("Can only delete tokens that exist");
            }
            AccessRes::Success(idx) => {
                self.connections[idx] = None;
            }
        }
    }

    /// Inserts a token into the collection
    /// and creates the new Connection too
    ///
    /// assumes that token doesn't exist
    /// assumes there's room (call is_full
    /// before calling this function)
    fn insert(&mut self, token: Token, socket: TcpStream) {
        assert!(!self.is_full());

        for _ in 0..self.connections.len() {
            match &self.connections[self.clock] {
                // Exists - skip it and move on
                Some(_) => {
                    continue;
                }

                // Doesn't exist - use this slot
                None => {
                    let idx = self.clock;
                    self.clock = (self.clock + 1) % self.connections.len();
                    match self.token_to_socket.insert(token.0, idx) {
                        InsertRes::Full => {
                            // If connections is not full
                            // then the hash map should also
                            // not be full
                            unreachable!("Connections isn't full so hash map shouldn't be full");
                        }
                        InsertRes::Success => {
                            self.connections[idx] = Some(ConnectionStateMachine::new(socket));
                            self.size += 1;
                            return;
                        }
                        InsertRes::Exists => {
                            unreachable!("Insert expects no duplicates");
                        }
                    }
                }
            }
        }

        unreachable!("Insert expects a full check before");
    }
}

#[cfg(test)]
mod test {
    use std::error::Error;

    use super::*;

    #[test]
    fn smoke_test() -> Result<(), Box<dyn Error>> {
        let address = "127.0.0.1:0".parse()?;
        let listener = TcpListener::bind(address)?;

        let mut col = ConnectionCollection::new();

        let stream = TcpStream::connect(listener.local_addr()?)?;
        col.insert(Token(0), stream);

        Ok(())
    }
}

/// A polling server maintains a collection of connections,
/// and a polling state
pub struct PollingServer {
    listener: TcpListener,
    poll: Poll,
    events: Events,
    next_socket: usize,
    connections: ConnectionCollection,
}

impl PollingServer {
    /// Opens a new polling server on the specified address
    pub fn new(addr: &str) -> Result<Self, Error> {
        let addr = addr.parse::<SocketAddr>()?;
        let mut listener = TcpListener::bind(addr)?;

        let poll = Poll::new()?;
        poll.registry()
            .register(&mut listener, Token(0), Interest::READABLE)?;

        Ok(Self {
            listener,
            poll,
            events: Events::with_capacity(100),
            next_socket: 1,
            connections: ConnectionCollection::new(),
        })
    }

    pub fn execute_once(&mut self) -> Result<(), Error> {
        // First - do the poll
        self.poll.poll(&mut self.events, None)?;

        // Do all accepts at the end
        let mut should_accept = false;

        for event in &self.events {
            match event.token() {
                // Token 0 is the Server token - this just means
                // we need to accept - do it later so we
                // can process connections first
                Token(0) => {
                    should_accept = true;
                }

                // Otherwise, do a read or write operation
                token => {
                    // Read and write closed
                    if event.is_read_closed() || event.is_write_closed() {
                        println!("Closed connection");

                        let conn = self.connections.get_mut(token).expect("Error");
                        self.poll.registry().deregister(&mut conn.stream)?;
                        self.connections.delete(token);

                        continue;
                    }

                    let conn = self.connections.get_mut(token).expect(
                        "It's impossible to have an absent connection for a given event token",
                    );

                    let before = conn.state;
                    conn.step();
                    let after = conn.state;

                    // Change transitions
                    match (before, after) {
                        (HandlerState::Reading(_), HandlerState::Writing(_)) => {
                            assert!(event.is_readable());

                            // Re register in write mode
                            self.poll
                                .registry()
                                .reregister(&mut conn.stream, token, Interest::WRITABLE)
                                .unwrap();
                        }
                        (HandlerState::Processing(_), HandlerState::Writing(_)) => {
                            assert!(event.is_readable());

                            // Register in write mode
                            self.poll
                                .registry()
                                .register(&mut conn.stream, token, Interest::WRITABLE)
                                .unwrap();
                        }
                        (HandlerState::Reading(_), HandlerState::Processing(_)) => {
                            // Unregister this stream
                            self.poll.registry().deregister(&mut conn.stream).unwrap();
                        }
                        (HandlerState::Writing(_), HandlerState::Reading(_)) => {
                            // Re register in read mode
                            self.poll
                                .registry()
                                .reregister(&mut conn.stream, token, Interest::READABLE)
                                .unwrap();
                        }
                        _ => {}
                    }
                }
            }
        }

        // Accept new connections at the end so that
        // connection churn is done
        if should_accept {
            self.accept_all()?;
        }

        Ok(())
    }

    /// Accepts all requested connections on a server socket
    fn accept_all(&mut self) -> Result<(), Error> {
        loop {
            match self.listener.accept() {
                Ok((mut socket, addr)) => {
                    println!("Accepted new connection from: {}", addr);

                    if self.connections.is_full() {
                        let _ = socket.shutdown(std::net::Shutdown::Both);
                        drop(socket);
                        continue;
                    }

                    // Generate a new token
                    let token = Token(self.next_socket);
                    self.next_socket += 1;

                    // Register this socket with this token
                    self.poll
                        .registry()
                        .register(&mut socket, token, Interest::READABLE)?;

                    // insert this token and socket into the map
                    self.connections.insert(token, socket);
                }
                Err(e) if e.kind() == io::ErrorKind::WouldBlock => return Ok(()),
                Err(e) => return Err(Error::from(e)),
            }
        }
    }
}

#[cfg(test)]
mod test_polling_server {
    use crate::error::Error;
    use crate::polling_server::PollingServer;
    use std::io::{Read, Write};
    use std::net::TcpStream;

    const CONN_BUFFER_SIZE: usize = 2048;

    pub struct TestClient {
        stream: TcpStream,
        buffer: [u8; CONN_BUFFER_SIZE],
    }

    impl TestClient {
        pub fn new(addr: &str) -> Self {
            let stream = TcpStream::connect(addr).unwrap();

            TestClient {
                stream,
                buffer: [0; CONN_BUFFER_SIZE],
            }
        }

        fn send(&mut self, msg: &[u8]) {
            if msg.len() > CONN_BUFFER_SIZE - 4 {
                panic!("Message is too big");
            }

            let len = msg.len() + 4;
            self.buffer[0..4].copy_from_slice(&(len as u32).to_be_bytes());
            self.buffer[4..len].copy_from_slice(msg);

            let n = self.stream.write(&self.buffer[0..len]).unwrap();
            println!("Wrote {:?} bytes", n);
        }

        fn recv(&mut self) {
            let mut read = 0;
            while read < 4 {
                read += self.stream.read(&mut self.buffer[read..4]).unwrap();
                println!("Read {:?} bytes", read);
            }
            let len = u32::from_be_bytes(self.buffer[0..4].try_into().unwrap()) as usize;
            println!("Msg {:?} bytes", len);

            while read < len {
                read += self.stream.read(&mut self.buffer[read..len]).unwrap();
                println!("Read {:?} bytes", read);
            }
        }
    }

    #[test]
    fn test() -> Result<(), Error> {
        let mut server = PollingServer::new("127.0.0.1:9090")?;
        let mut client1 = TestClient::new("127.0.0.1:9090");
        let mut client2 = TestClient::new("127.0.0.1:9090");

        println!("Sending");
        client1.send("Hello World".as_bytes());
        client2.send("Hello World".as_bytes());

        println!("Executing");
        server.execute_once()?; // Accept
        server.execute_once()?; // Read
        server.execute_once()?; // Write

        println!("Receiving");
        client1.recv();
        client2.recv();

        Ok(())
    }
}
