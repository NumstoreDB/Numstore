use std::io::{Read, Write};
use std::net::TcpStream;

use crate::error::Error;

const CONN_BUFFER_SIZE: usize = 2048;

pub struct Client {
    stream: TcpStream,
    buffer: [u8; CONN_BUFFER_SIZE],
}

impl Client {
    pub fn new(addr: &str) -> Self {
        let stream = TcpStream::connect(addr).unwrap();

        Client {
            stream,
            buffer: [0; CONN_BUFFER_SIZE],
        }
    }

    pub fn send(&mut self, msg: &[u8]) -> Result<(), Error> {
        assert!(msg.len() <= CONN_BUFFER_SIZE - 4);

        // Total length including size prefix before
        let len = msg.len() + 4;
        self.buffer[0..4].copy_from_slice(&(len as u32).to_be_bytes());
        self.buffer[4..len].copy_from_slice(msg);

        let mut written = 0;
        while written < len {
            written += self.stream.write(&self.buffer[written..len])?;
        }

        Ok(())
    }

    pub fn recv(&mut self) -> Result<(), Error> {
        // Read the first size prefix
        let mut read = 0;
        while read < 4 {
            read += self.stream.read(&mut self.buffer[read..4]).unwrap();
        }

        // The length of the buffer
        let len = u32::from_be_bytes(self.buffer[0..4].try_into().unwrap()) as usize;
        if len > CONN_BUFFER_SIZE {
            return Err(Error::ServerSentUnknownMessage);
        }

        // Read the rest of the data
        while read < len {
            let n = self.stream.read(&mut self.buffer[read..len])?;
            read += n;
        }

        Ok(())
    }
}
