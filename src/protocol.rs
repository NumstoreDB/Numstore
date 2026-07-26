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
use std::{
    fmt,
    io::{self, Read, Write},
};

use mio::net::TcpStream;

/// [ ++++++++++_________________ ]
///            ^
///          rblen
#[derive(Clone, Copy, Debug)]
pub struct ReadState {
    rblen: usize,
}

/// [ ++++++++++_________________ ]
///            ^
///           len
#[derive(Clone, Copy, Debug)]
pub struct ProcessingState {
    len: usize,
}

/// [ --++++++++_________________ ]
///     ^      ^
///    wlen   len
#[derive(Clone, Copy, Debug)]
pub struct WriteState {
    len: usize,
    wlen: usize,
}

#[derive(Clone, Copy, Debug)]
pub enum HandlerState {
    Reading(ReadState),
    Processing(ProcessingState),
    Writing(WriteState),
}

impl fmt::Display for HandlerState {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            HandlerState::Reading(r) => write!(f, "Reading: {} read", r.rblen),
            HandlerState::Processing(p) => write!(f, "Processing: {} len", p.len),
            HandlerState::Writing(w) => write!(f, "Writing: (written, len) ({} {})", w.wlen, w.len),
        }
    }
}

pub struct ConnectionStateMachine {
    // Shared state
    pub stream: TcpStream,
    buffer: [u8; 2048],

    // Isolated state
    pub state: HandlerState,
}

impl ConnectionStateMachine {
    pub fn new(stream: TcpStream) -> Self {
        ConnectionStateMachine {
            stream,
            buffer: [0; 2048],
            state: HandlerState::Reading(ReadState { rblen: 0 }),
        }
    }

    pub fn step(&mut self) {
        match &mut self.state {
            HandlerState::Reading(state) => {
                loop {
                    let buffer = if state.rblen >= 4 {
                        let msg_len =
                            u32::from_be_bytes(self.buffer[0..4].try_into().unwrap()) as usize;

                        // Done reading
                        if state.rblen == msg_len {
                            self.state = HandlerState::Writing(WriteState {
                                len: state.rblen,
                                wlen: 0,
                            });
                            return;
                        }

                        // The remainder of the buffer
                        &mut self.buffer[state.rblen..msg_len]
                    } else {
                        // Just the front bit to read the size first
                        &mut self.buffer[state.rblen..4]
                    };

                    // Do the read
                    match self.stream.read(buffer) {
                        // Got 0 bytes - in everything I'm reading
                        // this is reliably an EOF signal (unless buffer size
                        // is 0, which is not the case)
                        //
                        // So This should return an error to the client
                        // saying malformed message
                        //
                        // Optionally you could expect n number of 0's?
                        // I'm not sure if that's actually necessary
                        Ok(0) => {
                            println!("Got 0 bytes read");
                            todo!("Send error message to client");
                        }

                        // We read some - we'll terminate on the next loop
                        // if our buffer is full
                        Ok(n) => {
                            println!("Read {:?} bytes", n);
                            state.rblen += n;
                            continue;
                        }

                        // Will Block - so terminate and stay in the Read state
                        Err(ref e) if e.kind() == io::ErrorKind::WouldBlock => {
                            return;
                        }

                        // Unrelated error
                        Err(e) => {
                            todo!("Error on read: {:?}", e);
                        }
                    }
                }
            }

            HandlerState::Processing(_) => {
                // Do nothing
                return;
            }

            HandlerState::Writing(state) => {
                loop {
                    // Done reading
                    if state.wlen == state.len {
                        return;
                    }

                    match self.stream.write(&mut self.buffer[state.wlen..state.len]) {
                        // 0 Bytes written?
                        Ok(0) => {
                            println!("Got 0 bytes written");
                            todo!("Error handler");
                        }

                        // Normal amount written, we'll
                        // terminate on the next loop
                        Ok(n) => {
                            println!("Write {:?} bytes", n);
                            state.wlen += n;
                            continue;
                        }

                        // Would block so just continue writing
                        Err(ref e) if e.kind() == io::ErrorKind::WouldBlock => {
                            return;
                        }

                        Err(e) => {
                            todo!("Error on write: {:?}", e);
                        }
                    }
                }
            }
        }
    }
}
