use std::{
    io,
    sync::{
        Arc,
        atomic::{AtomicBool, Ordering},
    },
};

use numstore::{
    error::{Error, IOErrorData},
    polling_server::PollingServer,
};

fn main() -> Result<(), Error> {
    let mut server = PollingServer::new_default("127.0.0.1:9090")?;

    let running = Arc::new(AtomicBool::new(true));
    let r = running.clone();

    ctrlc::set_handler(move || r.store(false, Ordering::SeqCst))
        .expect("Error setting Ctrl-C handler");

    while running.load(Ordering::SeqCst) {
        match server.execute_once() {
            Ok(()) => continue,
            Err(Error::IOError(IOErrorData::IOError(e)))
                if e.kind() == io::ErrorKind::Interrupted => {}
            e => return e,
        }
    }

    println!("Goodbye!");
    Ok(())
}
