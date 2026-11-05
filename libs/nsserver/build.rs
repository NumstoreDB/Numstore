// Vendored protoc so the build needs nothing from the system.
fn main() {
    std::env::set_var("PROTOC", protoc_bin_vendored::protoc_bin_path().unwrap());
    tonic_prost_build::compile_protos("proto/numstore.proto").unwrap();
}
