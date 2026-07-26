use numstore::error::Error;
use numstore::nsclient::Client;
use std::io::{Read, Write};
use std::net::TcpStream;

fn main() -> Result<(), Error> {
    let mut client = Client::new("127.0.0.1:9090");
    client.send("Hello world".as_bytes())?;
    client.recv()?;
    Ok(())
}
