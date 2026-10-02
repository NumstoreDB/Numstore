//! nsserver: gRPC front end for libnumstore.
//!
//! Shared by both binaries: `nsserver` (src/main.rs) and `nsclient`
//! (src/bin/nsclient.rs).

pub mod ffi;

pub mod proto {
    tonic::include_proto!("numstore.v1");
}
