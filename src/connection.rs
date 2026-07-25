use crate::error::Error;
use std::io::{Read, Write};
use std::net::TcpStream;

const CONN_BUFFER_SIZE: usize = 2048;

pub struct Client {
    stream: TcpStream,
    buffer: [u8; CONN_BUFFER_SIZE],
}

impl Client {
    fn new(addr: &str) -> Self {
        let stream = TcpStream::connect(addr).unwrap();

        Client {
            stream,
            buffer: [0; CONN_BUFFER_SIZE],
        }
    }

    fn send(&mut self, msg: &[u8]) -> Result<(), Error> {
        assert!(msg.len() <= CONN_BUFFER_SIZE - 4);

        let len = msg.len() + 4;

        // Length prefix
        self.buffer[0..4].copy_from_slice(&(len as u32).to_be_bytes());

        // Data
        self.buffer[4..len].copy_from_slice(msg);

        let mut written = 0;
        while written < len {
            written += self.stream.write(&self.buffer[written..len])?;
        }

        Ok(())
    }

    fn recv(&mut self) -> Result<(), Error> {
        // First, read the length prefix
        let mut read = 0;
        while read < 4 {
            read += self.stream.read(&mut self.buffer[read..4]).unwrap();
        }

        // Check if length is valid
        let len = u32::from_be_bytes(self.buffer[0..4].try_into().unwrap()) as usize;
        if len > CONN_BUFFER_SIZE {
            return Err(Error::ClientRecievedInvalidServerMessage);
        }

        // Read the rest of the data
        while read < len {
            read += self.stream.read(&mut self.buffer[read..len]).unwrap();
        }

        Ok(())
    }
}
